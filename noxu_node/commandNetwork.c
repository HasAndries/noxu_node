#include "commandNetwork.h"
#include "common.h"

//---------- startListen ----------
void CommandNetwork::startListen(){
    //printf("<<START listen\n");
    //todo: set channel
    (*radio).openReadingPipe(1, epBroadcast.pipe);
    if (hasCommander)
        (*radio).openReadingPipe(2, epCommander.pipe);
    (*radio).startListening();
    listening = true;
}
//---------- stopListen ----------
void CommandNetwork::stopListen(){
    //printf("<<STOP listen\n");
    (*radio).stopListening();
    listening = false;
}
//---------- receive ----------
void CommandNetwork::receive(){
    uint8_t pipe;
    if ((*radio).available(&pipe)){
        byte buffer[bufferSize] = { 0 };
        if ((*radio).read(&buffer, bufferSize)) {
            //printf("<<REC -> ");
            //printBytes(buffer, bufferSize);
            CommandMessage msg(buffer);
            if (msg.validate()){
                if (epBroadcast.pipe == pipe) receiveBroadcast(msg);
                else if (hasCommander && epCommander.pipe == pipe) receiveCommander(msg);
            }
            msg.~CommandMessage();
        }
    }
}
//---------- receiveBroadcast ----------
void CommandNetwork::receiveBroadcast(CommandMessage msg) {
    msg.print("<<BROADCAST");
    switch (msg.instruction)
    {
    case REQ_COMMAND:
        if (hasCommander){
            //respond with this device as commander
            int tempId = msg.data[0];
            realloc(msg.data, sizeof(tempId) + sizeof(epDevice.id));
            msg.data[msg.dataLength] = epDevice.id;
            msg.dataLength += sizeof(epDevice.id);
            msg.instruction = RES_COMMAND;
            sendMessage(epBroadcast.id, msg);
        }
        break;
    case RES_COMMAND:
        if (!hasCommander){
            hasCommander = true;
            epCommander.id = msg.data[0];
            epCommander.pipe = basePipe+msg.data[0];
        }
        break;
    default:
        break;
    }
    (*receiveHandler)(msg.instruction, msg.data);
}
//---------- receiveCommander ----------
void CommandNetwork::receiveCommander(CommandMessage msg){
    msg.print("<<COMMAND");
    if (msg.fromCommander && msg.lastHop == epDevice.id){//message is for this node
        process(msg);
    }
    else if (msg.fromCommander && msg.hopCount > 0){//message is routed back through this node
        //remove last hop and forward message to next hop
        byte lastHop = msg.removeLastHop();
        sendMessage(lastHop, msg);
    }
    else if (msg.fromCommander){//big problem, maybe send out notification?
    }
    else if (!msg.fromCommander){//forward message onto commander
        //add current node id and send to commander
        msg.addHop(epDevice.id);
        sendMessage(epCommander.id, msg);
    }
}
//---------- process ----------
void CommandNetwork::process(CommandMessage msg){
    switch (msg.instruction)
    {
    case RES_NETWORKID:
        epDevice.id = msg.data[0];
        break;
    default:
        break;
    }
    (*receiveHandler)(msg.instruction, msg.data);
}
//---------- sendBuffer ----------
void CommandNetwork::sendBuffer(uint64_t pipe, byte buffer[]) {
    bool wasListening = listening;
    if (listening)
        stopListen();

    //todo: set channel
    (*radio).openWritingPipe(pipe);
    (*radio).write(buffer, sizeof(buffer));

    if (wasListening)
        startListen();
}
//---------- sendMessage ----------
void* CommandNetwork::sendMessage(byte dest, CommandMessage msg){
    byte* buffer = msg.buildBuffer();
    printf(">>SEND %d to id(%d) -> ", msg.instruction, dest);
    printBytes(buffer, bufferSize);
    sendBuffer(basePipe+dest, buffer);
}
//---------- resetDeviceId ----------
void CommandNetwork::resetDeviceId(){
    epDevice.id = 0;
    epDevice.pipe = 0;
    hasCommander = false;
    tempId = random(1000000000, 2147483647);
}
CommandNetwork::CommandNetwork(){

}
//---------- setup ----------
void CommandNetwork::setup(){
    printf("\r\n====== RFnode ======\r\n");
    listening = false;
    hasCommander = false;
    randomSeed(analogRead(0));
    resetDeviceId();

    radio = &RF24(9, 10);
    (*radio).begin();
    (*radio).setRetries(0,0);
    (*radio).setPayloadSize(bufferSize);
    (*radio).setAutoAck(true);
    (*radio).setAutoAck(epBroadcast.pipe, false);
    (*radio).printDetails();
    printf("\r\n=====================\r\n");
}
//---------- loop ----------
void CommandNetwork::loop(){
    bool shouldRun = false;
    unsigned long now = millis();
    if (diff(lastRun, now) >= 1000)
        shouldRun = true;

    if (shouldRun || !hasCommander){//run that shit!
        lastRun = now;
        startListen();
        while(diff(now, millis()) < receiveDuration){
            receive();
        }
        stopListen();
        if (!hasCommander){
            broadcast(REQ_COMMAND, &tempId, sizeof(tempId));
        }
    }
}
//---------- setReceiveHandler ----------
void CommandNetwork::setReceiveHandler(void (*f)(byte, byte*)){
    receiveHandler = *f;
}
//---------- broadcast ----------
void CommandNetwork::broadcast(byte instruction, void* data, byte len){
    byte buffer[bufferSize] = { 0 };
    CommandMessage msg(buffer);
    msg.instruction = instruction;
    realloc(msg.data, sizeof(byte) * len);
    for(byte ct=0;ct<len;ct++)
        msg.data[ct] = ((byte*)data)[ct];
    msg.dataLength = len;
    sendMessage(epBroadcast.id, msg);
    msg.~CommandMessage;
}
//---------- send ----------
void CommandNetwork::send(byte instruction, void* data, byte len){
    byte buffer[bufferSize] = { 0 };
    CommandMessage msg(buffer);
    msg.instruction = instruction;
    realloc(msg.data, sizeof(byte) * len);
    for(byte ct=0;ct<len;ct++)
        msg.data[ct] = ((byte*)data)[ct];
    msg.dataLength = len;
    msg.addHop(epDevice.id);
    sendMessage(epCommander.id, msg);
    msg.~CommandMessage;
}