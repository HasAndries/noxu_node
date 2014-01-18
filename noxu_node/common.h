//common.h
#ifndef common_H
#define common_H

#include "base.h"

//---------- bit operations ----------
bool isBitSet (byte b, int n);
byte setBit(byte b, byte n, bool on);
//---------- math ----------
unsigned long diff(unsigned long a, unsigned long b);
//---------- printing ----------
void printBytes(byte* ptr, int length);

#endif