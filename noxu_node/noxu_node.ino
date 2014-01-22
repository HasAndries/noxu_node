#include <EEPROM.h>
#include <SPI.h>
#include "printf.h"
#include "common.h"
#include "commandNetwork.h"

//---------- settings ----------
const byte bufferSize = 32;//max 32
const unsigned int maxLoopInterval = 1000;
const unsigned int runInterval = 20000;
const unsigned int receiveDuration = 10000;


CommandNetwork *network;

void setup() {
    Serial.begin(57600);
    printf_begin();
    network = new CommandNetwork(bufferSize, maxLoopInterval, runInterval, receiveDuration);
    network->setup();
    network->setReceiveHandler(receive);
}

void loop() {
    network->loop();
}

void receive(byte type, byte* data){

}