#include "common.h"

//---------- bit operations ----------
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
//---------- math ----------
unsigned long diff(unsigned long a, unsigned long b){
    if (a > b) return abs(a - b);
    else return abs(b - a);
}
//---------- printing ----------
void printBytes(byte* ptr, int length){
    for(int ct=0;ct<length;ct++){
        printf("%d", ptr[ct]);
    }
    printf("\r\n");
}
