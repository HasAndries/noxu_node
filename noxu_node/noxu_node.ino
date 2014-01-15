#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

const struct { 
    int networkId;
} eepromLocation = { 0 };

//---------- instructions ----------
typedef enum {
    REQ_COMMAND = 0, RES_COMMAND = 100,
    REQ_NETWORKID = 1, RES_NETWORKID = 101
} instructions;
//---------- endpoint ----------
typedef struct{
    byte channel;
    byte id;
    uint64_t pipe;
} endpoint;
//---------- message ----------
typedef struct{
    byte *buffer;
    byte control;
    bool fromCommander;
    instructions instruction;
    byte *data;
	byte hopCount;
    byte *hops;
    byte lastHop;
} message;
//---------- settings ----------
const uint64_t basePipe = 0xF0F0F0F000LL;
const endpoint epBroadcast = { 0x00, 0x00, 0xF0F0F0F000LL };
const unsigned int runInterval = 1000;
const unsigned int receiveDuration = 1000;

class RFNode{
private:
	//---------- properties ----------
    RF24 *radio;
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
        (*radio).openReadingPipe(1, epBroadcast.pipe);
        if (hasCommander)
            (*radio).openReadingPipe(2, epCommander.pipe);
        (*radio).startListening();
        listening = true;
    }
    //---------- stopListen ----------
    void stopListen(){
        (*radio).stopListening();
        listening = false;
    }
    //---------- receive ----------
    void receive(){
        uint8_t pipe;
        if ((*radio).available(&pipe)){
            byte buffer[16];
            if ((*radio).read(&buffer, 16)) {
				message msg = buildMessage(buffer);
                if (epBroadcast.pipe == pipe) receiveBroadcast(msg);
                else if (hasCommander && epCommander.pipe == pipe) receiveCommander(msg);
            }
        }
    }
    //---------- receiveBroadcast ----------
    void receiveBroadcast(message msg) {

    }
    //---------- receiveCommander ----------
	void receiveCommander(message msg){
        if (msg.fromCommander && msg.lastHop == epDevice.id){//message is for this node
            process(msg);
        }
        else if (msg.fromCommander && msg.hopCount > 0){//message is routed back through this node
            //remove last hop and forward message to next hop
            msg.hopCount--;
            byte lastHop = ((byte*)msg.hops)[msg.hopCount-1];
            sendMessage(lastHop, msg);
        }
        else if (msg.fromCommander){//big problem, maybe send out notification?
        }
        else if (!msg.fromCommander){//forward message onto commander
			//add current node id and send to commander
            addMessageHop(msg, epDevice.id);
			sendMessage(epCommander.id, msg);
        }
    }
    //---------- dissectMessage ----------
    message buildMessage(byte *buffer){
		message msg;
		msg.buffer = buffer;
        //header
        msg.control = buffer[0];
        msg.fromCommander = isBitSet(msg.control, 0);
        msg.instruction = (instructions)buffer[1];
        //data
        byte *dataPtr = &buffer[0] + 2;
        byte dataLength = *dataPtr;
        void *data = malloc(sizeof(byte) * dataLength);
        msg.data = (byte*)dataPtr+1;
        //hops
        byte *hopPtr = dataPtr + 1 + sizeof(byte) * dataLength;
        msg.hopCount = *hopPtr;
        void *hops = malloc(sizeof(byte) * msg.hopCount);
        msg.hops = (byte*)hopPtr + 1;
        msg.lastHop = ((byte*)hops)[msg.hopCount-1];
		return msg;
    }
	//---------- rebuildBuffer ----------
	void rebuildBuffer(message msg){
		msg.buffer[0] = setBit(msg.buffer[0], 0, msg.fromCommander);//fromCommander
		msg.buffer[1] = msg.instruction;
		//data
		msg.buffer[2] = sizeof(msg.data);
		for(byte ct=0;ct<msg.buffer[2];ct++)
			msg.buffer[2+ct] = msg.data[ct];
		//hops
		byte hopIndex = 3+msg.buffer[2];
		msg.buffer[hopIndex] = msg.hopCount;
		for(byte ct=0;ct<msg.hopCount;ct++)
			msg.buffer[hopIndex+1+ct] = msg.hops[ct];
	}
	void addMessageHop(message msg, byte id){
		msg.hopCount++;
		realloc(msg.hops, sizeof(byte) * msg.hopCount);
		msg.hops[msg.hopCount-1] = id;
	}
    //---------- process ----------
    void process(message msg){
        switch (msg.instruction)
        {
        case RES_COMMAND:
            if (!hasCommander){
                epCommander.id = msg.data[0];
                epCommander.pipe = basePipe+msg.data[0];
            }
            break;
        case REQ_COMMAND:
            if (hasCommander){
                //todo respond with commander details
            }
        default:
            break;
        }
        (*receiveHandler)(msg.instruction, msg.data);
    }
    //---------- sendBuffer ----------
	void sendBuffer(uint64_t pipe, byte buffer[]) {
        bool wasListening = listening;
        if (listening)
            stopListen();

        //todo: set channel
        (*radio).openWritingPipe(pipe);
        (*radio).write(buffer, sizeof(buffer));

        if (wasListening)
            startListen();
    }
    //---------- sendMessage ----------
    void* sendMessage(byte dest, message msg){
		rebuildBuffer(msg);
		sendBuffer(basePipe+dest, msg.buffer);
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
	RFNode(){
		
	}
    //---------- setup ----------
    void setup(){
        listening = false;
        discovering = false;
        hasCommander = false;

        radio = &RF24(9, 10);
        (*radio).begin();
        (*radio).setRetries(15,15);
        (*radio).setPayloadSize(16);
        (*radio).printDetails();
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
    void send(byte instruction, void* data){
		byte buffer[16];
		message msg = buildMessage(buffer);
		msg.instruction = (instructions)instruction;
		msg.data = (byte*)data;
		addMessageHop(msg, epDevice.id);
		sendMessage(epCommander.id, msg);
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
