// 2024/12/24 09:15:45 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/Cards/TypeCard.h"


namespace CardMF
{
    namespace Classic
    {
        bool ReadBlock(int nbr_block, Block16 &);

        bool WriteBlock(int nbr_block, const Block16 &);

        void ResetReadWriteOperations();
    }

    namespace Plus
    {
        bool ReadBlock(int nbr_block, Block16 &);

        bool WriteBlock(int nbr_block, const Block16 &);

        void ResetReadWriteOperations();
    }
}


void CardMF::ResetReadWriteOperations()
{
    Plus::ResetReadWriteOperations();
    Classic::ResetReadWriteOperations();
}


bool CardMF::ReadBlock(int nbr_block, Block16 &block)
{
    if (TypeCard::IsSupportT_CL())
    {
        return Plus::ReadBlock(nbr_block, block);
    }
    else
    {
        return Classic::ReadBlock(nbr_block, block);
    }
}


bool CardMF::WriteBlock(int nbr_block, const Block16 &block)
{
    if (TypeCard::IsSupportT_CL())
    {
        return Plus::WriteBlock(nbr_block, block);
    }
    else
    {
        return Classic::WriteBlock(nbr_block, block);
    }
}


bool CardMF::GetAddressBlockNTAG(int nbr_block_NTAG, int *nbr_block, int *first_byte)
{
    *nbr_block = 0;
    *first_byte = 0;

    if (nbr_block_NTAG < 4)
    {
    }
    else if (nbr_block_NTAG < 8)    //////////////////// Sector 0
    {
        *nbr_block = 1;

        *first_byte = (nbr_block_NTAG - 4) * 4;
    }
    else if (nbr_block_NTAG < 12)
    {
        *nbr_block = 2;

        *first_byte = (nbr_block_NTAG - 8) * 4;
    }

    else if (nbr_block_NTAG < 16)   //////////////////// Sector 1
    {
        *nbr_block = 4;

        *first_byte = (nbr_block_NTAG - 12) * 4;
    }
    else if (nbr_block_NTAG < 20)
    {
        *nbr_block = 5;

        *first_byte = (nbr_block_NTAG - 16) * 4;
    }
    else if (nbr_block_NTAG < 24)
    {
        *nbr_block = 6;

        *first_byte = (nbr_block_NTAG - 20) * 4;
    }

    else if (nbr_block_NTAG < 28)   //////////////////// Sector 2
    {
        *nbr_block = 8;

        *first_byte = (nbr_block_NTAG - 24) * 4;
    }
    else if (nbr_block_NTAG < 32)
    {
        *nbr_block = 9;

        *first_byte = (nbr_block_NTAG - 28) * 4;
    }
    else if (nbr_block_NTAG < 36)
    {
        *nbr_block = 10;

        *first_byte = (nbr_block_NTAG - 32) * 4;
    }

    else if (nbr_block_NTAG < 40)   //////////////////// Sector 3
    {
        *nbr_block = 12;

        *first_byte = (nbr_block_NTAG - 36) * 4;
    }

    return (*nbr_block) != 0;
}


bool CardMF::BlockIsPassword(int nbr_block_MF)
{
    return ((nbr_block_MF + 1) % 4) == 0;
}


int CardMF::NumFirstBlockInSector(int sector)
{
    return sector * 4;      // \todо для Mifare Classic 1k это всегда верно
}


int CardMF::NumSectorForBlock(int block)
{
    return block / 4;       // \todо для Mifare Classic 1k это всегда верно
}
