// 2023/7/3 14:42:36 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Utils/Debug.h"
#include "Utils/Log.h"


namespace Debug
{
    pchar file = "";
    int line = 0;

    pchar file_int = "";
    int line_int = 0;

    int index = 0;
}


void Debug::Log(pchar _file, int _line)
{
    LOG_WRITE("%s:%d", _file, _line);
}
