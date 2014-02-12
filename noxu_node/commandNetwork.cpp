#include "commandNetwork.h"

//========== PRIVATE ==========
//---------- inbound ----------
void CommandNetwork::start(){
    if (listening) return;
    //printf("<<LISTEN START\n");
    //todo: set channel
    radio->openReadingPipe(1, epBroadcast.pipe);
    radio->startListening();
    listening = true;
}
void CommandNetwork::stop(){
    if (!listening) return;
    //printf("<<LISTEN STOP\n");
    radio->stopListening();
    listening = false;
}
void CommandNetwork::processInbound(){
    uint8_t pipe;
    if (radio->available(&pipe)){
        byte* buffer = (byte*)malloc(sizeof(byte) * bufferSize);
        if (radio->read(buffer, bufferSize)) {
            printf("<<INBOUND(%x) -> ", pipe);
            printBytes(buffer, bufferSize);
            CommandMessage *msg = new CommandMessage(buffer, bufferSize);
            if (msg->validate()){
                receive(msg);
            }
            delete msg;
        }
        free(buffer);
    }
}
void CommandNetwork::receive(CommandMessage *msg) {
    msg->print("<<BROADCAST");
    switch (msg->instruction)
    {
    case RES_NETWORKID:
        printf(">>RES_NETWORKID -> %d\r\n", msg->data[2]);
        epDevice.id = msg->data[2];
        break;
    default:
        if (msg->fromCommander && msg->lastHop == epDevice.id){//message is for this node
            (*receiveHandler)(msg->instruction, msg->data, msg->dataLength);
        }
        else if (msg->fromCommander && msg->hopCount > 0){//message is routed back through this node
            //remove last hop and forward message to next hop
            byte lastHop = msg->removeLastHop();
            queueMessage(lastHop, msg);
        }
        else if (msg->fromCommander){//big problem, maybe send out notification?
        }
        else if (!msg->fromCommander){//forward message onto commander
            //add current node id and send to commander
            msg->addHop(epDevice.id);
            queueMessage(epBroadcast.id, msg);
        }
        break;
    }
    (*receiveHandler)(msg->instruction, msg->data, msg->dataLength);
}

//---------- outbound ----------
void CommandNetwork::queueMessage(byte pipeId, CommandMessage *msg){
    byte* buffer = msg->buildBuffer();
    //printf(">>QUEUEMESSAGE -> ");
    //printBytes(buffer, bufferSize);
    outboundQueueLength++;
    realloc(outboundQueue, outboundQueueLength * sizeof(byte));
    outboundQueue[outboundQueueLength-1] = buffer;
    realloc(outboundQueuePipe, outboundQueueLength * sizeof(byte));
    outboundQueuePipe[outboundQueueLength-1] = pipeId;
}

void CommandNetwork::processOutbound(){
    //broadcast
    uint64_t ct =0;
    while(ct < outboundQueueLength){
        uint64_t pipe = (basePipe + outboundQueuePipe[ct]);
        printf(">>OUTBOUND(");
        printLL(pipe, 16);
        printf(") -> ");
        printBytes(outboundQueue[ct], bufferSize);
        sendBuffer(pipe, outboundQueue[ct]);
        free(outboundQueue[ct]);
        ct++;
    }
    outboundQueueLength = 0;
    realloc(outboundQueue, 0);
    realloc(outboundQueuePipe, 0);
}
void CommandNetwork::sendBuffer(uint64_t pipe, byte buffer[]) {
    bool wasListening = listening;
    if (listening)
        stop();

    //todo: set channel
    radio->openWritingPipe(pipe);
    radio->write(buffer, bufferSize);

    if (wasListening)
        start();
}

//---------- control ----------
void CommandNetwork::resetDeviceId(){
    epDevice.id = 0;
    epDevice.pipe = 0;
    tempId = random(10000, 65535);
}

//========== PUBLIC ==========
//---------- constructors ----------
CommandNetwork::CommandNetwork(const byte _bufferSize, unsigned int _maxLoopInterval, unsigned int _runInterval, unsigned int _receiveDuration){
    bufferSize = _bufferSize;
    maxLoopInterval = _maxLoopInterval;
    runInterval = _runInterval;
    receiveDuration = _receiveDuration;
    outboundQueue = (byte**)malloc(0);
    outboundQueuePipe = (byte*)malloc(0);
    outboundQueueLength = 0;
}

//---------- lifetime ----------
void CommandNetwork::setup(){
    printf("\r\n====== RFnode ======\r\n");
    listening = false;
    randomSeed(analogRead(0));
    resetDeviceId();

    radio = new RF24(9, 10);
    radio->begin();
    radio->setPALevel(RF24_PA_MAX);
    radio->setChannel(0x4c);
    radio->setCRCLength(RF24_CRC_16);
    radio->setDataRate(RF24_1MBPS);
    radio->setRetries(0,0);
    radio->setPayloadSize(bufferSize);
    radio->enableDynamicPayloads();
    radio->setAutoAck(true);
    radio->powerUp();
    //radio->setAutoAck(epBroadcast.pipe, true);
    radio->printDetails();
    printf("\r\n=====================\r\n");
}
void CommandNetwork::loop(){
    bool shouldRun = false;
    unsigned long now = millis();
    unsigned int runDiff = diff(lastRun, now);
    if (runDiff >= runInterval || epDevice.id == 0) shouldRun = true;
    if (runDiff < maxLoopInterval) shouldRun = false;

    if (shouldRun){//run that shit!
        lastRun = now;
        start();
        while(diff(now, millis()) < receiveDuration){
            processInbound();
        }
        stop();
        if (epDevice.id == 0){
            broadcast(REQ_NETWORKID, &tempId, sizeof(tempId));
        }
        processOutbound();
    }
}

//---------- inbound ----------
void CommandNetwork::setReceiveHandler(void (*f)(byte, byte*, byte)){
    receiveHandler = *f;
}

//---------- outbound ----------
void CommandNetwork::broadcast(byte instruction, void* data, byte byteLength){
    CommandMessage *msg = new CommandMessage(instruction, data, byteLength, bufferSize);
    queueMessage(epBroadcast.id, msg);
    delete msg;
}