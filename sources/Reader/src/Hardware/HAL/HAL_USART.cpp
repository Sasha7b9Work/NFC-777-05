// 2022/6/14 22:08:35 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Hardware/HAL/HAL.h"
#include "Modules/LIS2DH12/LIS2DH12.h"
#include "Reader/RC663/RC663.h"
#include "Hardware/Timer.h"
#include "Hardware/Power.h"
#include "Utils/Buffer.h"
#include "Utils/Mutex.h"
#include "Utils/StringUtils.h"
#include "Reader/Reader.h"
#include "Utils/RingBuffer.h"
#include "Utils/Math.h"
#include "Communicator/OSDP.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"
#include <system.h>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>


/*
*  Максимальная скорость передачи данных через USART
*  https://bravikov.wordpress.com/2012/08/24/%D1%81%D0%BA%D0%BE%D1%80%D0%BE%D1%81%D1%82%D1%8C-%D0%BF%D0%B5%D1%80%D0%B5%D0%B4%D0%B0%D1%87%D0%B8-%D0%B4%D0%B0%D0%BD%D0%BD%D1%8B%D1%85-%D1%87%D0%B5%D1%80%D0%B5%D0%B7-uart/
*/


// PA2     ------> USART2_TX / Сюда подаётся последовательность в режиме WG26
// PA3     ------> USART2_RX


namespace HAL_USART
{
    namespace Data
    {
        static bool filtering = true;

        static RingBufferU8 recv_buffer;            // Сюда помещаем принимаемые данные

        static BufferUSART processing_buffer;       // Перекладываем сюда перед обработкой

        static bool Get()
        {
            return recv_buffer.GetData(processing_buffer);
        }

        typedef void (*funcAppend)(uint8);

        void AppendUSART(uint8 byte)
        {
            if (filtering)
            {
                if (byte < 32 || byte > 127)
                {
                    if (byte != '\n')
                    {
                        return;
                    }
                }

                byte = (uint8)std::toupper((char)byte);
            }

            recv_buffer.Append(byte);
        }

        void AppendOSDP(uint8 byte)
        {
            recv_buffer.Append(byte);

            Get();

            ::OSDP::Update(processing_buffer);
        }

        funcAppend func = AppendUSART;

        void OnRecvByte(uint8 byte)
        {
            func(byte);
        }
    }


    void EnableFiltering(bool enable)
    {
        Data::filtering = enable;
    }

    namespace Mode
    {
        // Включить режим передачи
        void Transmit();

        // Включить режим приёма
        void Receive();
    }

    namespace UART
    {
#ifdef MCU_GD
#else
        uint8 buffer = 0;                    // Буфер для передачи данных через UART
#endif

        void Init();
    }

#ifdef MCU_GD
#else
    static UART_HandleTypeDef handleUART;

    void *handle = (void *)&handleUART;
#endif
}


void HAL_USART::UART::SendStringRAW(pchar message)
{
    TransmitRAW(message, (int)std::strlen(message));
}


void HAL_USART::UART::SendString(pchar message)
{
    TransmitRAW(message, (int)std::strlen(message));

    TransmitRAW("\n", 1);
}


void HAL_USART::UART::SendOK()
{
    SendString("#OK");
}


void HAL_USART::UART::SendStringF(char *format, ...)
{
    char message[300];
    std::va_list args;
    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);

    char buf[320];

    std::sprintf(buf, "%s\n", message);

    TransmitRAW(buf, (int)std::strlen(buf));
}


void HAL_USART::UART::TransmitRAW(const void *data, int size)
{
    if (!ModeReader::IsWG() && !::OSDP::IsEnabled() && !ModeOffline::IsEnabled())
    {
        Mode::Transmit();

#ifdef MCU_GD
        const uint8 *message = (const uint8 *)data;
        for (int i = 0; i < size; i++)
        {
            USART_TDATA(USART1) = (USART_TDATA_TDATA & (*message++));

            while (RESET == usart_flag_get(USART1, USART_FLAG_TC)) {}

            USART_INTC(USART1) |= USART_INTC_TCC;
        }

#else
        HAL_UART_Transmit(&handleUART, (uint8 *)data, (uint16)size, 1000);
#endif

        Mode::Receive();
    }
}


void HAL_USART::OSDP::Transmit(const void *data, int size)
{
    if (!ModeReader::IsWG() && ::OSDP::IsEnabled())
    {
        Mode::Transmit();

#ifdef MCU_GD
        const uint8 *message = (const uint8 *)data;
        for (int i = 0; i < size; i++)
        {
            while ((USART_REG_VAL(USART1, USART_FLAG_TBE) & BIT(USART_BIT_POS(USART_FLAG_TBE))) == 0) {}

            USART_TDATA(USART1) = (USART_TDATA_TDATA & ((uint)*message++));
        }

        while ((USART_REG_VAL(USART1, USART_FLAG_TC) & BIT(USART_BIT_POS(USART_FLAG_TC))) == 0) {}

#else
        HAL_UART_Transmit(&handleUART, (uint8 *)data, (uint16)size, 1000);
#endif

        Mode::Receive();
    }
}


void HAL_USART::OSDP::TransmitByte(uint8 byte)
{
    Transmit(&byte, 1);
}


void HAL_USART::UART::TransmitF(pchar format, ...)
{
    char message[256];
    std::va_list args;
    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);

    char data[300];

    std::sprintf(data, "%s\n", message);

    TransmitRAW(data, (int)std::strlen(data));
}


#ifdef EMULATE_WG

void HAL_USART::WG::SendToUart(const SendStruct &ss)
{
    char buffer[128] = { '\0' };

    for (int i = 0; i < ss.Size(); i++)
    {
        if (ss.GetBit(i))
        {
            std::strcat(buffer, "1");
        }
        else
        {
            std::strcat(buffer, "0");
        }

        if (gset.IsWiegandControlBitsEnabled())
        {
            if ((i % 8) == 0)
            {
                std::strcat(buffer, ".");
            }
        }
        else
        {
            if (((i + 1) % 8) == 0)
            {
                std::strcat(buffer, ".");
            }
        }
    }

    LOG_WRITE(buffer);
}

#endif


void HAL_USART::Mode::Transmit()
{
    pinTXD1.ToLow();
}


void HAL_USART::Mode::Receive()
{
    pinTXD1.ToHi();
}


bool HAL_USART::Update()
{
    if (::OSDP::IsEnabled())
    {
        return false;
    }

    bool result = Data::Get();

    Communicator::Update(Data::processing_buffer);

    return result;
}


#ifdef MCU_GD

// RS485
void USART1_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE))
    {
        uint8 data = (uint8)usart_data_receive(USART1);

        HAL_USART::Data::OnRecvByte(data);
    }
}

#else

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *)
{
    HAL_USART::Data::OnRecvByte(HAL_USART::UART::buffer);

    HAL_UART_Receive_IT(&HAL_USART::handleUART, &HAL_USART::UART::buffer, 1);
}

#endif
