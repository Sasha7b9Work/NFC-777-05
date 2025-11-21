// 2023/06/08 11:58:09 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Utils/String.h"


// ѕросто место дл€ хранени€ строки
struct ParseBuffer
{
    char data[256];

    int GetLength() const;

    // true, строка соотвествует
    bool IsEqual(pchar) const;

    // ¬озвращает true, если здесь хранитс€ параметр (две строки, разделЄнные символом '=')
    bool IsParameter(pchar param) const;
};


// ѕараметр вида "param=123"
struct Parameter
{
    Parameter(const ParseBuffer &_buffer) : buffer(_buffer) { }

    int GetInt() const;

    bool GetUInt(uint *out_value, int base) const;

    // ¬озвращает значение параметра uint64, представленного в дес€тичном виде
    uint64 GetUInt64dec() const;

    // ¬озвращает значение параметра uint128, прадставленного в шестнадцатиричном виде
    Block16 GetUint128hex() const;

private:

    ParseBuffer buffer;
};


namespace SU
{
    // Ќумераци€ начинаетс€ с 1
    char *GetWord(pchar data, int num, ParseBuffer *);

    // Ќумераци€ начинаетс€ с 1
    char *GetString(pchar data, int num, char *out, pchar *first_symbol = nullptr);

    // ¬озвращает первый символ подстроки sub_str в строке string
    pchar GetSubString(pchar string, pchar sub_str);

    // —читывает значение int из дес€тичного ASCII-представлени€
    bool AtoIntDec(pchar, int *value);

    // —читывает значение uint из дес€тичного или шестнадцатиричного ASCII-представлени€
    bool AtoUInt(pchar, uint *value, int radix);

    // —читывает значение uint64 из дес€тичного или шестнадцатиричного ASCII-представлени€
    bool AtoUInt64(pchar, uint64 *value, int radix);

    // —читывает значение uint128 из шестнадцатиричного ASCII-представлени€
    bool AtoUInt128hex(pchar, Block16 *value);

    // ≈сли строка data содержит символ symbol, то в position возвращаетс€ позици€ его первого вхождени€
    bool ConsistSymbol(pchar data, char symbol, int *position);

    // »звлекает строку списка разрещЄнных карт в out.
    // —имволы на выходе получаютс€ в том же виде, что на входе - последовательность 16-ричных символов, пришедша€ из CIC дл€ записи мастер-карты
    char *ExtractBitMapCards(pchar, char out[161]);
}
