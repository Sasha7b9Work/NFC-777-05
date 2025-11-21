// 2024/09/18 14:54:12 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/Commands.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/RC663/Commands.h"
#include "Reader/RC663/Registers.h"
#include "Reader/RC663/RC663_def.h"
#include "Hardware/Timer.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"


using RC663::Register;


namespace CardMF
{
    namespace Classic
    {
        void LoadKey();

        static bool AuthSector(int nbr_sector);

        bool ReadBlock(int nbr_block, Block16 &);

        bool WriteBlock(int nbr_block, const Block16 &block);

        static bool ReadBlockRAW(int nbr_block, Block16 &);

        static bool WriteBlockRAW(int nbr_block, const Block16 &);

        static int last_auth_sector = -1;      // Последний сектор, с которым производилась работа.
                                                // Нужно, чтобы не производить лишнюю аутентификацию

        static void CommandAutenticate(uint8 key_type, int nbr_block, const uint8 *card_uid);

        void ResetReadWriteOperations();
    }
}


void CardMF::Classic::ResetReadWriteOperations()
{
    last_auth_sector = -1;
}



bool CardMF::Classic::ReadBlock(int nbr_block, Block16 &block)
{
    if (ReadBlockRAW(nbr_block, block))
    {
        Block16 block2(0, 0);

        if (ReadBlockRAW(nbr_block, block2))
        {
            return (block == block2);
        }
    }

    return false;
}


bool CardMF::Classic::WriteBlock(int nbr_block, const Block16 &block)
{
    // \todo На Classic 4x UID7 много ошибок записи. Потому так и сделано. Разобраться и исправить
    for (int i = 0; i < 10; i++)
    {
        WriteBlockRAW(nbr_block, block);

        if (BlockIsPassword(nbr_block))
        {
            return true;
        }
        else
        {
            Block16 rx = block;

            for (int byte = 0; byte < 16; byte++)
            {
                rx.bytes[byte]++;
            }

            ReadBlockRAW(nbr_block, rx);

            if (std::memcmp(block.bytes, rx.bytes, 16) == 0)
            {
                return true;
            }
        }
    }

    return false;
}


static bool CardMF::Classic::WriteBlockRAW(int nbr_block, const Block16 &source)
{
    if (AuthSector(NumSectorForBlock(nbr_block)))
    {
        uint8 data[2] = { MFRC630_MF_CMD_WRITE, (uint8)nbr_block };

        Card::_14443::Transceive(data, 2, 1, 0);

        Command::Idle();

        if (RC663::IRQ0::GetValue() & MFRC630_IRQ0_ERR_IRQ)
        {
            // some error
            return false;
        }

        if (RC663::FIFO::Length() != 1)
        {
            return false;
        }

        uint8 res = 0;

        RC663::FIFO::Read(&res, 1);

        if (res != MFRC630_MF_ACK)
        {
            return false;
        }

        RC663::IRQ0::Clear();  // clear irq0
        RC663::IRQ1::Clear();  // clear irq1

        // go for the second stage.
        Command::Transceive(source.bytes, 16);

        TimeMeterMS meter;

        while (!(RC663::IRQ1::GetValue() & MFRC630_IRQ1_GLOBAL_IRQ))        // stop polling irq1 and quit the timeout loop.
        {
            if (meter.ElapsedMS() > Card::timeout)
            {
                return false;
            }
        }

        Command::Idle();

        if (RC663::IRQ0::GetValue() & MFRC630_IRQ0_ERR_IRQ)
        {
            // some error
            return false;
        }

        if (RC663::FIFO::Length() != 1)
        {
            return false;
        }

        RC663::FIFO::Read(&res, 1);

        if (res == MFRC630_MF_ACK)
        {
            return true;  // second stage was responded with ack! Write successful.
        }
    }

    return false;
}


static bool CardMF::Classic::ReadBlockRAW(int nbr_block, Block16 &dest)
{
    int sector = NumSectorForBlock(nbr_block);

    if (AuthSector(sector))
    {
        uint8 data[2] = { MFRC630_MF_CMD_READ, (uint8)nbr_block };

        Card::_14443::Transceive(data, 2, 1, 1);

        Command::Idle();

        uint8 irq0_value = RC663::IRQ0::GetValue();
        if (irq0_value & MFRC630_IRQ0_ERR_IRQ)
        {
            // some error
            return false;
        }

        // all seems to be well...
        uint8 buffer_length = (uint8)RC663::FIFO::Length();
        uint8 rx_len = (uint8)((buffer_length <= 16) ? buffer_length : 16);
        RC663::FIFO::Read(dest.bytes, rx_len);
        return true;
    }

    return false;
}


void CardMF::Classic::LoadKey()
{
    if (TypeCard::IsMifare() && !TypeCard::IsSupportT_CL())
    {
        Command::Idle();
        RC663::FIFO::Flush();
        RC663::FIFO::Write(Card::GetPasswordAuth().bytes, 6);
        Register(MFRC630_REG_COMMAND).Write(MFRC630_CMD_LOADKEY);
    }
}


bool CardMF::Classic::AuthSector(int sector)
{
    if (sector == last_auth_sector)
    {
        return true;
    }

    LoadKey();

    // Enable the right interrupts.

    // configure a timeout timer.
    uint8 timer_for_timeout = 0;  // should match the enabled interupt.

    // According to datashet Interrupt on idle and timer with MFAUTHENT, but lets
    // include ERROR as well.
    Register(MFRC630_REG_IRQ0EN).Write(MFRC630_IRQ0EN_IDLE_IRQEN | MFRC630_IRQ0EN_ERR_IRQEN);
    Register(MFRC630_REG_IRQ1EN).Write(MFRC630_IRQ1EN_TIMER0_IRQEN);  // only trigger on timer for irq1

    // Set timer to 221 kHz clock, start at the end of Tx.
    RC663::Timer::SetControl(timer_for_timeout, MFRC630_TCONTROL_CLK_211KHZ | MFRC630_TCONTROL_START_TX_END);
    // Frame waiting time: FWT = (256 x 16/fc) x 2 FWI
    // FWI defaults to four... so that would mean wait for a maximum of ~ 5ms

    RC663::Timer::SetReload(timer_for_timeout, 2000);  // 2000 ticks of 5 usec is 10 ms.
    RC663::Timer::SetValue(timer_for_timeout, 2000);

    uint8 irq1_value = 0;

    RC663::IRQ0::Clear();  // clear irq0
    RC663::IRQ1::Clear();  // clear irq1

    uint8 sn[4];

    // start the authentication procedure.
    CommandAutenticate(MFRC630_MF_AUTH_KEY_A, NumFirstBlockInSector(sector), Card::uid.GetSerialNumber(sn));

    // block until we are done
    while (!(irq1_value & (1 << timer_for_timeout)))
    {
        irq1_value = RC663::IRQ1::GetValue();
        if (irq1_value & MFRC630_IRQ1_GLOBAL_IRQ)
        {
            break;  // stop polling irq1 and quit the timeout loop.
        }
    }

    if (irq1_value & (1 << timer_for_timeout))
    {
        last_auth_sector = -1;

        // this indicates a timeout
        return false;  // we have no authentication
    }

    // status is always valid, it is set to 0 in case of authentication failure.
    bool result = (Register(MFRC630_REG_STATUS).Read() & MFRC630_STATUS_CRYPTO1_ON) != 0;

    last_auth_sector = result ? sector : -1;

    return result;
}


static void CardMF::Classic::CommandAutenticate(uint8 key_type, int nbr_block, const uint8 *card_uid)
{
    Command::Idle();
    uint8 parameters[6] = { key_type, (uint8)nbr_block, card_uid[0], card_uid[1], card_uid[2], card_uid[3] };
    RC663::FIFO::Flush();
    RC663::FIFO::Write(parameters, 6);
    Register(MFRC630_REG_COMMAND).Write(MFRC630_CMD_MFAUTHENT);
}
