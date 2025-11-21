// 2023/06/09 16:40:37 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Settings/SettingsTypes.h"


struct ProtectionBruteForce
{
    enum E
    {
        Enabled,
        Disabled
    };

    static void Set(E v)
    {
        current = v;
    }

    // При неподошедшем пароле вызываем эту функцию
    static void FailAttempt();

    // Пароль подошёл. Сбрасываем счётчик
    static void Reset()
    {
        count_fail = 0;
    }

    // Время ожидания до очередной попытки аутентификации
    static uint TimeWait();

private:

    static E current;
    static int count_fail;      // Количество неуспешных попыток
    static uint time_last_fail; // Время последний неудачной попытки
};


// Настройки мастер-карты.
// Они же - настройки считывателя
struct SettingsMaster
{
    Block4 s04;   // Для признака мастер-карты
    Block4 s05;   // Старый пароль. Должен совпадать с установленным сейчас
    Block4 s06;   // на считывателе. 0xFF, если карта меняет только пароль
    Block4 s07;   // Новый пароль. Будет установлен на считыватель
    Block4 s08;   // после отработки мастер-карты
    Block4 s09;   // WG[0], oсдп-режим, громкости[1], мелодии[2...3]
    Block4 s10;   // Байт на номер, первый блок номера
    Block4 s11;   // "Красный" цвет
    Block4 s12;   // "Зелёный" цвет
    Block4 s13;   // Время блокировки замка, время тревоги, адрес OSDP, baud rate OSDP, режим ЭКО
    Block4 s14;   // Датчик отрыва
    Block4 s15;   // 0 - only SL3, wiegand control bits enable, wiegand control bits parity
    Block4 s16;
    Block4 s17;
    Block4 s18;
    Block4 s19;
    Block4 s20;   // Контрольная сумма (всех предыдущих значений и битовой карты, если есть)

    // Подготавливает структуру для записи мастер-карты установки только пароля
    void     PrepareMasterOnlyPassword(uint64 new_password);

    // Возвращает размер в байтах
    int      Size() const { return sizeof(*this); }
    const Block16  OldPassword() const;                         // Это пароль считывателя, для которого предназначена карта. Его будем менять
    Block16  Password() const;                                  // Менять будем на этот
    void     SetPassword(const Block16 &);
    void     SetOldPassword(const Block16 &);
    void     EnableOSDP(bool);
    bool     IsEnabledOSDP() const;
    uint8    GetWiegandValue() const;
    void     SetWiegand(uint);                  // Сюда пишем значение из команды, в котором и размерность, и информациия о контрольных битах
    uint     GetWiegandFullSettings() const;
    void     SetOnlySL3(bool);
    bool     GetOnlySL3() const;
    uint8    Melody(TypeSound::E) const;
    void     SetMelody(TypeSound::E, uint8);
    uint8    Volume(TypeSound::E) const;
    void     SetVolume(TypeSound::E, uint8);
    // s10
    uint8    SizeNumber() const              { return s10.bytes[0]; }
    void     SetSizeNumber(uint8 s)          { s10.bytes[0] = s; }
    uint8    BeginNumber() const             { return s10.bytes[1]; }
    void     SetBeginNumber(uint8 b)         { s10.bytes[1] = b; }
    bool     IsSecurityModeEnabled() const { return (s10.bytes[2] & 2) != 0; }
    void     SetSecurityModeEnabled(bool en) { en ? (s10.bytes[2] |= 2) : (s10.bytes[2] &= ~2); }
    bool     IsOfflineModeAldowed() const { return (s10.bytes[2] & 1) != 0; }                       // Разрешено использование в автономном режиме (номера разрешённых
    void     SetOfflineModeAllowed(bool en) { en ? (s10.bytes[2] |= 1) : (s10.bytes[2] &= ~1); }    // карт хранятся в считывателе)
    // s11
    Color    ColorRed() const { return Color::FromUint(s11.u32); }
    void     SetColorRed(const Color &color) { s11.u32 = color.value; }
    // s12
    Color    ColorGreen() const                { return Color::FromUint(s12.u32); }
    void     SetColorGreen(const Color &color) { s12.u32 = color.value; }
    // s13
    uint     TimeLock() const                { return (uint)s13.bytes[0] * 1000; }
    void     SetTimeLock(uint8 t)            { s13.bytes[0] = t; }
    uint     TimeAlarm() const               { return (uint)s13.bytes[1] * 1000; }
    void     SetTimeAlarm(uint8 t)           { s13.bytes[1] = t; }
    uint8    AddressOSDP() const             { return s13.bytes[2]; }
    void     SetAddressOSDP(uint8 address)   { s13.bytes[2] = address; }
    BaudRate BaudRateOSDP() const;
    void     SetBaudRateOSDP(BaudRate::E b);
    bool     IsEnabledLPCD() const;
    void     SetEnabledLPCD(bool);
    // s14
    void     SetAntibreakSens(uint8);               // Установить чувствительность датчика отрыва
    uint8    GetAntibreakSensRAW();
    float    GetAntibreakSens();                    // Возвращает угол срабатывания датчика отклоанения
    bool     IsEnabledAntibreak();
    void     SetAntibreakNumber(uint);              // Установить номер карты, который будет передаваться по WG в случае срабатывания датчика отрыва
    uint     GetAntibreakNumber();
    // s15
    bool     IsWiegandControlBitsEnabled() const;           // Если false - контрольные биты не передаются
    bool     IsWiegandControlBitsParityStandard() const;    // Если true, то такое же, как для стандартного WG26, иначе - стартовый и конечный биты рассчитываются наоборот
    bool     IsWiegandFullGUID() const;                     // Если true, то всегда передаётся полный GUID (вернее, столько бит, сколько поместится)
    bool     IsWiegandReverseOrderBits() const;
    bool     IsWiegnadDiscard_NUID_LSB() const;
    void     SetNumberScriptLED(int8);                      // Если 15, то лампочки не переходят в скрипт
    int8     GetNumberScriptLED() const;                    // Возвращает номер скрипта, по которому играют светодиоды в простое. Если 15, то не играет
    bool     IsEnabledScriptLED() const;

    static const int FIRST_BLOCK_BITMAP_CARDS = 21;         // Номер первого блока с битовой картой разрешённых карт
    static const int NUMBER_BLOCKS_BITMAP_CARDS = 19;       // Количество блоков с битовой картой рарешённхы карт

    static void Load();

    static void Save();

    // Сброс настроек на заводские
    static void ResetToFactory();

    struct PSWD
    {
        // Возвращает текущий пароль
        static Block16 Get();

        // Установка нового пароля с мастер-карты
        static void Set(const Block16 &);

        static const Block16 default_Mifare;

        static Block16 GetFactory();
    };

    bool CRC32IsMatches();
    void CalculateAndWriteCRC32();

private:
    uint  CalculateCRC32() const;       // Рассчитывает CRC32 (новая методология)
    uint &CRC32() { return s20.u32; }  //-V524
};


extern SettingsMaster gset;
