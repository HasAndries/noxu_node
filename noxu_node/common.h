static bool isBitSet (byte b, int n) {
    static byte mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    return ((b & mask[n]) != 0);
}
//---------- setBit ----------
static byte setBit(byte b, byte n, bool on){
    static byte mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    if (on)
        return b |= mask[n];
    else
        return isBitSet(b, n) ? b ^= mask[n] : b;
}
unsigned long diff(unsigned long a, unsigned long b){
    if (a > b) return abs(a - b);
    else return abs(b - a);
}
void printBytes(byte* ptr, byte length){
    for(int ct=0;ct<length;ct++) printf("%d", ptr[ct]);
    printf("\r\n");
}