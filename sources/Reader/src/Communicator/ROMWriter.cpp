// 2023/08/29 20:59:55 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Utils/Buffer.h"
#include "Hardware/HAL/HAL.h"
#include "Modules/Memory/Memory.h"
#include "Hardware/Timer.h"
#include "Modules/Indicator/Indicator.h"
#include "Communicator/ROMWriter.h"
#include <cstdlib>


namespace ROMWriter
{
    static int size_left = 0;       // Столько данных записать
    static uint address = 0;        // Сюда будем записывать очередную порцию данных
    static bool wait_data = false;  // Если true - ждём новые данные
    static int need_data = 0;       // Столько байт ждём
    static int offset = 0;

    static uint time_start = 0;
    static uint time_write = 0;     // Время записи в память
}


void ROMWriter::SetParameters(uint _address, int size)
{
    time_start = TIME_MS;
    time_write = 0;

    size_left = size;
    address = _address;
    wait_data = false;
    need_data = 0;

    offset = 0;

    HAL_USART::EnableFiltering(false);
}


void ROMWriter::Update(BufferUSART &buffer)
{
    const int SIZE_CHUNK = 256;

    if (!wait_data)
    {
        if (size_left > 0)
        {
            wait_data = true;
            need_data = (size_left >= SIZE_CHUNK) ? SIZE_CHUNK : size_left;
            HAL_USART::UART::SendStringF("#DATA OFFSET=%d SIZE=%d", offset, need_data);
        }
    }
    else if(buffer.Size() >= need_data)
    {
        wait_data = false;
        Memory::WriteBuffer(address, buffer.Data(), need_data);
        address += (uint)need_data;
        buffer.RemoveFirst(need_data);
        size_left -= need_data;
        offset += need_data;
    }

    if (IsCompleted())
    {
        HAL_USART::UART::SendString("#END");

        HAL_USART::EnableFiltering(true);

        if (address < ADDRESS_SOUNDS)
        {
            Timer::Delay(10);

#ifdef MCU_GD
            nvic_system_reset();
#else
            HAL_NVIC_SystemReset();
#endif
        }
    }
}


bool ROMWriter::IsCompleted()
{
    return (size_left == 0);
}
