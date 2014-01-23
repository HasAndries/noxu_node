#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
//Board = Arduino Uno
#define __AVR_ATmega328P__
#define 
#define _VMDEBUG 1
#define ARDUINO 105
#define ARDUINO_MAIN
#define __AVR__
#define F_CPU 16000000L
#define __cplusplus
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define __attribute__(x)
#define NOINLINE __attribute__((noinline))
#define prog_void
#define PGM_VOID_P int
            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}

//
//
void receive(byte type, byte* data);

#include "C:\Program Files (x86)\Arduino\hardware\arduino\variants\standard\pins_arduino.h" 
#include "C:\Program Files (x86)\Arduino\hardware\arduino\cores\arduino\arduino.h"
#include "C:\projects\noxu_node\noxu_node\noxu_node.ino"
#include "C:\projects\noxu_node\noxu_node\RF24.cpp"
#include "C:\projects\noxu_node\noxu_node\RF24.h"
#include "C:\projects\noxu_node\noxu_node\RF24_config.h"
#include "C:\projects\noxu_node\noxu_node\base.h"
#include "C:\projects\noxu_node\noxu_node\commandMessage.cpp"
#include "C:\projects\noxu_node\noxu_node\commandMessage.h"
#include "C:\projects\noxu_node\noxu_node\commandNetwork.cpp"
#include "C:\projects\noxu_node\noxu_node\commandNetwork.h"
#include "C:\projects\noxu_node\noxu_node\common.cpp"
#include "C:\projects\noxu_node\noxu_node\common.h"
#include "C:\projects\noxu_node\noxu_node\nRF24L01.h"
#include "C:\projects\noxu_node\noxu_node\printf.cpp"
#include "C:\projects\noxu_node\noxu_node\printf.h"
#endif
