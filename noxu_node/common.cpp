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
    for(int ct=0;ct<length-1;ct++){
        printf("%d,", ptr[ct]);
    }
    printf("%d\r\n", ptr[length-1]);
}
//---------- printLL ----------
void printLL(uint64_t n, uint8_t base)
{
    unsigned char buf[16 * sizeof(long)]; 
    unsigned int i = 0;

    if (n == 0) 
    {
        Serial.write((char)'0');
        return;
    }

    while (n > 0) 
    {
        buf[i++] = n % base;
        n /= base;
    }

    for (; i > 0; i--)
        Serial.write((char) (buf[i - 1] < 10 ?
        '0' + buf[i - 1] :
    'A' + buf[i - 1] - 10));
}
int findIndex(byte arr[], byte length, byte searchVal)
{
    byte i;
    for (i=0; i<length; i++)
        if (arr[i] == searchVal) return(i);
    return(-1);
}
uint32_t get_seed(int pin)
{
    uint16_t aread;
    union {
        uint32_t as_uint32_t;
        uint8_t  as_uint8_t[4];
    } seed;
    uint8_t i, t;


    /* "aread" shifts 3 bits each time and the shuffle
    * moves bytes around in chunks of 8.  To ensure
    * every bit is combined with every other bit,
    * loop at least 3 x 8 = 24 times.  A few hundred
    * seems safe.
    */
    for (i = 0; i < 240; i++) {

        /* Shift three bits of A2D "noise" into aread.
        */
        aread <<= 3;
        aread |= analogRead(pin) & 0x7;

        /* Now shuffle the bytes of the seed
        * and xor our new set of bits onto the
        * the seed.
        */
        t = seed.as_uint8_t[0];
        seed.as_uint8_t[0] = seed.as_uint8_t[3];
        seed.as_uint8_t[3] = seed.as_uint8_t[1];
        seed.as_uint8_t[1] = seed.as_uint8_t[2];
        seed.as_uint8_t[2] = t;

        seed.as_uint32_t ^= aread;
    }

    return(seed.as_uint32_t);
}