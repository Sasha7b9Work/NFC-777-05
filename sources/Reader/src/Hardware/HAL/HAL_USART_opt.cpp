// 2025/04/21 10:57:11 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Hardware/HAL/HAL.h"
#include "Communicator/OSDP.h"
#include "Hardware/Timer.h"


/*

    OFS - Optimized For Size

    Эти функции вынесены в отдельный файл для уменьшения размера

*/


namespace HAL_USART
{
    namespace Data
    {
        typedef void (*funcAppend)(uint8);

        extern funcAppend func;

        extern void AppendOSDP(uint8 byte);

        extern void AppendUSART(uint8 byte);

        static void Init()
        {
            func = gset.IsEnabledOSDP() ? AppendOSDP : AppendUSART;
        }
    }

    namespace UART
    {
        void Init();
    }

    namespace Mode
    {
        void Receive();

        // Здесь в режиме WG мы вибираем режим бита либо режим между битами
        namespace WG
        {
            // Уровень, соотвествующий передаче бита
            void LevelBit();

            // Уровень, соотвествующий промежутку между битами
            void LevelInterval();
        }
    }

    namespace WG
    {
        /*
        *  Старшие байты первые.
        *  Старшие биты первые.
        *  Длительность бита 100 мкс.
        *  Расстояние между битами 1 мс.
        *  1-й контрольный бит:
        *       если сложение значений первых 12 бит является нечетным числом, контрольному биту присваивается значение 1, чтобы результат сложения 13 бит был четным.
        *  2-й контрольный бит:
        *       последние 13 бит всегда дают в сумме нечетное число.
        */

        struct SendStruct
        {
            SendStruct(int _SIZE) : SIZE(_SIZE)
            {
            }
            void Append(bool);
            void CalculateControlBits();
            int  Size() const;
            bool GetBit(int) const;
        private:
            const int SIZE;
            bool bits[128];
            int pointer = 0;
            int NumOnes(int start_bit, int end_bit) const;
        };

        void Init();

        // Передать биты, начиная со старшего
        void TransmitRawBits(const SendStruct &);

        // Передать один бит. По meter отмеряют время до старта передачи (1мс)
        void TransmitBit(bool, TimeMeterMS &);

#ifdef EMULATE_WG

        static void SendToUart(const SendStruct &);

#endif
    }

    namespace UART
    {
#ifdef MCU_GD
#else
        extern uint8 buffer;
#endif
    }
}


void HAL_USART::Init()
{
    UART::Init();

    Data::Init();
}


void HAL_USART::UART::Init()
{
    pinTXD1.Init();

#ifdef MCU_GD

    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_2);      // TX
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_3);      // RX

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_2);

    /* configure USART Rx as alternate function push-pull */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_3);

    //    usart_deinit(USART1);
    rcu_periph_reset_enable(RCU_USART1RST);
    rcu_periph_reset_disable(RCU_USART1RST);

    //    usart_baudrate_set(USART1, ::OSDP::IsEnabled() ? gset.BaudRateOSDP().ToRAW() : 115200U);
    uint baudval = ::OSDP::IsEnabled() ? gset.BaudRateOSDP().ToRAW() : 115200U;
    uint uclk = rcu_clock_freq_get(CK_APB1);
    /* oversampling by 16, configure the value of USART_BAUD */
    uint udiv = (uclk + baudval / 2U) / baudval;
    uint intdiv = udiv & 0x0000fff0U;
    uint fradiv = udiv & 0x0000000fU;
    USART_BAUD(USART1) = ((USART_BAUD_FRADIV | USART_BAUD_INTDIV) & (intdiv | fradiv));

    //    usart_parity_config(USART1, USART_PM_NONE);
        /* disable USART */
    USART_CTL0(USART1) &= ~(USART_CTL0_UEN);
    /* clear USART_CTL0 PM,PCEN bits */
    USART_CTL0(USART1) &= ~(USART_CTL0_PM | USART_CTL0_PCEN);
    /* configure USART parity mode */
    USART_CTL0(USART1) |= USART_PM_NONE;

    //    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    USART_CTL0(USART1) &= ~USART_CTL0_TEN;
    /* configure transfer mode */
    USART_CTL0(USART1) |= USART_TRANSMIT_ENABLE;

    //    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    USART_CTL0(USART1) &= ~USART_CTL0_REN;
    /* configure receiver mode */
    USART_CTL0(USART1) |= USART_RECEIVE_ENABLE;

    //    usart_word_length_set(USART1, USART_WL_8BIT);
        /* disable USART */
    USART_CTL0(USART1) &= ~(USART_CTL0_UEN);
    /* clear USART_CTL0 WL bit */
    USART_CTL0(USART1) &= ~USART_CTL0_WL;
    /* configure USART word length */
    USART_CTL0(USART1) |= USART_WL_8BIT;

    //    usart_stop_bit_set(USART1, USART_STB_1BIT);
        /* disable USART */
    USART_CTL0(USART1) &= ~(USART_CTL0_UEN);
    /* clear USART_CTL1 STB bits */
    USART_CTL1(USART1) &= ~USART_CTL1_STB;
    USART_CTL1(USART1) |= USART_STB_1BIT;

    //    usart_interrupt_enable(USART1, USART_INT_RBNE);
    USART_REG_VAL(USART1, USART_INT_RBNE) |= BIT(USART_BIT_POS(USART_INT_RBNE));

    //    usart_enable(USART1);
    USART_CTL0(USART1) |= USART_CTL0_UEN;

#else

    pinTXD2.Init();
    pinRXD2.Init();

    __HAL_RCC_USART2_CLK_ENABLE();

    UART_HandleTypeDef *hUART = (UART_HandleTypeDef *)HAL_USART::handle;

    hUART->Instance = USART2;
    hUART->Init.BaudRate = ::OSDP::IsEnabled() ? gset.BaudRateOSDP().ToRAW() : 115200U;
    hUART->Init.WordLength = UART_WORDLENGTH_8B;
    hUART->Init.StopBits = UART_STOPBITS_1;
    hUART->Init.Parity = UART_PARITY_NONE;
    hUART->Init.Mode = UART_MODE_TX_RX;
    hUART->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hUART->Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(hUART);

    HAL_NVIC_SetPriority(USART2_IRQn, PRIORITY_USART);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    HAL_UART_Receive_IT(hUART, &buffer, 1);

#endif

    Mode::Receive();
}


void HAL_USART::WG::Transmit(uint64 value)
{
#ifndef EMULATE_WG

    WG::Init();

#endif

    /*
    *       Настройки
    *   1. Полный UID
    *   2. Наличие контрольных бит
    *   3. Инверсия контрольных бит по сравнению со стандартным WG26
    *   4. Обратный порядок передачи бит по сравнению со стандартным WG26
    *   5. Отбрасывать другой (младший) байт в UID4
    */

    // Вычисляем бит чётности. Он должен быть таким, чтобы количество единиц в первой половине бит было чётным
    // Если общее количество бит нечётное (не делится на 2), то средний бит участвует в расчёте и первой и второй половины

    SendStruct ss(gset.GetWiegandValue());

    if (gset.IsWiegandControlBitsEnabled())
    {
        ss.Append(false);                               // резервируем место для первого бита контроля чётности
    }

    int num_bits = gset.GetWiegandValue();              // Количество передаваемых бит

    if (gset.IsWiegandControlBitsEnabled())
    {
        num_bits -= 2;
    }

    if (gset.IsWiegandReverseOrderBits())
    {
        for (int i = num_bits - 1; i >= 0; i--)
        {
            const uint64 num_bit = (uint64)(63 - i);

            const bool bit = (value & (((uint64)1) << num_bit)) != 0;

            ss.Append(bit);
        }
    }
    else
    {
        for (int i = 0; i < num_bits; i++)
        {
            ss.Append((value & 0x8000000000000000) != 0);
            value <<= 1;
        }
    }

    while (ss.Size() < gset.GetWiegandValue())          // Забиваем оставшееся место нулями
    {
        ss.Append(false);
    }

    ss.CalculateControlBits();

#ifdef EMULATE_WG

    SendToUart(ss);

#else

    TransmitRawBits(ss);

    UART::Init();

#endif
}


void HAL_USART::WG::TransmitRawBits(const SendStruct &ss)
{
    Mode::WG::LevelInterval();

    TimeMeterMS meter;

    for (int i = 0; i < ss.Size(); i++)
    {
        TransmitBit(ss.GetBit(i) == false, meter);
    }

    Mode::WG::LevelInterval();

    meter.WaitFor(1);
}


void HAL_USART::WG::Transmit(uint8 b0, uint8 b1, uint8 b2)
{
    uint64 value = 0;

    uint64 b064 = (uint64)b0;
    value |= (uint64)(b064 << 56);

    uint64 b164 = (uint64)b1;
    value |= (uint64)(b164 << 48);

    uint64 b264 = (uint64)b2;
    value |= (uint64)(b264 << 40);

    Transmit(value);
}


void HAL_USART::WG::TransmitBit(bool bit, TimeMeterMS &meter)
{
    meter.WaitFor(1);

    meter.Reset();

    TimeMeterUS meterDuration;              // Для отмерения длительности импульса

#ifdef MCU_GD

    //    bit ? gpio_bit_set(GPIOA, GPIO_PIN_2) : gpio_bit_reset(GPIOA, GPIO_PIN_2);
    bit ? (GPIO_BOP(GPIOA) = GPIO_PIN_2) : (GPIO_BC(GPIOA) = GPIO_PIN_2);

#else

    pinTXD2.Set(bit);

#endif

    Mode::WG::LevelBit();

    meterDuration.WaitFor(100);

    Mode::WG::LevelInterval();
}


void HAL_USART::WG::Init()
{
#ifdef MCU_GD

    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

#else
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);

    uint ctrl = GPIOA->CRL;

    _CLEAR_BIT(ctrl, 10);   // \ CNF2 = 00  Push-Pull
    _CLEAR_BIT(ctrl, 11);   // /

    _SET_BIT(ctrl, 8);      // \ MODE2 = 11 max speed 50 MHz
    _SET_BIT(ctrl, 9);      // /

    GPIOA->CRL = ctrl;

    uint odr = GPIOA->ODR;
    _SET_BIT(odr, 2);
    GPIOA->ODR = odr;
#endif
}


void HAL_USART::WG::SendStruct::Append(bool bit)
{
    if (pointer < SIZE)
    {
        bits[pointer++] = bit;
    }
}


int HAL_USART::WG::SendStruct::Size() const
{
    return pointer;
}


void HAL_USART::Mode::WG::LevelBit()
{
    pinTXD1.ToLow();
}


void HAL_USART::Mode::WG::LevelInterval()
{
    pinTXD1.ToHi();
}


void HAL_USART::WG::SendStruct::CalculateControlBits()
{
    if (!gset.IsWiegandControlBitsEnabled())
    {
        return;
    }

    const int first_bit = 1;              // Первый подсчитываемый бит
    const int last_bit = Size() - 2;      // Последний подсчитываемый бит

    const int all_bits = last_bit - first_bit + 1;      // Столько всего бит нужно подсчитывать
    const int middle_bit = (last_bit + first_bit) / 2;

    {
        // Считаем первую половину

        int num_ones = NumOnes(first_bit, middle_bit);

        bool parity_start = ((num_ones % 2) != 0);

        if (!gset.IsWiegandControlBitsParityStandard())
        {
            parity_start = !parity_start;
        }

        bits[0] = parity_start;
    }

    {
        // Считаем вторую половину

        int start_bit = middle_bit;         // Это верно для нечётного количества передаваемых бит

        if ((all_bits % 2) == 0)            // А для чётного
        {
            start_bit++;                    // начинать будем со следующего
        }

        int num_ones = NumOnes(start_bit, last_bit);

        bool parity_end = ((num_ones % 2) == 0);

        if (!gset.IsWiegandControlBitsParityStandard())
        {
            parity_end = !parity_end;
        }

        bits[pointer - 1] = parity_end;
    }
}


int HAL_USART::WG::SendStruct::NumOnes(int start_bit, int end_bit) const
{
    int sum = 0;

    for (int i = start_bit; i <= end_bit; i++)
    {
        if (bits[i])
        {
            sum++;
        }
    }

    return sum;
}


bool HAL_USART::WG::SendStruct::GetBit(int num_bit) const
{
    return bits[num_bit];
}
