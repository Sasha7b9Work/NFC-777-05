// 2023/06/26 12:51:47 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>


template<int size_buffer = 64>
class String
{
public:
    String(pchar format, ...)
    {
        buffer[0] = '\0';

        char text[1024];
        std::va_list args;
        va_start(args, format);
        vsprintf(text, format, args);
        va_end(args);

        if ((std::strlen(text) + 1) > size_buffer)
        {
        }
        else
        {
            std::strcpy(buffer, text);
        }
    }

    char *c_str()
    {
        return buffer;
    }

    bool operator ==(pchar rhs) const
    {
        return std::strcmp(buffer, rhs) == 0;
    }

    void Append(pchar str)
    {
        if ((std::strlen(buffer) + std::strlen(str)) < size_buffer)
        {
            std::strcat(buffer, str);
        }
    }

private:
    char buffer[size_buffer];
};
