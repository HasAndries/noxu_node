#include "commandMessage.h"

class CommandNetwork{
    private:
    //---------- properties ----------
    RF24 *radio;
	uint16_t tempId;
    unsigned long lastRun;
    bool listening;
    bool hasCommander;
    endpoint epCommander;
    endpoint epDevice;
    void (*receiveHandler)(byte, byte[]);

	byte** sendQueue;
	uint64_t sendQueueLength;
	byte** broadcastQueue;
	uint64_t** broadcastQueueLength;

	//---------- inbound ----------
    void startListen();
    void stopListen();
    void receive();
    void receiveBroadcast(CommandMessage msg);
    void receiveCommand(CommandMessage msg);
	
	//---------- outbound ----------
	byte* queueMessage(byte **queue, uint64_t queueLength, CommandMessage msg);
	byte* dequeueMessage(byte **queue, uint64_t queueLength);
    void sendBuffer(uint64_t pipe, byte buffer[]);
    void* sendMessage(byte dest, CommandMessage msg);

	//---------- other ----------
    void resetDeviceId();
protected:
public:
    CommandNetwork();
    void setup();
    void loop();

    void setReceiveHandler(void (*f)(byte, byte*));

    void broadcast(byte instruction, void* data, byte len);
    void send(byte instruction, void* data, byte len);
};