// 2024/12/24 14:56:23 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


typedef unsigned char      uint8;
typedef signed char        int8;
typedef unsigned short     uint16;
typedef signed short       int16;
typedef unsigned int       uint;
typedef const char *pchar;
typedef unsigned char      uchar;
typedef unsigned long long uint64;
typedef signed long long   int64;


union Block2
{
    uint16 u16 = 0x0000;
    uint8  u8[2];

    explicit Block2(uint16 value) : u16(value)
    {
    }

    explicit Block2(uint8 low, uint8 hi)
    {
        u8[0] = low;
        u8[1] = hi;
    }
};  


union Block4
{
    uint   u32 = 0x00000000;
    uint16 u16[2];
    uint8  bytes[4];

    void Fill(uint8 in[4])
    {
        std::memcpy(bytes, in, 4);
    }
};


union Block8
{
    uint64 u64 = 0x0000000000000000;
    uint   u32[2];
    uint8  bytes[8];
};


union Block16
{
    uint64   u64[2];
    uint     u32[4];
    uint8    bytes[16];

    explicit Block16(uint64 hi, uint64 lo)     // \warn здесь пишетс€ сначала старшие байты, потом младшие
    {
        Set(hi, lo);
    }

    explicit Block16(uint8 in[16])
    {
        std::memcpy(bytes, in, 16);
    }

    bool operator==(const Block16 &rhs) const
    {
        return std::memcmp(bytes, rhs.bytes, 16) == 0;
    }

    void FillBytes(uint8[16]);

    void Set(uint64 hi, uint64 lo)
    {
        u64[0] = lo;
        u64[1] = hi;
    }

    void FillPseudoRandom();

    // ¬ыводит в "читаемом" виде - с точками между байтами
    char *ToReadableASCII(char out[36]) const;

    char *ToRawASCII(char out[33]) const;
};
