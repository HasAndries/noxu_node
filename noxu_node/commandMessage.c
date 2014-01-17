#include "commandMessage.h"

CommandMessage::CommandMessage(){
}

CommandMessage::CommandMessage(byte *buffer){
    //header
    control = buffer[0];
    fromCommander = isBitSet(control, 0);
    instruction = buffer[1];
    //data
    dataLength = buffer[2];
    byte dataIndex = 3;
    data = (byte*)malloc(sizeof(byte) * dataLength);
    for(byte ct=0;ct<dataLength;ct++)
        data[ct] = buffer[dataIndex + ct];
    //hops
    hopCount = buffer[dataIndex+dataLength];
    byte hopIndex = dataIndex+dataLength+1;
    hops = (byte*)malloc(sizeof(byte) * hopCount);
    for(byte ct=0;ct<hopCount;ct++)
        hops[ct] = buffer[hopIndex + ct];
    lastHop = hops[hopCount-1];
}

CommandMessage::~CommandMessage(){
    if (freed)
        return;
    free(hops);
    free(data);
    freed = true;
}

bool CommandMessage::validate(){
    return dataLength+hopCount+4 < bufferSize && dataLength+hopCount > 0;
}

byte* CommandMessage::buildBuffer(){
    byte buffer[bufferSize];
    buffer[0] = setBit(buffer[0], 0, fromCommander);//fromCommander
    buffer[1] = instruction;
    //data
    buffer[2] = dataLength;
    for(byte ct=0;ct<dataLength;ct++)
        buffer[2+ct] = data[ct];
    //hops
    byte hopIndex = 3+buffer[2];
    buffer[hopIndex] = hopCount;
    for(byte ct=0;ct<hopCount;ct++)
        buffer[hopIndex+1+ct] = hops[ct];
    return buffer;
}

void CommandMessage::addHop(byte id){
    hopCount++;
    realloc(hops, sizeof(byte) * hopCount);
    hops[hopCount-1] = id;
}

byte CommandMessage::removeLastHop(){
    hopCount--;
    lastHop = ((byte*)hops)[hopCount-1];
    return lastHop;
}

void CommandMessage::print(char *heading){
    printf("===== %s(Message) =====\r\n", heading);
    printf("control:%d instruction:%d dataLength:%d hopCount:%d\r\n", msg.control, msg.instruction, msg.dataLength, msg.hopCount);

    printf("data:");
    printBytes(msg.data, msg.dataLength);

    printf("hops:");
    printBytes(msg.hops, msg.hopCount);

    printf("buffer:");
    printBytes(msg.buffer, bufferSize);

    printf("==========\r\n");
}