// 2024/12/24 09:15:08 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/Cards/Card.h"


namespace CardMF
{
    namespace Plus
    {
        bool ReadBlock(int nbr_block, Block16 &);

        bool WriteBlock(int nbr_block, const Block16 &);
    }
}


bool CardMF::Plus::ReadBlock(int nbr_block, Block16 &block)
{
    if (Card::_14443::T_CL::ReadBlock(nbr_block, block))
    {
        Block16 rd(0, 0);

        rd.FillPseudoRandom();

        if (Card::_14443::T_CL::ReadBlock(nbr_block, rd))
        {
            return rd == block;
        }
    }

    return false;
}


bool CardMF::Plus::WriteBlock(int nbr_block, const Block16 &block)
{
    if (Card::_14443::T_CL::WriteBlock(nbr_block, block))
    {
        Block16 rd(0, 0);

        rd.FillPseudoRandom();

        if (Card::_14443::T_CL::ReadBlock(nbr_block, rd))
        {
            if (rd == block)
            {
                return true;
            }
        }
    }

    return false;
}
