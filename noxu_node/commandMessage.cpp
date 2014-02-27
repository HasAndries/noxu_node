#include "commandMessage.h"

//========== PRIVATE ==========
//---------- constructors ----------
void CommandMessage::init(){
	control = 0;
	fromCommander = false;
	instruction = 0;
	dataLength = 0;
	hopCount = 0;
    hops = (byte*)malloc(0);
}

//========== PUBLIC ==========
//---------- constructors ----------
CommandMessage::CommandMessage(byte _instruction, void *_data, byte byteLength, byte _bufferSize){
	init();
	bufferSize = _bufferSize;
	instruction = _instruction;
	data = (byte*)malloc(sizeof(byte) * byteLength);
	memset(data, 0, byteLength);
	for(byte ct=0;ct<byteLength;ct++)
		data[ct] = ((byte*)_data)[ct];
	dataLength = byteLength;
}
CommandMessage::CommandMessage(byte* buffer, byte _bufferSize){
	init();
	bufferSize = _bufferSize;
	//header
	control = buffer[0];
	fromCommander = isBitSet(control, 0);
	instruction = buffer[1];

	dataLength = buffer[2];
	hopCount = buffer[3];

	byte dataStart = 4;
	byte hopStart = 4+dataLength;

	if (validate()){
		//data
		if (dataLength < bufferSize-4-hopCount && hopCount < bufferSize-4-dataLength){
			data = (byte*)malloc(sizeof(byte) * dataLength);
			for(byte ct=0;ct<dataLength;ct++)
				data[ct] = buffer[dataStart + ct];
		}
		//hops
		(byte*)realloc(hops, sizeof(byte) * hopCount);
		for(byte ct=0;ct<hopCount;ct++)
			hops[ct] = buffer[hopStart + ct];
	}
}
CommandMessage::~CommandMessage(){
	if (freed)
		return;
	free(hops);
	free(data);
	freed = true;
}

//---------- validation ----------
bool CommandMessage::validate(){
	return dataLength+hopCount+4 < bufferSize && dataLength+hopCount > 0;
}

//---------- output ----------
byte* CommandMessage::buildBuffer(){
	byte* buffer = (byte*)malloc(sizeof(byte) * bufferSize);
	memset(buffer, 0, bufferSize);
	buffer[0] = setBit(buffer[0], 0, fromCommander);//fromCommander
	buffer[1] = instruction;
	buffer[2] = dataLength;
	buffer[3] = hopCount;
	byte dataStart = 4;
	byte hopStart = 4+dataLength;
	//data
	for(byte ct=0;ct<dataLength;ct++)
		buffer[dataStart+ct] = data[ct];
	//hops
	for(byte ct=0;ct<hopCount;ct++)
		buffer[hopStart+ct] = hops[ct];
	return buffer;
}

//---------- hops ----------
void CommandMessage::addHop(byte id){
	hopCount++;
	realloc(hops, sizeof(byte) * hopCount);
	hops[hopCount-1] = id;
}
int CommandMessage::hopIndex(byte id){
    return findIndex(hops, hopCount, id);
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