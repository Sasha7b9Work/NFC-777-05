// 2022/7/6 10:32:35 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/Commands.h"
#include "Reader/RC663/Registers.h"
#include "Reader/RC663/RC663.h"
#include "Hardware/HAL/HAL.h"
#include "Hardware/HAL/HAL_PINS.h"
#include "Hardware/Timer.h"
#include "Utils/Math.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"
#include <cstring>
#include <cstdio>


using namespace RC663;


void Command::Idle()
{
    Register(MFRC630_REG_COMMAND).Write(MFRC630_CMD_IDLE);
}


void Command::Reset()
{
    Register(MFRC630_REG_COMMAND).Write(MFRC630_CMD_SOFTRESET);
}


uint16 Command::ReqA()
{
    FIFO::Clear();

    IRQ0::Clear();                                   // Очищаем флаги

    Register(MFRC630_REG_TXCRCPRESET).Write(MFRC630_RECOM_14443A_CRC | MFRC630_CRC_OFF);    // Switches the CRC extention OFF in tx direction //-V525
                                                                                            // TxCRCEn == 0 (CRC is not appended to the data stream)
                                                                                            // TXPresetVal == 0001 (6363h for CRC16)
                                                                                            // TxCRCtype == 10 (CRC16)

    Register(MFRC630_REG_RXCRCCON).Write(MFRC630_RECOM_14443A_CRC | MFRC630_CRC_OFF);       // Switches the CRC extention OFF in rx direction
                                                                                            // RxCRCEn == 0 (If set, the CRC is checked and in case of a wrong CRC an error flag is
                                                                                            // set.Otherwise the CRC is calculated but the error flag is not modified)

    Register(MFRC630_REG_TXDATANUM).Write(0x0F);            // Only the 7 last bits will be sent via NFC
                                                            // TxLastBits == 111
                                                            // DataEn == 1 (data is sent)

    Command::Transceive1(MFRC630_ISO14443_CMD_REQA);        // REQA

    uint16 atqa = 0;

    TimeMeterUS meterUS;

    while (meterUS.ElapsedUS() < 2900)              // Запрос REQA //-V654
    {
        if (!pinIRQ_TRX.IsHi())                     // данные получены
        {
            if (IRQ0::GetValue() & IRQ0::ErrIRQ)     // ошибка данных
            {
                break;
            }
            else                                    // данные верны
            {
                atqa = FIFO::Pop();
                atqa |= (FIFO::Pop() << 8);

                break;
            }
        }
    }

    return atqa;
}


void Command::Transceive1(uint8 command)
{
    Transceive(&command, 1);
}


void Command::Transceive(uint8 command, uint8 data)
{
    uint8 buffer[2] = { command, data };

    Transceive(buffer, 2);
}


void Command::Transceive(const uint8 *buffer, int size)
{
    Idle();

    FIFO::Flush();

    for (int i = 0; i < size; i++)
    {
        FIFO::Push(*buffer++);
    }

    Register(MFRC630_REG_COMMAND).Write(MFRC630_CMD_TRANSCEIVE);
}


bool Command::AnticollisionCL(int cl, UID *uid)
{
    Command::Idle();
    FIFO::Clear();
    IRQ0::Clear();

    Register(MFRC630_REG_TXCRCPRESET).Write(MFRC630_RECOM_14443A_CRC | MFRC630_CRC_OFF);    // Switches the CRC extention ON in tx direction //-V525
    Register(MFRC630_REG_RXCRCCON).Write(MFRC630_RECOM_14443A_CRC | MFRC630_CRC_OFF);       // Switches the CRC extention OFF in rx direction

    Register(MFRC630_REG_TXDATANUM).Write(0x08);                                            // All bits will be sent via NFC

    IRQ0::Clear();

    Command::Transceive((cl == 1) ? MFRC630_ISO14443_CAS_LEVEL_1 : MFRC630_ISO14443_CAS_LEVEL_2, 0x20U);

    TimeMeterMS meter;

    while (meter.ElapsedMS() < 10)
    {
        if (!pinIRQ_TRX.IsHi())
        {
            if (RegError().Read() & RegError::CollDet)
            {
                return false;
            }

            if (IRQ0::GetValue() & IRQ0::ErrIRQ)
            {
                return false;
            }
            else
            {
                int i0 = (cl == 1) ? 0 : 5;

                uid->bytes[i0 + 0] = FIFO::Pop();
                uid->bytes[i0 + 1] = FIFO::Pop();
                uid->bytes[i0 + 2] = FIFO::Pop();
                uid->bytes[i0 + 3] = FIFO::Pop();
                uid->bytes[i0 + 4] = FIFO::Pop();

                return true;
            }
        }
    }

    return false;
}


bool Command::SelectCL(int cl, UID *uid)
{
    Command::Idle();
    FIFO::Clear();

    Register(MFRC630_REG_TXCRCPRESET).Write(MFRC630_RECOM_14443A_CRC | MFRC630_CRC_ON);      // Switches the CRC extention ON in tx direction //-V525
    Register(MFRC630_REG_RXCRCCON).Write(MFRC630_RECOM_14443A_CRC | MFRC630_CRC_ON);         // Switches the CRC extention OFF in rx direction

    Register(MFRC630_REG_TXDATANUM).Write(0x08);

    IRQ0::Clear();

    int i0 = (cl == 1) ? 0 : 5;

    uint8 buffer[7] = { cl == 1 ? (uint8)MFRC630_ISO14443_CAS_LEVEL_1 : (uint8)MFRC630_ISO14443_CAS_LEVEL_2,
        0x70,
        uid->bytes[i0 + 0],
        uid->bytes[i0 + 1],
        uid->bytes[i0 + 2],
        uid->bytes[i0 + 3],
        uid->bytes[i0 + 4] };

    Command::Transceive(buffer, 7);

    TimeMeterMS meter;

    Card::sak.Reset();

    while (meter.ElapsedMS() < 10)
    {
        if (!pinIRQ_TRX.IsHi())
        {
            if (IRQ0::GetValue() & IRQ0::ErrIRQ)
            {
                return false;
            }
            else
            {
                Card::sak.Set(FIFO::Pop());

                if (Card::sak.GetBit(2))
                {
                    return true;
                }

                if (!Card::sak.GetBit(2))
                {
                    uid->Calculate();
                    return true;
                }
            }
        }
    }

    return false;
}
