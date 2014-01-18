#include "commandMessage.h"

//========== PRIVATE ==========
//========== PUBLIC ==========
//---------- constructor ----------
CommandMessage::CommandMessage(byte _instruction, void *_data, byte byteLength){
    printf("\r\ni:%d len:%d data:", _instruction, byteLength);
    printBytes((byte*)_data, byteLength);
    printf("inst: %d", instruction);
    instruction = _instruction;
    printf("inst: %d", instruction);
	realloc(data, sizeof(byte) * byteLength);
	for(byte ct=0;ct<byteLength;ct++)
		data[ct] = ((byte*)_data)[ct];
	dataLength = byteLength;

    printf("\r\ni:%d len:%d data:", instruction, dataLength);
    printBytes((byte*)data, dataLength);
}
CommandMessage::CommandMessage(byte* buffer, byte _bufferSize){
    bufferSize = _bufferSize;
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
//---------- destructor ----------
CommandMessage::~CommandMessage(){
    if (freed)
        return;
    free(hops);
    free(data);
    freed = true;
}
//---------- validate ----------
bool CommandMessage::validate(){
    return dataLength+hopCount+4 < bufferSize && dataLength+hopCount > 0;
}
//---------- buildBuffer ----------
byte* CommandMessage::buildBuffer(){
    byte* buffer = (byte*)malloc(sizeof(byte) * bufferSize);
    memset(buffer, 0, bufferSize);
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
//---------- addHop ----------
void CommandMessage::addHop(byte id){
    hopCount++;
    realloc(hops, sizeof(byte) * hopCount);
    hops[hopCount-1] = id;
}
//---------- removeLastHop ----------
byte CommandMessage::removeLastHop(){
    hopCount--;
    lastHop = ((byte*)hops)[hopCount-1];
    return lastHop;
}
//---------- print ----------
void CommandMessage::print(char *heading){
    printf("===== %s(Message) =====\r\n", heading);
    printf("control:%d instruction:%d dataLength:%d hopCount:%d\r\n", control, instruction, dataLength, hopCount);

    printf("data:");
    printBytes(data, dataLength);

    printf("hops:");
    printBytes(hops, hopCount);

    printf("buffer:");
    byte* buffer = buildBuffer();
    printBytes(buffer, bufferSize);
    free(buffer);

    printf("==========\r\n");
}