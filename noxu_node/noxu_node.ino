#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//---------- instructions ----------
typedef enum {
    REQ_COMMAND = 0, RES_COMMAND = 100,//broadcast
    REQ_NETWORKID = 1, RES_NETWORKID = 101
} instructions;
//---------- endpoint ----------
struct endpoint{
    byte channel;
    byte id;
    uint64_t pipe;
};
//---------- settings ----------
const uint64_t basePipe = 0xF0F0F0F000LL;
const endpoint epBroadcast = { 0x00, 0x00, 0xF0F0F0F000LL };
const unsigned int runInterval = 2000;
const unsigned int receiveDuration = 500;
const byte bufferSize = 32;


CommandNetwork network;

void setup() {
    Serial.begin(57600);
    printf_begin();
    network = CommandNetwork();
    network.setup();
    network.setReceiveHandler(receive);
}

void loop() {
    network.loop();
}

void receive(byte type, byte* data){

}