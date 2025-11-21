// 2024/01/12 14:48:46 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Hardware/HAL/HAL.h"
#include "Reader/Reader.h"
#include "Utils/Math.h"
#include "Settings/SET2.h"
#include "Utils/Log.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>


namespace SET2
{
    static void Log(pchar name_point);

    namespace AccessCards
    {
        // Структура описывает место хранения бита доступа карты
        struct StructCard
        {
            StructCard(uint addr, uint _bit) : address((uint8 *)addr), bit((int)_bit) { }
            uint8 *address;                                                                 // Адрес байта, где хранится искомый бит
            int bit;                                                                        // Номер байта в бите
        };

        static StructCard FindPlaceForCard(uint64 num_card);

        static void ReadAccess(uint8[SIZE_BUFFER_ACCESS]);
    }

    namespace KeysMFP
    {
        // Возвращает ранее рассчитанную и записанную в ROM контрольную сумму ключей.
        // Она хранится сразу после ключей
        static uint GetCRC();

        // Рассчитывает контрольную сумму ключей
        static uint CalculateCRC(Block16 *keys);

        // Cчитывает ключи
        static void ReadKeys(Block16 *keys);

        // Возвращает адрес первого ключа
        static Block16 *AddressBegin();
    }

    // Записать доступА и ключи в сектор
    static void WriteAll(uint8 access[SIZE_BUFFER_ACCESS], const Block16 *keys);
}


void SET2::WriteAll(uint8 access[SIZE_BUFFER_ACCESS], const Block16 *keys)
{
    bool result = false;

    int counter = 0;

    while (!result && counter < 3)
    {
        HAL_ROM::ErasePage(HAL_ROM::ADDRESS_SECTOR_ACCESS_CARDS_AND_KEYSMFP);

        {
            // Записали доступы
            HAL_ROM::WriteBuffer(HAL_ROM::ADDRESS_SECTOR_ACCESS_CARDS_AND_KEYSMFP, access, SIZE_BUFFER_ACCESS);
        }

        {
            uint crc32_in = Math::CalculateCRC32(keys, (int)sizeof(Block16) * KeysMFP::NUM_KEYS);                           // Считаем контрольную сумму исходных значений

            // Теперь записываем ключи
            HAL_ROM::WriteBuffer((uint)KeysMFP::AddressBegin(), (void *)keys, (int)sizeof(Block16) * KeysMFP::NUM_KEYS);

            uint crc32_wr = Math::CalculateCRC32(KeysMFP::AddressBegin(), (int)sizeof(Block16) * KeysMFP::NUM_KEYS);        // Считаем контрольную сумму того, что записалось

            HAL_ROM::WriteBuffer((uint)(KeysMFP::AddressBegin() + 5), &crc32_wr, sizeof(crc32_wr));

            result = (crc32_in == KeysMFP::GetCRC());

            if (!result)
            {
                ModeReader::E mode = ModeReader::Current();

                ModeReader::Set(ModeReader::UART);

                LOG_ERROR("CRC32 does not match : %08X != %08X", crc32_in, crc32_wr);

                ModeReader::Set(mode);
            }
        }

        counter++;
    }
}


void SET2::KeysMFP::WriteToROM(const Block16 *keys)
{
    uint8 access[SIZE_BUFFER_ACCESS];

    AccessCards::ReadAccess(access);

    WriteAll(access, keys);
}


void SET2::AccessCards::WriteToROM(const char bits[SIZE_BUFFER_ACCESS])
{
    uint8 buffer[SIZE_BUFFER_ACCESS];

    std::memset(buffer, 0, SIZE_BUFFER_ACCESS);

    pchar pointer = bits;

    int index = 0;

    while (*pointer)
    {
        char data[32];

        char byte1 = *pointer++;
        char byte2 = *pointer++;

        std::sprintf(data, "%c%c", byte1, byte2);

        buffer[index++] = (uint8)std::strtoull(data, nullptr, 16);
    }

    Block16 keys[KeysMFP::NUM_KEYS] =
    {
        Block16(0, 0),
        Block16(0, 0),
        Block16(0, 0),
        Block16(0, 0),
        Block16(0, 0)
    };

    KeysMFP::ReadKeys(keys);

    WriteAll(buffer, keys);
}


bool SET2::AccessCards::Granted(uint64 number)
{
    if (number > ModeOffline::MaxNumCards())
    {
        return false;
    }

    StructCard card = FindPlaceForCard(number);

    uint8 byte = HAL_ROM::ReadUInt8((uint)card.address);

    return (byte & (1 << card.bit)) != 0;
}


SET2::AccessCards::StructCard SET2::AccessCards::FindPlaceForCard(uint64 num_card)
{
    uint index = (uint)(num_card / 8);

    uint bit = (uint)num_card - index * 8;

    return StructCard(HAL_ROM::ADDRESS_SECTOR_ACCESS_CARDS_AND_KEYSMFP + index, bit);
}


void SET2::KeysMFP::Init()
{
    if (GetCRC() != CalculateCRC(AddressBegin()) ||     // Если контрольная сумма ключей не соответствует
        GetCRC() == 0x0 ||                              // Или контрольная сумма забита нулями из-за записи ключей доступа
        GetCRC() == 0xFFFFFFFF)                         // Или в памяти ещё ничего нет
    {
        Reset();
    }
}


Block16 *SET2::KeysMFP::Get(int num)
{
    Block16 *key = AddressBegin() + num;

    char buffer[33];

    LOG_WRITE("key %d = %08X:%s", key, key->ToRawASCII(buffer));

    return key;
}


Block16 *SET2::KeysMFP::AddressBegin()
{
    // Нулвеой ключ находится в середине сектора

    return (Block16 *)(HAL_ROM::ADDRESS_SECTOR_ACCESS_CARDS_AND_KEYSMFP + SIZE_BUFFER_ACCESS);
}


uint SET2::KeysMFP::GetCRC()
{
    uint *address = (uint *)(AddressBegin() + 5);

    return *address;
}


uint SET2::KeysMFP::CalculateCRC(Block16 *keys)
{
    return Math::CalculateCRC32(keys, (int)sizeof(Block16) * KeysMFP::NUM_KEYS);
}


void SET2::KeysMFP::ReadKeys(Block16 *keys)
{
    std::memcpy(keys, AddressBegin(), sizeof(*keys) * KeysMFP::NUM_KEYS);
}


void SET2::AccessCards::ReadAccess(uint8 access[SIZE_BUFFER_ACCESS])
{
    uint *address = (uint *)HAL_ROM::ADDRESS_SECTOR_ACCESS_CARDS_AND_KEYSMFP; //-V566

    std::memcpy(access, address, SIZE_BUFFER_ACCESS);
}


void SET2::Test()
{
    for (int counter = 0; counter < 3; counter++)
    {
        Log("Before:");

        char buffer[SIZE_BUFFER_ACCESS] = { '\0' };

        for (int num_byte = 0; num_byte < 16; num_byte++)
        {
            char byte[3] = { '\0', '\0', '\0' };

            std::sprintf(byte, "%02X", (uint8)std::rand());

            std::strcat(buffer, byte);
        }

        AccessCards::WriteToROM(buffer);

        Log("Between:");

        Block16 keys[KeysMFP::NUM_KEYS] =
        {
            Block16(0, 0),
            Block16(0, 0),
            Block16(0, 0),
            Block16(0, 0),
            Block16(0, 0)
        };

        for (int i = 0; i < KeysMFP::NUM_KEYS; i++)
        {
            for (int num_byte = 0; num_byte < 16; num_byte++)
            {
                keys[i].bytes[num_byte] = (uint8)std::rand();
            }
        }

        KeysMFP::WriteToROM(keys);

        Log("After:");
    }

    AccessCards::Clear();

    KeysMFP::Reset();

    Log("After reset:");
}


void SET2::Log(pchar name_point)
{
    LOG_WRITE(" ");
    LOG_WRITE(name_point);

    uint8 bytes[16];

    for (int i = 0; i < 16; i++)
    {
        AccessCards::StructCard card = AccessCards::FindPlaceForCard((uint64)(i * 8));

        bytes[i] = HAL_ROM::ReadUInt8((uint)card.address);
    }

    Log::Log16Bytes("access", bytes);

    Log::Log16Bytes("rom", (uint8 *)HAL_ROM::ADDRESS_SECTOR_ACCESS_CARDS_AND_KEYSMFP); //-V566

    char buffer[36];

    for (int i = 0; i < KeysMFP::NUM_KEYS; i++)
    {
        LOG_WRITE(KeysMFP::Get(i)->ToReadableASCII(buffer));
    }
}


void SET2::AccessCards::Clear()
{
    char buffer[SIZE_BUFFER_ACCESS];

    for (int i = 0; i < SIZE_BUFFER_ACCESS; i++)
    {
        buffer[i] = 0;
    }

    WriteToROM(buffer);
}


void SET2::KeysMFP::Reset()
{
    Block16 keys[NUM_KEYS] =
    {
        Block16{0x0101010101010101, 0x0101010101010101},
        Block16{0x1111111111111111, 0x1111111111111111},
        Block16{0x2222222222222222, 0x2222222222222222},
        Block16{0x3333333333333333, 0x3333333333333333},
        Block16{0x4444444444444444, 0x4444444444444444}
    };

    WriteToROM(keys);
}
