// 2023/08/31 13:22:50 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


#define LOG_WRITE(...)    Log::Write(Log::Type::Info, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...)  Log::Write(Log::Type::Warning, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)    Log::Write(Log::Type::Error, __FILE__, __LINE__, __VA_ARGS__)


namespace HAL_Status
{
    pchar Name(int);
};

namespace Log
{
    struct Type
    {
        enum E
        {
            Info,
            Warning,
            Error
        };
    };

    void Write(Type::E, pchar file, int line, pchar format, ...);

    void TraceLine(char *, int);

    void Log16Bytes(pchar name, const uint8 *);
}
