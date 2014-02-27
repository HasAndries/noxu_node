#include "commandNetwork.h"
#include "trueRandom.h"

//========== PRIVATE ==========
//---------- inbound ----------
void CommandNetwork::start(){
    if (listening) return;
    //printf("<<LISTEN START\n");
    //todo: set channel
    radio->openReadingPipe(1, address);
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
    //msg->print("<<RECEIVE");
    bool shouldProcess = false;
    int deviceIndex = findIndex(msg->hops, msg->hopCount, networkId);
    //printf("((uint16_t*)msg->data)[0] = %d", ((uint16_t*)msg->data)[0]);

    if (msg->fromCommander && deviceIndex == msg->hopCount-1){//last hop is for this deviceId, message is for this node
        shouldProcess = true;
    }
    else if (msg->fromCommander && deviceIndex == -1 && msg->dataLength == sizeof(tempId) && ((uint16_t*)msg->data)[0]==tempId){//message is for this devices' tempId
        shouldProcess = true;
    }
    else if (msg->fromCommander && deviceIndex != -1 && msg->hopCount > 1){//message is routed back through this node, forward message to next hop
        queueMessage(msg);
    }
    else if (msg->fromCommander){//big problem, maybe send out notification?
    }
    else if (!msg->fromCommander){//forward message onto commander
        //add current node id and send to commander
        msg->addHop(networkId);
        sendMessage(msg);
    }

    if (shouldProcess){
        msg->print("<<RECEIVE");
        switch (msg->instruction)
        {
        case NETWORKID_NEW:
            networkId = msg->hops[msg->hopCount-1];
            printf(">>NETWORKID_NEW -> %d\r\n", networkId);
            msg->instruction = NETWORKID_CONFIRM;
            sendMessage(msg);
            break;
        case PING:
            printf(">>PING_REPLY -> ");
            printBytes(msg->data, msg->dataLength);
            msg->instruction = PING_REPLY;
            sendMessage(msg);
            break;
        default:
            break;
        }
        (*receiveHandler)(msg->instruction, msg->data, msg->dataLength);
    }
}

//---------- outbound ----------
void CommandNetwork::queueMessage(CommandMessage *msg){
    byte* buffer = msg->buildBuffer();
    //printf(">>QUEUEMESSAGE -> ");
    //printBytes(buffer, bufferSize);
    outboundQueueLength++;
    realloc(outboundQueue, outboundQueueLength * sizeof(byte));
    outboundQueue[outboundQueueLength-1] = buffer;
    processOutbound();
}

void CommandNetwork::processOutbound(){
    //broadcast
    uint64_t ct =0;
    while(ct < outboundQueueLength){
        sendBuffer(address, outboundQueue[ct]);
        free(outboundQueue[ct]);
        ct++;
    }
    outboundQueueLength = 0;
    realloc(outboundQueue, 0);
}
void CommandNetwork::sendMessage(CommandMessage *msg){
    byte* buffer = msg->buildBuffer();
    sendBuffer(address, buffer);
    free(buffer);
}
void CommandNetwork::sendBuffer(uint64_t pipe, byte buffer[]) {
    bool wasListening = listening;
    if (listening)
        stop();

    printf(">>OUTBOUND(");
    printLL(address, 16);
    printf(") -> ");
    printBytes(buffer, bufferSize);

    radio->openWritingPipe(pipe);
    radio->write(buffer, bufferSize);

    if (wasListening)
        start();
}

//---------- control ----------
void CommandNetwork::resetDeviceId(){
    networkId = 0;
    //tempId = random(10000, 65535);
    tempId = TrueRandom.random(10000, 65535);
}

//========== PUBLIC ==========
//---------- constructors ----------
CommandNetwork::CommandNetwork(uint64_t _address, uint8_t _channel, rf24_datarate_e _datarate, byte _bufferSize, unsigned int _maxLoopInterval, unsigned int _receiveDuration){
    address = _address;
    channel = _channel;
    datarate = _datarate;
    bufferSize = _bufferSize;
    maxLoopInterval = _maxLoopInterval;
    receiveDuration = _receiveDuration;
    outboundQueue = (byte**)malloc(0);
    outboundQueueLength = 0;
}

//---------- lifetime ----------
void CommandNetwork::setup(){
    printf("\r\n====== RFnode ======\r\n");
    listening = false;
    //randomSeed((analogRead(0)+analogRead(1))/2);
    resetDeviceId();
    printf("TempId: %ld\r\n", tempId);

    radio = new RF24(9, 10);
    radio->begin();
    radio->setPALevel(RF24_PA_MAX);
    radio->setChannel(channel);
    radio->setCRCLength(RF24_CRC_16);
    radio->setDataRate(datarate);
    radio->setRetries(0,0);
    radio->setPayloadSize(bufferSize);
    radio->enableDynamicPayloads();
    radio->setAutoAck(false);
    radio->powerUp();
    //radio->setAutoAck(epBroadcast.pipe, true);
    //radio->printDetails();
    printf("\r\n=====================\r\n");
}
void CommandNetwork::loop(){
    bool shouldRun = true;
    unsigned long now = millis();
    unsigned int runDiff = diff(lastRun, now);
    if (runDiff < maxLoopInterval) shouldRun = false;
    if (networkId == 0) shouldRun = true;
    if (shouldRun){//run that shit!
        lastRun = now;
        start();
        while(diff(now, millis()) < receiveDuration){
            processInbound();
        }
        stop();
        if (networkId == 0){
            send(NETWORKID_REQ, &tempId, sizeof(tempId));
        }
        //processOutbound();
    }
}

//---------- inbound ----------
void CommandNetwork::setReceiveHandler(void (*f)(byte, byte*, byte)){
    receiveHandler = *f;
}

//---------- outbound ----------
void CommandNetwork::send(byte instruction, void* data, byte byteLength){
    CommandMessage *msg = new CommandMessage(instruction, data, byteLength, bufferSize);
    if (networkId != 0)
        msg->addHop(networkId);
    sendMessage(msg);
    delete msg;
}