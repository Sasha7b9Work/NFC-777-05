// 2023/08/31 13:22:41 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Utils/Log.h"
#include "Hardware/HAL/HAL.h"
#include "Communicator/ROMWriter.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>


void Log::TraceLine(char *file, int line)
{
    if (!ROMWriter::IsCompleted())
    {
        return;
    }

    HAL_USART::UART::TransmitF("Trace : %s : %d", file, line);
}


void Log::Log16Bytes(pchar name, const uint8 *b)
{
    LOG_WRITE("%s : %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X", name,
        b[0],
        b[1],
        b[2],
        b[3],
        b[4],
        b[5],
        b[6],
        b[7],
        b[8],
        b[9],
        b[10],
        b[11],
        b[12],
        b[13],
        b[14],
        b[15]
    );
}


void Log::Write(Type::E type, pchar file, int line, pchar format, ...)
{
    if (!ROMWriter::IsCompleted())
    {
        return;
    }

    char message[256] = { '\0' };

    static int counter = 0;

    if (type == Type::Info)
    {
        std::sprintf(message, "#LOG %d:", counter++);
    }
    else if (type == Type::Warning)
    {
        std::sprintf(message, "#WARNING %d:", counter++);
    }
    else if (type == Type::Error)
    {
        std::sprintf(message, "#ERROR %d:", counter++);
    }

    int num_symbols_name = 20 - (int)std::strlen(message);          // Столько символов на имя файла

    pchar pointer_name = file + std::strlen(file) - num_symbols_name;

    if (pointer_name < file)
    {
        pointer_name = file;
    }

    std::snprintf(message + std::strlen(message), 256, "%s:%4d ", pointer_name, line);

    std::va_list args;
    va_start(args, format);
    std::vsprintf(message + std::strlen(message), format, args);
    va_end(args);

    HAL_USART::UART::TransmitF(message);
}


pchar HAL_Status::Name(int status)
{
    static const pchar names[4] =
    {
        "HAL_OK",
        "HAL_ERROR",
        "HAL_BUSY",
        "HAL_TIMEOUT"
    };

    if (status < 4)
    {
        return names[status];
    }

    return "Undefined status";
}
