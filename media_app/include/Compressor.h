#pragma once

typedef unsigned char Byte;

/*
Basic zlib implementation. Uses default settings. 
In the future, should add functionality for changing settings
*/
class Compressor 
{
private:

public:
    Compressor();
    ~Compressor();

    int Compress(Byte *dest,   unsigned long *destLen, const Byte *source, unsigned long sourceLen);
    int Uncompress(Byte *dest,   unsigned long *destLen, const Byte *source, unsigned long sourceLen);

    unsigned long long CalculateCompressBound(unsigned long sourceLen);
};
