// 2024/12/06 12:18:36 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Utils/String.h"


struct UID
{
    UID()
    {
        Clear();
    }

    void Clear();

    // Расчитать UID исходя из считнных с карты данных byte[0...9]
    void Calculate();

    // Возвращает true, если UID рассчитан
    bool Calcualted() const
    {
        return calculated;
    }

    // Первые 5 байт - 1 каскад, вторые 5 байт - второй каскад
    uint8 bytes[10];

    // Возвращает 3 байта для передачи по WG
    // Если короткий 4-байтный UID - то 3 из них. Если 7-байтный UID - то 3 байта NUID
    uint Get3BytesForm() const;

    // Возвращает UID в виде [полный UID]*[3-байтный UID]
    String<> ToASCII() const;

    // Возвращает четыре байта serial number для аутентификации Mifare Classic
    uint8 *GetSerialNumber(uint8[4]) const;

    // Получить полный GUID
    uint64 GetRaw() const;

private:

    bool calculated;

    // В самом младшем байте хранится последний байт UID, таким образом спецификация %llu даёт правильный вывод
    uint64 uid;

    bool Is7BytesUID() const
    {
        return bytes[0] == 0x88;
    }

    // Если full == true - полная форма (4 или 7 байт), если false - сокращённая - 3 байта
     String<> ToString(bool full) const;
};


// "Размерность" карты
struct CardSize
{
    enum E
    {
        Undefined,
        _1k,
        _2k,
        _4k,
        Count
    };

    E value;

    pchar ToASCII() const;
};


struct ATS
{
    ATS() : length(0)
    {
        Clear();
    }
    void Clear();
    void Set(uint8 *bytes, int length);
    bool IsCorrect() const;
    String<65> ToASCII() const;
    bool HistoricalBytes_MFP_S() const;
    bool HistoricalBytes_MFP_X() const;
    bool HistoricalBytes_MFP_SE() const;
private:
    uint64 HistoricalBytes() const;
    static const int SIZE = 16;
    uint8 b[SIZE];
    int length;
};


struct SAK
{
    SAK() : sak(0) { }
    void Set(uint8 value)
    {
        sak = value;
    }
    void Reset()
    {
        sak = 0;
    }
    bool GetBit(int bit)
    {
        return _GET_BIT(sak, bit) != 0;
    }
    uint8 Raw() const
    {
        return sak;
    }

private:
    uint8 sak;
};


// Преобразует 7-байтный UID в 4-байтный NUID
struct ConvertorUID
{
    ConvertorUID(uint64 _uid) : uid(_uid)
    {
    }

    uint Get() const;

private:

    uint64 uid;

    void Convert7ByteUIDTo4ByteNUID(uint8 *uc7ByteUID, uint8 *uc4ByteUID) const;

    uint16 UpdateCrc(uint8 ch, uint16 *lpwCrc) const;

    void ComputeCrc(uint16 wCrcPreset, uint8 *Data, int Length, uint16 &usCRC) const;
};


struct TypeAuth
{
    TypeAuth(bool _enabled = true, uint64 _password = 0) :
        enabled(_enabled),
        password(_password)
    {
    }

    bool   enabled;     // Если true - подключаемся по паролю. Иначе - без
    uint64 password;

    bool operator!=(const TypeAuth &rhs)
    {
        return !(*this == rhs);
    }

    bool operator==(const TypeAuth &rhs) const                          // \todo две версии оператора
    {
        return enabled == rhs.enabled && password == rhs.password;
    }

    bool operator==(const TypeAuth &rhs)                                // \todo две версии оператора //-V659
    {
        if (enabled != rhs.enabled)
        {
            return false;
        }

        if (!enabled)
        {
            return true;
        }

        return password == rhs.password;
    }
};
