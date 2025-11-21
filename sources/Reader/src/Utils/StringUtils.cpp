// 2023/06/08 12:00:44 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Utils/String.h"
#include "Utils/StringUtils.h"
#include <cstring>
#include <cstdlib>
#include <climits>


int ParseBuffer::GetLength() const
{
    return (int)std::strlen(data);
}


bool SU::ConsistSymbol(pchar data, char symbol, int *position)
{
    *position = 0;

    int length = (int)std::strlen(data);

    for (int i = 0; i < length; i++)
    {
        if (data[i] == symbol)
        {
            *position = i;
            return true;
        }
    }

    return false;
}


bool ParseBuffer::IsParameter(pchar param) const
{
    int position = 0;

    if (SU::ConsistSymbol(data, '=', &position))
    {
        uint length = std::strlen(param);

        return ((uint)position == length) && (std::memcmp(param, data, length) == 0);
    }

    return false;
}


bool ParseBuffer::IsEqual(pchar text) const
{
    return std::strcmp(data, text) == 0;
}


int Parameter::GetInt() const
{
    int position = 0;

    if (SU::ConsistSymbol(buffer.data, '=', &position))
    {
        char buf[32];

        uint length = std::strlen(buffer.data) - (uint)position - 1U;

        std::memcpy(buf, buffer.data + position + 1, length);

        buf[length] = '\0';

        int value = 0;

        SU::AtoIntDec(buf, &value);

        return value;
    }

    return 0;
}


bool Parameter::GetUInt(uint *out_value, int base) const
{
    int position = 0;

    if (SU::ConsistSymbol(buffer.data, '=', &position))
    {
        char buf[32];

        uint length = std::strlen(buffer.data) - (uint)position - 1U;

        std::memcpy(buf, buffer.data + position + 1, length);

        buf[length] = '\0';

        return SU::AtoUInt(buf, out_value, base);
    }

    return 0;
}


uint64 Parameter::GetUInt64dec() const
{
    int position = 0;

    if (SU::ConsistSymbol(buffer.data, '=', &position))
    {
        char buf[32];

        uint length = std::strlen(buffer.data) - (uint)position - 1U;

        std::memcpy(buf, buffer.data + position + 1, length);

        buf[length] = '\0';

        uint64 value = 0;

        SU::AtoUInt64(buf, &value, 10);

        return value;
    }

    return 0;
}


Block16 Parameter::GetUint128hex() const
{
    int position = 0;

    Block16 value(0, 0);

    if (SU::ConsistSymbol(buffer.data, '=', &position))
    {
        char buf[64];

        uint length = std::strlen(buffer.data) - (uint)position - 1U;

        std::memcpy(buf, buffer.data + position + 1, length);

        buf[length] = '\0';

        SU::AtoUInt128hex(buf, &value);
    }

    return value;
}


char *SU::GetString(pchar data, int num, char *out, pchar *first_symbol)
{
    if (num == 1)
    {
        out[0] = '\0';

        uint len = std::strlen(data);

        for (uint i = 0; i < len; i++)
        {
            if (data[i] == ' ' || data[i] == 0 || data[i] == '\n')
            {
                out[i] = '\0';
                return out;
            }

            out[i] = data[i];
        }

        if (first_symbol)
        {
            *first_symbol = data;
        }

        return out;
    }

    num--;

    out[0] = 0;

    uint size = std::strlen(data);

    uint pos_start = 0;

    for (uint i = 0; i < size; i++)
    {
        if (i == size - 1)
        {
            return out;
        }
        if (data[i] == ' ' || data[i] == '\n')
        {
            num--;

            if (num == 0)
            {
                pos_start = i + 1;
                break;
            }
        }
    }

    uint pos_end = pos_start;

    for (uint i = pos_end; i <= size; i++)
    {
        if (data[i] == ' ' || data[i] == '\0' || data[i] == '\n')
        {
            pos_end = i;
            break;
        }
    }

    if (pos_end > pos_start)
    {
        int index = 0;

        if (first_symbol)
        {
            *first_symbol = &data[pos_start];
        }

        for (uint i = pos_start; i < pos_end; i++)
        {
            out[index++] = data[i];
        }

        out[index] = 0;
    }

    return out;
}


char *SU::GetWord(pchar data, int num, ParseBuffer *out)
{
    return GetString(data, num, out->data);
}


pchar SU::GetSubString(pchar string, pchar sub_word)
{
    pchar end = string + std::strlen(string) - std::strlen(sub_word) + 1;

    uint str_len = std::strlen(sub_word);

    for (pchar pointer = string; pointer < end; pointer++)
    {
        if (std::memcmp(pointer, sub_word, str_len) == 0)
        {
            return pointer;
        }
    }

    return nullptr;
}


bool SU::AtoIntDec(pchar str, int *value)
{
    if (*str == '\0')
    {
        return false;
    }

    char *end;

    int r = std::strtol(str, &end, 10);

    if ((end != (str + std::strlen(str))))
    {
        return false;
    }

    *value = r;

    return true;
}


bool SU::AtoUInt(pchar str, uint *value, int radix)
{
    if (*str == '\0')
    {
        return false;
    }

    char *end;

    uint r = (uint)std::strtoull(str, &end, radix);

    if ((end != (str + std::strlen(str))))
    {
        return false;
    }

    *value = r;

    return true;
}


bool SU::AtoUInt64(pchar str, uint64 *value, int radix)
{
    char *end;

    uint64 r = std::strtoull(str, &end, radix);

    if ((end != (str + std::strlen(str))))
    {
        return false;
    }

    *value = r;

    return true;
}


bool SU::AtoUInt128hex(pchar str_value, Block16 *value)
{
    if (std::strlen(str_value) <= 16)                           // Число меньше, чем UINT64
    {
        value->u64[1] = 0;

        return AtoUInt64(str_value, &value->u64[0], 16);
    }
    else                                                        // Число не помещается в UIN64
    {
        char buffer[64];

        std::strncpy(buffer, str_value, 64);

        char *end = buffer + std::strlen(buffer);               // Указатель на конец

        if (AtoUInt64(end - 16, &value->u64[0], 16))         // Взяли младшую часть
        {
            end -= 16;

            *end = '\0';

            return AtoUInt64(buffer, &value->u64[1], 16);    // Взяли старшую часть
        }
    }

    return false;
}


char *SU::ExtractBitMapCards(pchar buffer, char out[161])
{
    pchar pointer = SU::GetSubString(buffer, "CRC32=");

    if (pointer)
    {
        pointer--;
        pointer--;

        while (*pointer != '\n')        // Ищем первый символ строки
        {
            pointer--;
        }   

        pointer++;

        for (int i = 0; i < 161; i++)
        {
            if (*pointer != '\n')
            {
                out[i] = *pointer++;
            }
            else
            {
                out[i] = '\0';
                break;
            }
        }

        out[160] = '\0';

        return out;
    }
    
    return nullptr;
}
