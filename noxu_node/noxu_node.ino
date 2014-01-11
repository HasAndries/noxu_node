#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

const struct { 
    int networkId;
} eepromLocation = { 0 };

const struct { 
    byte commandChannel;
    byte networkId;
} Request = { 0, 1 };

const struct { 
    byte commandChannel;
    byte networkId;
} Response = { 100, 101 };

struct  {

} Device = {};

struct {
    unsigned short device;
    unsigned short broadcast;
    unsigned short command;
} NetworkId = { 0 };

RF24 radio(9,10);
bool listening = false;

void setup() {
    Serial.begin(57600);
    radio.begin();
    radio.setRetries(15,15);
    radio.setPayloadSize(16);
    radio.printDetails();
}

void loop() {
    receive();
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

void startListen(){
    radio.openReadingPipe(1, NetworkId.broadcast);
    if (NetworkId.command != 0)
        radio.openReadingPipe(2, NetworkId.command);
    radio.startListening();
    listening = true;
}
void stopListen(){
    radio.stopListening();
    listening = false;
}
void receive() {
    if (radio.available())
    {
        byte message[16];
        if (radio.read(&message, 16))
        {
            //header
            byte control = message[0];
            bool fromCommander = isBitSet(control, 0);
            byte type = message[1];
            //data
            byte *dataPtr = &message[0] + 2;
            byte dataLength = *dataPtr;
            void *data = malloc(sizeof(byte) * dataLength);
            data = dataPtr+1;
            //hops
            byte *hopPtr = dataPtr + 1 + sizeof(byte) * dataLength;
            byte hopCount = *hopPtr;
            void *hops = malloc(sizeof(unsigned short) * hopCount);
            hops = hopPtr + 1;
            short lastHop = ((short*)hops)[hopCount-1];

            if (fromCommander && lastHop == NetworkId.device){//message is for this node
                process(message, type, (byte*)data);
            }
            else if (fromCommander && hopCount > 0){//message is routed through this node
                //remove last hop and forward message to next hop
                hopCount--;
                *hopPtr = hopCount;
                short lastHop = ((short*)hops)[hopCount-1];
                send(lastHop, message);
            }
            else if (fromCommander){//big problem, maybe send out notification?
            }
            else if (!fromCommander){//forward message onto commander
            }
        }
    }
}
void process(byte message[16], byte type, byte data[]){
    if (type == Response.commandChannel){
        if (NetworkId.command == 0)
            NetworkId.command = (unsigned short)*data;
    }
    if (type == Response.networkId){
        NetworkId.device = (unsigned short)*data;
    }
}
void* sendMessage(byte type, void* data){
    //construct message
    //header
    byte message[16];
    message[0] = setBit(message[0], 0, false);//fromCommander=false
    //data

    //hops

    //send new message
}
void send(unsigned short node, byte message[]) {
    bool wasListening = listening;
    stopListen();

    radio.openWritingPipe((unsigned long)node);
    radio.write(message, sizeof(message));

    if (wasListening)
        startListen();
}
bool isBitSet (byte b, int n) {
    static byte mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    return ((b & mask[n]) != 0);
}
byte setBit(byte b, byte n, bool on){
    static byte mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    if (on)
        return b |= mask[n];
    else
        return isBitSet(b, n) ? b ^= mask[n] : b;
}