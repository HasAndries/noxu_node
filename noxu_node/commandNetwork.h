//commandNetwork.h
/*

Instruction types are at the bottom

*/
#ifndef CommandNetwork_H
#define CommandNetwork_H

#include "base.h"
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "common.h"
#include "commandMessage.h"

struct endpoint{
    byte channel;
    byte id;
    uint64_t pipe;
};
const uint64_t basePipe = 0xF0F0F0F0F0LL;
const endpoint epBroadcast = { 0x00, 0x00, 0xF0F0F0F0F0LL };

class CommandNetwork{
private:
    //---------- properties ----------
    byte bufferSize;//max 32
    unsigned int maxLoopInterval;
    unsigned int runInterval;
    unsigned int receiveDuration;
    RF24 *radio;

    uint16_t tempId;
    unsigned long lastRun;
    bool listening;
    bool hasCommander;
    endpoint epCommand;
    endpoint epDevice;
    void (*receiveHandler)(byte, byte[]);

    byte** outboundQueue;
    byte *outboundQueuePipe;
    uint64_t outboundQueueLength;

    //---------- inbound ----------
    void startListen();
    void stopListen();
    void processInbound();
    void receiveBroadcast(CommandMessage *msg);
    void receiveCommand(CommandMessage *msg);

    //---------- outbound ----------
    void queueMessage(byte pipe, CommandMessage *msg);
    void processOutbound();
    void sendBuffer(uint64_t pipe, byte buffer[]);

    //---------- control ----------
    void resetDeviceId();
public:
	//---------- constructors ----------
    CommandNetwork(byte _bufferSize, unsigned int maxLoopInterval, unsigned int runInterval, unsigned int receiveDuration);

	//---------- lifetime ----------
    void setup();
    void loop();

	//---------- inbound ----------
    void setReceiveHandler(void (*f)(byte, byte*));

	//---------- outbound ----------
    void broadcast(byte instruction, void* data, byte byteLength);
    void command(byte instruction, void* data, byte byteLength);
};

//---------- instructions ----------
typedef enum {
    REQ_COMMAND = 0, RES_COMMAND = 100,//broadcast
    REQ_NETWORKID = 1, RES_NETWORKID = 101//broadcast, command
} instructions;

#endif