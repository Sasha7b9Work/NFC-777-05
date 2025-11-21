// 2024/09/18 17:16:41 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/Registers.h"
#include "Reader/RC663/RC663_def.h"
#include "Reader/RC663/Commands.h"
#include "Utils/Math.h"
#include "Hardware/Timer.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/Cards/CardNTAG.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/Card.h"


bool CardNTAG::ReadBlock(int nbr_block, uint8 block[4])
{
    uint8 data[2] = { MFRC630_MF_CMD_READ, (uint8)nbr_block };

    return Card::_14443::TransceiveReceive(data, 2, block, 4, 1, 1) == 16;
}



bool CardNTAG::WriteBlock(int nbr_block, const uint8 block[4])
{
    uint8 tx[6] = { 0xA2, (uint8)nbr_block };             // \warn Здесь используется оригинальная команда NTAG, а не совместимая от Mifare 0xA0

    std::memcpy(tx + 2, block, 4);

    uint8 result = 0;

    return (Card::_14443::TransceiveReceive(tx, 6, &result, 1, 1, 0) == 1) && (result == MFRC630_MF_ACK);
}


bool CardNTAG::PasswordAuth()
{
    Block16 password = Card::GetPasswordAuth();

    uint8 data[5] =
    {
        MFRC630_CMD_PWD_AUTH,
        password.bytes[0],
        password.bytes[1],
        password.bytes[2],
        password.bytes[3]
    };

    Card::_14443::Transceive(data, 5, 1, 1);

    Command::Idle();

    if (RC663::IRQ0::GetValue() & MFRC630_IRQ0_ERR_IRQ)
    {
        // some error
        return false;
    }

    bool result = RC663::FIFO::Length() >= 2;

    // all seems to be well...
    int buffer_length = RC663::FIFO::Length();
    int rx_len = (buffer_length <= 2) ? buffer_length : 2;

    uint8 buffer[2];

    RC663::FIFO::Read(buffer, rx_len);

    return result;
}
