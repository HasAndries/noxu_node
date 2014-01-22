//commandMessage.h
#ifndef CommandMessage_H
#define CommandMessage_H

#include "base.h"
#include "common.h"

class CommandMessage{
private:
    //---------- properties ----------
    bool freed;
    void init();
protected:
public:
    //---------- properties ----------
    byte bufferSize;
    byte control;
    bool fromCommander;
    byte instruction;
    byte dataLength;
    byte *data;
    byte hopCount;
    byte *hops;
    byte lastHop;

	//---------- constructors ----------
    CommandMessage(byte instruction, void *_data, byte byteLength, byte _bufferSize);
    CommandMessage(byte *buffer, byte _bufferSize);
    ~CommandMessage();

	//---------- validation ----------
    bool validate();
	
	//---------- output ----------
    byte* buildBuffer();
	
	//---------- hops ----------
    void addHop(byte id);
    byte removeLastHop();

	//---------- print ----------
    void print(char *heading);
};
#endif