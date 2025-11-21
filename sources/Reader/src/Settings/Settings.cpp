// 2023/06/09 16:40:23 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Settings/Settings.h"
#include "Hardware/HAL/HAL.h"
#include "Reader/RC663/RC663.h"
#include "Hardware/Timer.h"
#include "Modules/Indicator/Indicator.h"
#include "Utils/StringUtils.h"
#include "Utils/Math.h"
#include "Hardware/HAL/HAL.h"
#include <cstring>
#include <cmath>

ProtectionBruteForce::E ProtectionBruteForce::current = ProtectionBruteForce::Enabled;
int ProtectionBruteForce::count_fail = 0;
uint ProtectionBruteForce::time_last_fail = 0;
const Block16 SettingsMaster::PSWD::default_Mifare(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);


static const SettingsMaster factory_set =
{
    { 0xFFFFFFFF },     // s04
    { 0x00000000 },     // s05 Заводской
    { 0x00000000 },     // s06 пароль
    { 0x00000000 },     // s07 Новый
    { 0x00000000 },     // s08 пароль
    { 0x1A103F00 },     // s09 WG (1Ah:26), VV, ML, MH
    { 0x00000403 },     // s10
    { 0xFFFF0000 },     // s11
    { 0xFF00FF00 },     // s12
    { 0x00011e05 },     // s13
    { 0x00000000 },     // s14
    { 0x00000000 },     // s15
    { 0x00000000 },     // s16
    { 0x00000000 },     // s17
    { 0x00000000 },     // s18
    { 0x00000000 },     // s19
    { 0x00000000 }      // s20
};

SettingsMaster gset = factory_set;


void SettingsMaster::Load()
{
    HAL_ROM::Load(gset);
}


void SettingsMaster::Save()
{
    SettingsMaster settings;

    HAL_ROM::Load(settings);

    if (std::memcmp(&settings, &gset, (uint)settings.Size()) != 0)
    {
        gset.CalculateAndWriteCRC32();

        HAL_ROM::Save(gset);
    }
}


void SettingsMaster::PSWD::Set(const Block16 &password)
{
    gset.SetPassword(password);

    Save();
}


void SettingsMaster::ResetToFactory()
{
    gset = factory_set;

    Save();

    for (int i = 0; i < 3; i++)
    {
        Indicator::Blink(Color(0xFF00FF, 1.0f), Color(0, 0), 500, false);
    }
}


Color Color::FromUint(uint value)
{
    return Color(value & 0x00FFFFFF, (uint8)(value >> 24) / 255.0f);
}


//bool Color::operator==(const Color &rhs) const
//{
//    return std::memcmp(this, &rhs, sizeof(*this)) == 0;
//}


uint ProtectionBruteForce::TimeWait()
{
    if (TIME_MS - time_last_fail > 60000)
    {
        Reset();
    }

    const int d = 10;

    if (count_fail < d)
    {
        return 0;
    }

    int count = count_fail - d;

    uint time = 500;

    while (count >= 0)
    {
        time *= 2;
        count -= d;
    }

    return time;
}


void ProtectionBruteForce::FailAttempt()
{
    time_last_fail = TIME_MS;
    count_fail++;
}


BaudRate SettingsMaster::BaudRateOSDP() const
{
    uint8 baudrate = (uint8)(s13.bytes[3] & 0x7f);       // Потому что в старшем бите хранится информация о режиме ЭКО

    return BaudRate((BaudRate::E)baudrate);
}


void SettingsMaster::SetBaudRateOSDP(BaudRate::E b)
{
    s13.bytes[3] &= 0x80;       // В старшем бите хранится режим ЭКО
    s13.bytes[3] |= (uint8)b;
}


bool SettingsMaster::IsEnabledLPCD() const
{
    return (s13.bytes[3] & 0x80) != 0;
}


void SettingsMaster::SetEnabledLPCD(bool enabled)
{
    if (enabled)
    {
        s13.bytes[3] |= 0x80;
    }
    else
    {
        s13.bytes[3] &= 0x7F;
    }
}


uint BaudRate::ToRAW() const
{
    static const uint raws[Count] =
    {
        9600,
        19200,
        38400
    };

    return raws[value];
}


BaudRate::E BaudRate::FromUInt(uint raw)
{
    if (raw == 9600)
    {
        return _9600;
    }
    else if (raw == 19200)
    {
        return _19200;
    }
    else if (raw == 38400)
    {
        return _38400;
    }

    return BaudRate::_9600;
}


uint8 SettingsMaster::Melody(TypeSound::E type) const
{
    int shift = (int)type * 4;

    return (uint8)((s09.u16[1] >> shift) & 0x0F);
}


uint8 SettingsMaster::Volume(TypeSound::E type) const
{
    int shift = (int)type * 2;

    return (uint8)((s09.bytes[1] >> shift) & 0x03);
}


void SettingsMaster::SetMelody(TypeSound::E type, uint8 num)
{
    uint16 half_word = s09.u16[1];

    half_word &= ~(0x0F << (type * 4));

    half_word |= (num << (type * 4));

    s09.u16[1] = half_word;
}


void SettingsMaster::SetVolume(TypeSound::E type, uint8 num)
{
    uint8 byte = s09.bytes[1];

    byte &= ~(0x03 << (type * 2));

    byte |= (num << (type * 2));

    s09.bytes[1] = byte;
}


void SettingsMaster::SetPassword(const Block16 &password)
{
    s07.u32 = password.u32[0];
    s08.u32 = password.u32[1];
}


Block16 SettingsMaster::Password() const
{
    Block8 password;

    password.u32[0] = s07.u32;
    password.u32[1] = s08.u32;

    return Block16(0, password.u64);
}


void SettingsMaster::SetOldPassword(const Block16 &password)
{
    s05.u32 = password.u32[0];
    s06.u32 = password.u32[1];
}


const Block16 SettingsMaster::OldPassword() const
{
    Block8 password;

    password.u32[0] = s05.u32;
    password.u32[1] = s06.u32;

    return Block16(0, password.u64);
}


void SettingsMaster::PrepareMasterOnlyPassword(uint64 new_password)
{
    std::memset(this, 0xFF, (uint)Size());

    SetPassword(Block16(0, new_password));

    CRC32() = CalculateCRC32();
}


uint SettingsMaster::CalculateCRC32() const
{
    const uint8 *begin = (const uint8 *)this;
    const uint8 *end = (const uint8 *)&s20;
    int size = end - begin;

    return Math::CalculateCRC32(begin, size);
}


Block16 SettingsMaster::PSWD::Get()
{
    return gset.Password();
}


Block16 SettingsMaster::PSWD::GetFactory()
{
    return factory_set.OldPassword();
}


uint8 SettingsMaster::GetWiegandValue() const
{
    uint8 raw = (uint8)(s09.bytes[0] & 0x7F);

    if (raw < 7)                    // \todo Старая система - где тип вейганд выбирался из 5 возможных
    {                               // Удалить при глобальном обновлении
        struct Wiegand
        {
            enum E
            {
                _26,
                _33,
                _34,
                _37,
                _40,
                _42,
                Count
            };

            Wiegand(E v) : value(v)
            {
            }

            int ToRAW() const
            {
                static const int raws[Count] =
                {
                    26,
                    33,
                    34,
                    37,
                    40,
                    42
                };

                return raws[value];
            }

            E value;
        };

        Wiegand weig((Wiegand::E)raw);

        return (uint8)weig.ToRAW();
    }

    return raw;
}


#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4310)
#endif


void SettingsMaster::SetWiegand(uint value)
{
    s09.bytes[0] &= 0x80;                           // Очищаем младшие семь бит

    s09.bytes[0] |= (uint8)(value & 0x7F);          // Записываем младшие семь бит

    //                         65432
    s15.bytes[0] &= BINARY_U8(10000011);            // Очишаем биты настроек Wiegand

    if (_GET_BIT(value, 9))  _SET_BIT(s15.bytes[0],6);   // Если контрольные биты включены, то устанавливаем бит
    if (_GET_BIT(value, 10)) _SET_BIT(s15.bytes[0],5);   // Если инверсный расчёт котрольных бит
    if (_GET_BIT(value, 8))  _SET_BIT(s15.bytes[0],4);   // Если передём полный GUID ВСЕГДА (точнее, сколько поместится)
    if (_GET_BIT(value, 11)) _SET_BIT(s15.bytes[0],3);   // Обратный порядок передачи бит - от младшего к старшему
    if (_GET_BIT(value, 12)) _SET_BIT(s15.bytes[0],2);   // Отбрасывать старший байт от 4-байтного UID
}


uint SettingsMaster::GetWiegandFullSettings() const
{
    uint value = (uint)(s09.bytes[0] & 0x7F);

    const uint8 byte = s15.bytes[0];

    if (_GET_BIT(byte, 6)) _SET_BIT(value, 9);
    if (_GET_BIT(byte, 5)) _SET_BIT(value, 10);
    if (_GET_BIT(byte, 4)) _SET_BIT(value, 8);
    if (_GET_BIT(byte, 3)) _SET_BIT(value, 11);
    if (_GET_BIT(byte, 2)) _SET_BIT(value, 12);

    return value;
}


#ifdef WIN32
#pragma warning(pop)
#endif


bool SettingsMaster::IsWiegandControlBitsEnabled() const
{
    return (s15.bytes[0] & (1 << 6)) != 0;
}


bool SettingsMaster::IsWiegandControlBitsParityStandard() const
{
    return (s15.bytes[0] & (1 << 5)) == 0;
}


bool SettingsMaster::IsWiegandFullGUID() const
{
    return (s15.bytes[0] & (1 << 4)) != 0;
}


bool SettingsMaster::IsWiegandReverseOrderBits() const
{
    return (s15.bytes[0] & (1 << 3)) != 0;
}


bool SettingsMaster::IsWiegnadDiscard_NUID_LSB() const
{
    return (s15.bytes[0] & (1 << 2)) != 0;
}


void SettingsMaster::SetNumberScriptLED(int8 number)
{
    s15.bytes[0] &= 0x0F;
    s15.bytes[0] |= number << 4;
}


int8 SettingsMaster::GetNumberScriptLED() const
{
    return (s15.bytes[0] >> 4);
}


bool SettingsMaster::IsEnabledScriptLED() const
{
    return GetNumberScriptLED() != 15;
}


void SettingsMaster::SetOnlySL3(bool only)
{
    if (only)
    {
        s15.bytes[0] |= 0x80;
    }
    else
    {
        s15.bytes[0] &= 0x7F;
    }
}


bool SettingsMaster::GetOnlySL3() const
{
    return (s15.bytes[0] & 0x80) != 0;
}


void SettingsMaster::EnableOSDP(bool enable)
{
    if (enable)
    {
        s09.bytes[0] |= 0x80;
    }
    else
    {
        s09.bytes[0] &= 0x7F;
    }
}


bool SettingsMaster::IsEnabledOSDP() const
{
    return (s09.bytes[0] & 0x80) != 0;
}


void SettingsMaster::CalculateAndWriteCRC32()
{
    CRC32() = CalculateCRC32();
}


bool SettingsMaster::CRC32IsMatches()
{
    return CRC32() == CalculateCRC32();
}


void SettingsMaster::SetAntibreakSens(uint8 sens)
{
    s14.bytes[0] &= 0xF0;
    s14.bytes[0] |= sens;
}


uint8 SettingsMaster::GetAntibreakSensRAW()
{
    return (uint8)(s14.bytes[0] & 0x0F);
}


float SettingsMaster::GetAntibreakSens()
{
    uint8 sens = GetAntibreakSensRAW();

    if (sens == 0)
    {
        return 180.0f;
    }

    return (float)sens * 2.0f;
}


bool SettingsMaster::IsEnabledAntibreak()
{
    return GetAntibreakSensRAW() != 0;
}


void SettingsMaster::SetAntibreakNumber(uint number)
{
    std::memcpy(&s14.bytes[1], &number, 3);
}


uint SettingsMaster::GetAntibreakNumber()
{
    uint number = 0;

    std::memcpy(&number, &s14.bytes[1], 3);

    return number;
}
