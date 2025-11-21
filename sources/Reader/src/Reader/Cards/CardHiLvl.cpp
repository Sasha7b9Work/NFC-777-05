// 2025/01/06 13:58:40 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/CardNTAG.h"
#include "Utils/Math.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/Cards/Card.h"


bool Card::HI_LVL::ReadBlock(int nbr_block_NTAG, Block4 &block_out)
{
    if (TypeCard::IsNTAG())
    {
        uint8 data1[4];

        if (CardNTAG::ReadBlock(nbr_block_NTAG, data1))
        {
            uint8 data2[4];

            if (CardNTAG::ReadBlock(nbr_block_NTAG, data2))
            {
                if (CardNTAG::ReadBlock(nbr_block_NTAG, block_out.bytes))
                {
                    return Math::ArraysEquals(block_out.bytes, data1, data2, 4);
                }
            }
        }
    }
    else if (TypeCard::IsMifare())
    {
        int nbr_block = -1;
        int first_byte = -1;

        if (CardMF::GetAddressBlockNTAG(nbr_block_NTAG, &nbr_block, &first_byte))
        {
            Block16 block(0, 0);

            if (CardMF::ReadBlock(nbr_block, block))
            {
                std::memcpy(block_out.bytes, block.bytes + first_byte, 4);

                return true;
            }
        }
    }

    return false;
}


bool Card::HI_LVL::Read2Blocks(int nbr_block, Block8 *bit_set)
{
    Block4 block0;
    Block4 block1;

    if (Card::HI_LVL::ReadBlock(nbr_block, block0) && Card::HI_LVL::ReadBlock(nbr_block + 1, block1))
    {
        bit_set->u32[0] = block0.u32;
        bit_set->u32[1] = block1.u32;

        return true;
    }

    return false;
}


bool Card::HI_LVL::Write2Blocks(int nbr_block, const uint8 tx[8])
{
    return WriteBlock(nbr_block, tx) && WriteBlock(nbr_block + 1, tx + 4);
}


bool Card::HI_LVL::ReadData(int nbr_block, void *_data, int size)
{
    while (size % 4)
    {
        size++;
    }

    int written_bytes = 0;
    uint8 *data = (uint8 *)_data;
    const uint8 *const end = data + size;

    while (data < end)
    {
        Block4 block;

        if (Card::HI_LVL::ReadBlock(nbr_block, block))
        {
            std::memcpy(data, block.bytes, 4);

            written_bytes += 4;
        }

        data += 4;

        nbr_block++;
    }

    return (written_bytes == size);
}


bool Card::HI_LVL::WriteBlock(int nbr_block_NTAG, const uint8 block[4])
{
    if (TypeCard::IsNTAG())
    {
        if (CardNTAG::WriteBlock(nbr_block_NTAG, block))
        {
            uint8 rx[4];

            if (CardNTAG::ReadBlock(nbr_block_NTAG, rx))
            {
                return std::memcmp(block, rx, 4) == 0;
            }
            else
            {
                return false;
            }
        }
    }
    else if (TypeCard::IsMifare())
    {
        int nbr_block = -1;
        int first_byte = -1;

        if (CardMF::GetAddressBlockNTAG(nbr_block_NTAG, &nbr_block, &first_byte))
        {
            Block16 rx(0, 0);

            if (CardMF::ReadBlock(nbr_block, rx))
            {
                std::memcpy(rx.bytes + first_byte, block, 4);

                return CardMF::WriteBlock(nbr_block, rx);
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}


bool Card::HI_LVL::WriteData(int nbr_block, const void *_data, int size)
{
    while (size % 4)
    {
        size++;
    }

    const uint8 *data = (const uint8 *)_data;
    const uint8 *const end = data + size;

    while (data < end)
    {
        int num_bytes = end - data;

        if (num_bytes >= 4)
        {
            if (!Card::HI_LVL::WriteBlock(nbr_block, data))
            {
                return false;
            }
        }
        else
        {
            Block4 block;

            if (Card::HI_LVL::ReadBlock(nbr_block, block))
            {
                for (uint i = 0; i < (uint)num_bytes; i++)
                {
                    block.bytes[i] = data[i];
                }

                if (!Card::HI_LVL::WriteBlock(nbr_block, block.bytes))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        data += 4;

        nbr_block++;
    }

    return true;
}
