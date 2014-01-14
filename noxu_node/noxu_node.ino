#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

const struct { 
    int networkId;
} eepromLocation = { 0 };

typedef enum {
    REQ_COMMAND = 0, RES_COMMAND = 100,
    REQ_NETWORKID = 1, REQ_NETWORKID = 101
} instructions;
typedef struct{
    byte channel;
    byte id;
    unsigned long pipe;
} endpoint;
typedef struct{
    byte *buffer;
    byte control;
    bool fromCommander;
    instructions instruction;
    byte *data;
    byte *hops;
    byte lastHop;
} message;
const unsigned long basePipe = 0xF0F0F0F000;
const endpoint epBroadcast = { 0x00, 0x00, 0xF0F0F0F000 };
const unsigned int runInterval = 1000;
const unsigned int receiveDuration = 1000;

class RFNode{
private:
    RF24 radio;
    unsigned long lastRun;
    bool listening;
    bool discovering;
    bool hasCommander;
    endpoint epCommander;
    endpoint epDevice;
    void (*receiveHandler)(byte, byte[]);

    //---------- discover ----------
    void discover(){
    }
    //---------- getNetworkId ----------
    void getNetworkId(){
    }
    //---------- startListen ----------
    void startListen(){
        //todo: set channel
        radio.openReadingPipe(1, epBroadcast.pipe);
        if (hasCommander)
            radio.openReadingPipe(2, epCommander.pipe);
        radio.startListening();
        listening = true;
    }
    //---------- stopListen ----------
    void stopListen(){
        radio.stopListening();
        listening = false;
    }
    //---------- receive ----------
    void receive(){
        uint8_t pipe;
        if (radio.available(&pipe)){
            message msg;
            msg.buffer[16];
            if (radio.read(&msg.buffer, 16)) {
                if (epBroadcast.pipe == pipe) receiveBroadcast(msg.buffer);
                else if (hasCommander && epCommander.pipe == pipe) receiveCommander(msg.buffer);
            }
        }
    }
    //---------- receiveBroadcast ----------
    void receiveBroadcast(byte* buffer) {

    }
    //---------- receiveCommander ----------
    void receiveCommander(byte* buffer){
        bool fromCommander;
        instructions instruction;
        byte *data, *hops, lastHop;
        message msg;
        buildMessage(buffer, &msg);

        //header
        //byte control = message[0];
        //bool fromCommander = isBitSet(control, 0);
        //byte type = message[1];
        ////data
        //byte *dataPtr = &message[0] + 2;
        //byte dataLength = *dataPtr;
        //void *data = malloc(sizeof(byte) * dataLength);
        //data = dataPtr+1;
        ////hops
        //byte *hopPtr = dataPtr + 1 + sizeof(byte) * dataLength;
        //byte hopCount = *hopPtr;
        //void *hops = malloc(sizeof(byte) * hopCount);
        //hops = hopPtr + 1;
        //short lastHop = ((byte*)hops)[hopCount-1];

        if (fromCommander && lastHop == epDevice.id){//message is for this node
            process(buffer, instruction, (byte*)data);
        }
        else if (fromCommander && hopCount > 0){//message is routed through this node
            //remove last hop and forward message to next hop
            hopCount--;
            *hopPtr = hopCount;
            byte lastHop = ((byte*)hops)[hopCount-1];
            send(lastHop, buffer);
        }
        else if (fromCommander){//big problem, maybe send out notification?
        }
        else if (!fromCommander){//forward message onto commander
            byte *newHopPtr = hopPtr + 1 + sizeof(byte) * hopCount;
            hopCount++;
            *hopPtr = 1;
            *(newHopPtr) = *(&NetworkId.device);
        }
    }
    //---------- dissectMessage ----------
    void buildMessage(byte *buffer, message *msg){
        //header
        byte control = buffer[0];
        (*msg).fromCommander = isBitSet(control, 0);
        (*msg).instruction = (instructions)buffer[1];
        //data
        byte *dataPtr = &buffer[0] + 2;
        byte dataLength = *dataPtr;
        void *data = malloc(sizeof(byte) * dataLength);
        data = dataPtr+1;
        //hops
        byte *hopPtr = dataPtr + 1 + sizeof(byte) * dataLength;
        byte hopCount = *hopPtr;
        void *hops = malloc(sizeof(byte) * hopCount);
        hops = hopPtr + 1;
        *lastHop = ((byte*)hops)[hopCount-1];
    }
    //---------- process ----------
    void process(byte message[16], instructions instruction, byte data[]){
        switch (instruction)
        {
        case RES_COMMAND:
            if (!hasCommander){
                epCommander.id = data[0];
                epCommander.pipe = basePipe+data[0];
            }
            break;
        case REQ_COMMAND:
            if (hasCommander){
                //todo respond with commander details
            }
        default:
            break;
        }
        (*receiveHandler)(instruction, data);
    }
    //---------- send ----------
    void sendData(endpoint ep, byte message[]) {
        bool wasListening = listening;
        if (listening)
            stopListen();

        //todo: set channel
        radio.openWritingPipe((unsigned long)ep.pipe);
        radio.write(message, sizeof(message));

        if (wasListening)
            startListen();
    }
    //---------- sendMessage ----------
    void* sendMessage(byte type, void* data){
        //construct message
        //header
        byte message[16];
        message[0] = setBit(message[0], 0, false);//fromCommander=false

        //data
        byte dataSize = sizeof(data);
        message[1] = dataSize;
        for(byte ct=0;ct<dataSize;ct++){
            message[2+ct] = ((byte*)data)[ct];
        }

        //hops
        byte* hopPtr = &message[2+dataSize];
        *hopPtr = 1;
        *(hopPtr+1) = *(&NetworkId.device);
        *(hopPtr+2) = *(&NetworkId.device+1);

        //send new message
        send(NetworkId.command, message);
    }
    //---------- isBitSet ----------
    bool isBitSet (byte b, int n) {
        static byte mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
        return ((b & mask[n]) != 0);
    }
    //---------- setBit ----------
    byte setBit(byte b, byte n, bool on){
        static byte mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
        if (on)
            return b |= mask[n];
        else
            return isBitSet(b, n) ? b ^= mask[n] : b;
    }
protected:
public:
    //---------- setup ----------
    void setup(){
        listening = false;
        discovering = false;
        hasCommander = false;

        radio = RF24(9, 10);
        radio.begin();
        radio.setRetries(15,15);
        radio.setPayloadSize(16);
        radio.printDetails();
    }
    //---------- loop ----------
    void loop(){
        bool shouldRun = false;
        unsigned long now = millis();
        if (diff(lastRun, now) >= 1000)
            shouldRun = true;

        if (shouldRun || discovering){//run that shit!
            lastRun = now;
            if (!hasCommander && !discovering) discovering = true;
            startListen();
            while(diff(now, millis()) < receiveDuration){
                receive();
            }
            stopListen();
        }
    }
    unsigned long diff(unsigned long a, unsigned long b){
        if (a > b) return abs(a - b);
        else return abs(b - a);

    }
    //---------- setReceiveHandler ----------
    void setReceiveHandler(void (*f)(byte, byte*)){
        receiveHandler = *f;
    }
    //---------- send ----------
    void send(byte command, void* data){

    }
};

RFNode node;

void setup() {
    Serial.begin(57600);
    node = RFNode();
    node.setup();
    node.setReceiveHandler(receive);
}

void loop() {
    node.loop();
}

void receive(byte type, byte* data){

}

long readNetworkId() {
    byte networkId[2];
    networkId[0] = EEPROM.read(eepromLocation.networkId);
    networkId[1] = EEPROM.read(eepromLocation.networkId+1);
    return (long)networkId;
}

void writeNetworkId(unsigned short &networkId){
    EEPROM.write(0, (byte)networkId);
    EEPROM.write(1, (byte)networkId+1);
}
