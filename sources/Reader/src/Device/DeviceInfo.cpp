// 2023/12/18 16:19:58 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Device/Device.h"
#include "Hardware/HAL/HAL.h"
#include <ctime>

#ifdef WIN32
    #pragma warning(push)
    #pragma warning(disable:5264)
#endif


// Идентификатор программы.
// Заносится в исходный текст программы на этапе ее написания.
// Это число используется загрузчиком.Для СКЕ это значение равно EC010000h.
#ifdef TYPE_BOARD_777

static const uint PROGRAM_SIGNATURE __attribute__((used)) __attribute__((section(".ARM.__at_0x08001200"))) = 0xEC010000;        // Версия для никиты

#else

static const uint PROGRAM_SIGNATURE __attribute__((used)) __attribute__((section(".ARM.__at_0x08001200"))) = 0x55443300;

#endif

// Версия программы. 32х разрядное число. Заносится в
// исходный текст программы на этапе ее написания и сборки.
// Это число используется загрузчиком.
static const uint8  PROGRAM_VERSION_MAJOR __attribute__((used)) __attribute__((section(".ARM.__at_0x08001204"))) = VERSION_MAJOR;
static const uint8  PROGRAM_VERSION_MINOR __attribute__((used)) __attribute__((section(".ARM.__at_0x08001205"))) = VERSION_MINOR;
static const uint16 PROGRAM_VERSION_BUILD __attribute__((used)) __attribute__((section(".ARM.__at_0x08001206"))) = VERSION_BUILD;

// Длина программы в байтах. Длину определяет t4crypt исходя из характеристик hex файла и заносит по
// этому адресу перед кодированием.
static const uint PROGRAM_LENGTH __attribute__((used)) __attribute__((section(".ARM.__at_0x08001208"))) = 0xFFFFFFFF;

// Контрольная сумма программы.Рассчитывается приложением t4crypt по алгоритму CRC32 с полином
// EDB88320h и заносится по данному адресу при кодировании.Это значение будет использоваться загрузчиком
// для определения целостности основной программы перед каждым запуском.
static const uint PROGRAM_CRC __attribute__((used)) __attribute__((section(".ARM.__at_0x0800120C"))) = 0x12345678;

// Дата обновления программы. Дата и час(GMT), когда было произведено обновление программы.
// Выставляется загрузчиком на основании времени полученного от программы загрузки. 32х разрядное число.
// Последовательность байт прямая(little - endian).Старшие 8 бит это год считая от 2000. Биты с 16 по 23 месяц.
// Биты с 8 по 15 день месяца.Биты с 0 по 7 это час по гринвичу.
static const uint DATA_UPDATE __attribute__((used)) __attribute__((section(".ARM.__at_0x08001210"))) = 0xFFFFFFFF;

#define mkstr2(s) #s
#define mkstr(s) mkstr2(s)

#define stamp "RD3304 " mkstr(VERSION_BUILD)

static const uint PROGRAM_NAME __attribute__((used)) __attribute__((section(".ARM.__at_0x08001214"))) = (uint)stamp;


String<> Device::Info::Firmware::TextInfo()
{
    pchar info = (pchar)(PROGRAM_NAME);

    return String<>(info);
}


uint Device::Info::Hardware::Vendor()
{
//    uint result = PROGRAM_SIGNATURE;
//
//    result = 0x4B444F53;

    return 0x4B444F53;
}


uint8 Device::Info::Hardware::ManufacturedModelNumber()
{
    return 0x41;
}


uint8 Device::Info::Hardware::ManufacturedVersion()
{
    return 0x01;
}


String<> Device::Info::Hardware::Version_ASCII_HEX()
{
    uint16 *pointer = (uint16 *)(HAL_ROM::ADDRESS_BASE + 0x206); //-V566

    uint16 version = *pointer;

    return String<>("%02X.%02X", (uint8)(version >> 8), (uint8)(version));
}


String<> Device::Info::Hardware::DateManufactured()
{
    std::time_t secs = { *((uint *)(HAL_ROM::ADDRESS_BASE + 0x210)) }; //-V566

    std::tm time = *std::localtime(&secs);

    return String<>("%04d%02d%02d-%02d%02d%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
}


uint64 Device::Info::Hardware::MacUint64()
{
    struct StructMAC
    {
        uint8 b[6];
    };

    StructMAC *pointer = (StructMAC *)(HAL_ROM::ADDRESS_BASE + 0x214);

    StructMAC mac = *pointer;

    uint64 result = 0;

    int shift = 0;

    for (int i = 5; i >= 0; i--)
    {
        result |= (uint8)((int)mac.b[i] << shift);

        shift += 8;
    }

    return result;
}


String<> Device::Info::Hardware::SerialNumber_ASCII_HEX()
{
    return String<>("%08X", SerialNumber_UINT());
}


uint Device::Info::Hardware::SerialNumber_UINT()
{
    uint *address = (uint *)(HAL_ROM::ADDRESS_BASE + 0x200);        //-V566    \todo Избавиться от предупреждения

    return *address;
}


String<> Device::Info::Loader::Version_ASCII_HEX()
{
    uint16 *version = (uint16 *)(HAL_ROM::ADDRESS_BASE + 0x204);    //-V566    \todo Избавиться от предупреждения

    return String<>("%02X.%02X", (uint8)((*version) >> 8), (uint8)*version);
}


String<> Device::Info::Firmware::TypeID_ASCII_HEX()
{
    char buffer[64] = { 0 };

    uint8 *pointer = (uint8 *)"RD3304\x00\x00";

    for(int i = 0; i < 8; i++)
    {
        char symbol[3];

        std::sprintf(symbol, "%02X", pointer[i]);

        std::strcat(buffer, symbol);
    }

    return String<>(buffer);
}


uint16 Device::Info::Firmware::VersionBuild()
{
    uint16 *address = (uint16 *)(HAL_ROM::ADDRESS_FIRMWARE + 0x206);    //-V566    \todo Избавиться от предупреждения

    return *address;
}


String<> Device::Info::Firmware::VersionMajorMinor_ASCII()
{
    uint8 *major = (uint8 *)(HAL_ROM::ADDRESS_FIRMWARE + 0x204);        //-V566    \todo Избавиться от предупреждения
    uint8 *minor = (uint8 *)(HAL_ROM::ADDRESS_FIRMWARE + 0x205);        //-V566    \todo Избавиться от предупреждения

    return String<>("%02X.%02X", *major, *minor);
}


#ifdef WIN32
    #pragma warning(pop)
#endif
