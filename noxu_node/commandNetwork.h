#include "commandMessage.h"

class CommandNetwork{
    private:
    //---------- properties ----------
    RF24 *radio;
    int tempId;
    unsigned long lastRun;
    bool listening;
    bool hasCommander;
    endpoint epCommander;
    endpoint epDevice;
    void (*receiveHandler)(byte, byte[]);

    //---------- startListen ----------
    void startListen();
    //---------- stopListen ----------
    void stopListen();
    //---------- receive ----------
    void receive();
    //---------- receiveBroadcast ----------
    void receiveBroadcast(CommandMessage msg);
    //---------- receiveCommander ----------
    void receiveCommander(CommandMessage msg);
    //---------- process ----------
    void process(CommandMessage msg);
    //---------- sendBuffer ----------
    void sendBuffer(uint64_t pipe, byte buffer[]);
    //---------- sendMessage ----------
    void* sendMessage(byte dest, CommandMessage msg);
    //---------- setBit ----------
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