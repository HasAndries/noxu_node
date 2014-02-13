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

class CommandNetwork{
private:
    //---------- properties ----------
    uint64_t address;
    uint8_t channel;
    rf24_datarate_e datarate;
    byte bufferSize;//max 32
    unsigned int maxLoopInterval;
    unsigned int receiveDuration;
    RF24 *radio;

    uint16_t tempId;
    uint16_t networkId;
    unsigned long lastRun;
    bool listening;
    void (*receiveHandler)(byte, byte[], byte);

    byte** outboundQueue;
    uint64_t outboundQueueLength;

    //---------- inbound ----------
    void start();
    void stop();
    void processInbound();
    void receive(CommandMessage *msg);

    //---------- outbound ----------
    void queueMessage(CommandMessage *msg);
    void processOutbound();
    void sendBuffer(uint64_t pipe, byte buffer[]);

    //---------- control ----------
    void resetDeviceId();
public:
	//---------- constructors ----------
    CommandNetwork(uint64_t address, uint8_t channel, rf24_datarate_e datarate, byte _bufferSize, unsigned int maxLoopInterval, unsigned int receiveDuration);

	//---------- lifetime ----------
    void setup();
    void loop();

	//---------- inbound ----------
    void setReceiveHandler(void (*f)(byte, byte*, byte));

	//---------- outbound ----------
    void send(byte instruction, void* data, byte byteLength);
};

//---------- instructions ----------
typedef enum {
    REQ_NETWORKID = 1, RES_NETWORKID = 101
} instructions;

#endif