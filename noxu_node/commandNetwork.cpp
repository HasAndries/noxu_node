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
            printf("valid message=%d", msg->validate());
			if (msg->validate()){
				if (pipe == 1) receiveBroadcast(msg);
				else if (hasCommander && pipe == 2) receiveCommand(msg);
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
            queueMessage(broadcastQueue, &broadcastQueueLength, msg);
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
		//sendMessage(lastHop, msg);
	}
	else if (msg->fromCommander){//big problem, maybe send out notification?
	}
	else if (!msg->fromCommander){//forward message onto commander
		//add current node id and send to commander
		msg->addHop(epDevice.id);
        queueMessage(commandQueue, &commandQueueLength, msg);
	}
}

//---------- outbound ----------
void CommandNetwork::queueMessage(byte **queue, uint64_t *queueLength, CommandMessage *msg){
	byte* buffer = msg->buildBuffer();
    //printf(">>QUEUEMESSAGE -> ");
    //printBytes(buffer, bufferSize);
	(*queueLength)++;
	realloc(queue, *queueLength * sizeof(byte));
	queue[(*queueLength)-1] = buffer;
}
void CommandNetwork::processOutbound(){
    //broadcast
    while(broadcastQueueLength > 0){
        broadcastQueueLength--;
        printf(">>BROADCAST(%x) -> ", epBroadcast.pipe);
	    printBytes(broadcastQueue[broadcastQueueLength], bufferSize);
        sendBuffer(epBroadcast.pipe, broadcastQueue[broadcastQueueLength]);
        free(broadcastQueue[broadcastQueueLength]);
    }
    realloc(broadcastQueue, 0);
    //command
    while(commandQueueLength > 0){
        commandQueueLength--;
        printf(">>COMMAND(%x) -> ", epCommand.pipe);
	    printBytes(commandQueue[commandQueueLength], bufferSize);
        sendBuffer(epCommand.pipe, commandQueue[commandQueueLength]);
        free(commandQueue[commandQueueLength]);
    }
    realloc(commandQueue, 0);
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
	commandQueue = (byte**)malloc(0);
    commandQueueLength = 0;
	broadcastQueue = (byte**)malloc(0);
    broadcastQueueLength = 0;
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
    radio->setRetries(10,10);
	radio->setPayloadSize(bufferSize);
	radio->setAutoAck(true);
	radio->setAutoAck(epBroadcast.pipe, false);
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
    queueMessage(broadcastQueue, &broadcastQueueLength, msg);
	delete msg;
}
void CommandNetwork::command(byte instruction, void* data, byte byteLength){
	CommandMessage *msg = new CommandMessage(instruction, data, byteLength, bufferSize);
    queueMessage(commandQueue, &commandQueueLength, msg);
	delete msg;
}