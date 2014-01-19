#include "commandNetwork.h"

//========== PRIVATE ==========
//---------- inbound ----------
void CommandNetwork::startListen(){
    if (listening) return;
	//printf("<<LISTEN START\n");
	//todo: set channel
	radio->openReadingPipe(1, epBroadcast.pipe);
	if (hasCommander)
		radio->openReadingPipe(2, epCommand.pipe);
	radio->startListening();
	listening = true;
}
void CommandNetwork::stopListen(){
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
				if (pipe == 0) receiveBroadcast(msg);
				else if (hasCommander && pipe == 1) receiveCommand(msg);
			}
			delete msg;
		}
        free(buffer);
	}
}
void CommandNetwork::receiveBroadcast(CommandMessage *msg) {
	msg->print("<<BROADCAST");
	switch (msg->instruction)
	{
	case REQ_COMMAND:
		if (hasCommander){
			//respond with this device as commander
			int tempId = msg->data[0];
			realloc(msg->data, sizeof(tempId) + sizeof(epDevice.id));
			msg->data[msg->dataLength] = epDevice.id;
			msg->dataLength += sizeof(epDevice.id);
			msg->instruction = RES_COMMAND;
            queueMessage(epBroadcast.id, msg);
		}
		break;
	case RES_COMMAND:
		if (!hasCommander){
			hasCommander = true;
			epCommand.id = msg->data[0];
			epCommand.pipe = basePipe+msg->data[0];
		}
		break;
	case RES_NETWORKID:
		epDevice.id = msg->data[0];
		break;
	default:
		break;
	}
	(*receiveHandler)(msg->instruction, msg->data);
}
void CommandNetwork::receiveCommand(CommandMessage *msg){
	msg->print("<<COMMAND");
	if (msg->fromCommander && msg->lastHop == epDevice.id){//message is for this node
		(*receiveHandler)(msg->instruction, msg->data);
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
        queueMessage(epCommand.id, msg);
	}
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
        printf(">>OUTBOUND(%x) -> ", basePipe + outboundQueuePipe[ct]);
	    printBytes(outboundQueue[ct], bufferSize);
        sendBuffer(outboundQueuePipe[ct], outboundQueue[ct]);
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
		stopListen();

	//todo: set channel
	radio->openWritingPipe(pipe);
    radio->write(buffer, bufferSize);

	if (wasListening)
		startListen();
}

//---------- control ----------
void CommandNetwork::resetDeviceId(){
	epDevice.id = 0;
	epDevice.pipe = 0;
	hasCommander = false;
	tempId = random(10000, 65535);
}
//========== PUBLIC ==========
//---------- build ----------
CommandNetwork::CommandNetwork(const byte _bufferSize, unsigned int _loopInterval, unsigned int _runInterval, unsigned int _receiveDuration){
    bufferSize = _bufferSize;
    loopInterval = _loopInterval;
    runInterval = _runInterval;
    receiveDuration = _receiveDuration;
	outboundQueue = (byte**)malloc(0);
    outboundQueuePipe = (byte*)malloc(0);
    outboundQueueLength = 0;
}
void CommandNetwork::setup(){
	printf("\r\n====== RFnode ======\r\n");
	listening = false;
	hasCommander = false;
	randomSeed(analogRead(0));
	resetDeviceId();

	radio = new RF24(9, 10);
	radio->begin();
    radio->setChannel(0);
    radio->setRetries(0,0);
	radio->setPayloadSize(bufferSize);
	radio->setAutoAck(true);
	radio->setAutoAck(epBroadcast.pipe, true);
	radio->printDetails();
	printf("\r\n=====================\r\n");
}

//---------- operations ----------
void CommandNetwork::loop(){
	bool shouldRun = false;
	unsigned long now = millis();
    unsigned int runDiff = diff(lastRun, now);
    if (runDiff >= runInterval || !hasCommander) shouldRun = true;
    if (runDiff < loopInterval) shouldRun = false;

	if (shouldRun){//run that shit!
		lastRun = now;
		startListen();
		while(diff(now, millis()) < receiveDuration){
			processInbound();
		}
		stopListen();
		if (!hasCommander){
			broadcast(REQ_COMMAND, &tempId, sizeof(tempId));
		}
        processOutbound();
	}
}
void CommandNetwork::setReceiveHandler(void (*f)(byte, byte*)){
	receiveHandler = *f;
}
void CommandNetwork::broadcast(byte instruction, void* data, byte byteLength){
    CommandMessage *msg = new CommandMessage(instruction, data, byteLength, bufferSize);
    queueMessage(epBroadcast.id, msg);
	delete msg;
}
void CommandNetwork::command(byte instruction, void* data, byte byteLength){
	CommandMessage *msg = new CommandMessage(instruction, data, byteLength, bufferSize);
    queueMessage(epCommand.id, msg);
	delete msg;
}