// 2024/09/13 09:40:31 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/LPCD.h"
#include "Reader/RC663/RC663_def.h"
#include "Reader/RC663/Registers.h"
#include "Hardware/HAL/HAL.h"
#include "Hardware/Timer.h"
#include "Reader/Reader.h"


namespace RC663
{
    namespace LPCD
    {
        static uint8 OrTwoValuesLPCD(uint8 value1, uint8 value2, int first_bit);
    }
}


bool RC663::LPCD::IsEnabled()
{
    if (HAL::Is765() ||             // На USB-версии не включается этот режим
        ModeReader::IsExtended())   // И в режиме работы с ПО отключаем
    {
        return false;
    }

    return gset.IsEnabledLPCD();
}


void RC663::LPCD::Enter()
{
    if (!LPCD::IsEnabled())
    {
        return;
    }

    //> Prepare LPCD command, power down time 10[ms]. Cmd time 150[µsec].
    Register(MFRC630_REG_T3RELOADHI).Write(0x07);       // SR 1F 07 // Write T3 reload value Hi
    Register(MFRC630_REG_T3RELOADLO).Write(0xF2);       // SR 20 F2 // Write T3 reload value Lo
    Register(MFRC630_REG_T4RELOADHI).Write(0x00);       // SR 24 00 // Write T4 reload value Hi
    Register(MFRC630_REG_T4RELOADLO).Write(0x13);       // SR 25 13 // Write T4 reload value Lo
    Register(MFRC630_REG_T4CONTROL).Write(0xDF);        // SR 23 DF // Configure T4 for AutoLPCD and AutoRestart/Autowakeup. Use 2Khz LFO, Start T4
    Register(MFRC630_REG_LPCD_Q_RESULT).Write(0x40);    // SR 43 40 // Clear LPCD result

    Register(MFRC630_REG_RCV).Write(0x52);              // SR 38 52 // Set Rx_ADCmode bit

    regRxAna.StoreAndWrite(0x03);                       // GR 39 // Backup current RxAna setting
                                                        // SR 39 03 // Raise receiver gain to maximum

    // Wait until T4 is started
        // ::: L_WaitT4Started
    uint valueT4 = 0;
    while (valueT4 != 0x9F)
    {
        valueT4 = Register(MFRC630_REG_T4CONTROL).Read();
    }
    // Flush cmd and FIFO. Clear all IRQ flags
    Register(MFRC630_REG_COMMAND).Write(0x00);          // SR 00 00 //-V525
    Register(MFRC630_REG_FIFOCONTROL).Write(0xB0);      // SR 02 B0
    Register(MFRC630_REG_IRQ0).Write(0x7F);             // SR 06 7F
    Register(MFRC630_REG_IRQ1).Write(0x7F);             // SR 07 7F

    // Enable IRQ sources: Idle and PLCD
    Register(MFRC630_REG_IRQ0EN).Write(0x10);           // SR 08 10
    Register(MFRC630_REG_IRQ1EN).Write(0x60);           // SR 09 60

    Register(MFRC630_REG_COMMAND).Write(0x81);          // SR 00 81 //> Start RC663 cmd "Low power card detection". Enter PowerDown mode.
}


bool RC663::LPCD::IsCardDetected()
{
    if (!LPCD::IsEnabled())
    {
        return true;
    }

    // Wait until an IRQ occurs.
    // Power-down causes IC not to respond and script ops end with error.

    for(int i = 0; i < 100; i++)
    {
        // ::: L_StandBy
        uint8 command = Register(MFRC630_REG_COMMAND).Read();   // GR 00 // Read command register(00). If error - Stand-by.

        if (command == 0x01 || command == 0x00)
        {
            uint8 value_irq1 = IRQ1::GetValue();

            if (value_irq1 == 0x78)
            {
                return true;
            }
        }
    }

    return false;
}


static uint8 RC663::LPCD::OrTwoValuesLPCD(uint8 value1, uint8 value2, int first_bit)
{
    uint8 result = value1;

    if (_GET_BIT(value2, first_bit))
    {
        _SET_BIT(result, 6);
    }

    if (_GET_BIT(value2, first_bit + 1))
    {
        _SET_BIT(result, 7);
    }

    return result;
}


void RC663::LPCD::Calibrate()
{
    if (!LPCD::IsEnabled())
    {
        return;
    }

    FIFO::Init();

    HAL_FLASH::RC663::LoadAntennaConfiguration106();

    HAL_FLASH::RC663::LoadProtocol();

    Register(MFRC630_REG_TXAMP).Write(0x14);  /* Transmiter amplifier register */
    Register(MFRC630_REG_DRVCON).Write(0x39); /* Driver configuration register */
    Register(MFRC630_REG_TXL).Write(0x06);    /* Transmitter register */

    //  AN11145 3.2.1

    //  reset CLRC663 and idle
    //  SR  00 1F
    Command::Reset();
    //  SLP 50
    ::Timer::Delay(50);
    //   SR  00 00
    Command::Idle();

    //  Disable IRQ0, IRQ1 interrupt sources
    Register(MFRC630_REG_IRQ0).Write(0x7F);         //  SR 06 7F
    Register(MFRC630_REG_IRQ1).Write(0x7F);         //  SR 07 7F
    Register(MFRC630_REG_IRQ0EN).Write(0x00);       //  SR 08 00
    Register(MFRC630_REG_IRQ1EN).Write(0x00);       //  SR 09 00
    Register(MFRC630_REG_FIFOCONTROL).Write(0xB0);  //  SR 02 B0 // Flush FIFO

    //  //> LPCD_Config
    //  //> =============================================
    Register(MFRC630_REG_LPCD_QMIN).Write(0xC0);    //  SR 3F C0 // Set Qmin register
    Register(MFRC630_REG_LPCD_QMAX).Write(0xFF);    //  SR 40 FF // Set Qmax register
    Register(MFRC630_REG_LPCD_IMIN).Write(0xC0);    //  SR 41 C0 // Set Imin register

    //    Register(MFRC630_REG_DRVMOD).Write(0x89);       //  SR 28 89 // set DrvMode register
    Register(MFRC630_REG_DRVMOD).Write(0x86);

    //  // -----------------------------------------

    //  // Execute trimming procedure
    Register(MFRC630_REG_T3RELOADHI).Write(0x00);   //  SR 1F 00 // Write default. T3 reload value Hi
    Register(MFRC630_REG_T3RELOADLO).Write(0x10);   //  SR 20 10 // Write default. T3 reload value Lo
    Register(MFRC630_REG_T4RELOADHI).Write(0x00);   //  SR 24 00 // Write min. T4 reload value Hi
    Register(MFRC630_REG_T4RELOADLO).Write(0x05);   //  SR 25 05 // Write min. T4 reload value Lo
    Register(MFRC630_REG_T4CONTROL).Write(0xF8);    //  SR 23 F8 // Config T4 for AutoLPCD&AutoRestart.Set AutoTrimm bit.Start T4.

    //    Register(MFRC630_REG_LPCD_Q_RESULT).SetBit(6);  //  SR 43 40 // Clear LPCD result
    Register(MFRC630_REG_LPCD_Q_RESULT).Write(0x40);

    //    Register(MFRC630_REG_RCV).SetBit(6);            //  SR 38 52 // Set Rx_ADCmode bit
    Register(MFRC630_REG_RCV).Write(0x52);

    Register(MFRC630_REG_RXANA).Write(0x03);        //  SR 39 03 // Raise receiver gain to maximum
    Register(MFRC630_REG_COMMAND).Write(0x01);      //  SR 00 01 // Execute Rc663 command "Auto_T4" (Low power card detection and/or Auto trimming)

    ::Timer::Delay(10);

    //  // Flush cmd and Fifo
    Command::Idle();                                //  SR 00 00
    Register(MFRC630_REG_FIFOCONTROL).Write(0xB0);  //  SR 02 B0

    //    Register(MFRC630_REG_RCV).ClearBit(6);          //  SR 38 12 // Clear Rx_ADCmode bit
    Register(MFRC630_REG_RCV).Write(0x12);

    //  //> ------------ I and Q Value for LPCD ----------------
    //  //> ----------------------------------------------------
    uint8 result_i = Register(MFRC630_REG_LPCD_I_RESULT).Read();  //  GR 42 //> Get I
    uint8 result_q = Register(MFRC630_REG_LPCD_Q_RESULT).Read();  //  GR 43 //> Get Q

    const int TH = 1;

    uint8 bQMin = (uint8)(result_q - TH);
    uint8 bQMax = (uint8)(result_q + TH);
    uint8 bIMin = (uint8)(result_i - TH);
    uint8 bIMax = (uint8)(result_i + TH);

    uint8 lpcd_QMin = OrTwoValuesLPCD(bQMin, bIMax, 4);
    uint8 lpcd_QMax = OrTwoValuesLPCD(bQMax, bIMax, 2);
    uint8 lpcd_IMin = OrTwoValuesLPCD(bIMin, bIMax, 0);

    // Заносим рассчитанные значения
    Register(MFRC630_REG_LPCD_QMIN).Write(lpcd_QMin);
    Register(MFRC630_REG_LPCD_QMAX).Write(lpcd_QMax);
    Register(MFRC630_REG_LPCD_IMIN).Write(lpcd_IMin);
}
