// 2024/12/16 07:59:01 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/CardsEntities.h"
#include "Reader/Cards/CardNTAG.h"
#include "Reader/Cards/Card.h"
#include "Settings/Settings.h"
#include "Hardware/Timer.h"
#include "Utils/Log.h"


/*
+--------+------+----------------+------------------------------+
| ATQA   | SAK  |                |                              |
+--------+------+----------------+------------------------------+
| 0x44   | 0x00 |                | NTAG213 NTAG215 NTAG216      |
| 0x04   | 0x08 |                | MF Classic 1k UID4           |
| 0x42   | 0x18 |                | MF Classic 4k UID7           |
| 0x0400 | 0x88 |                | MF Classic EV1               |
| 0x0400 | 0x20 |                | MFP SE  SL0 1k               |
| 0x4400 | 0x20 | C1052F2F01BCD6 | MFP EV1 SL0 2k, MFP X SL0 2k |
+--------+------+----------------+------------------------------+
*/


namespace TypeCard
{
    static E current = Unidentified;

    static const pchar names_types[TypeCard::Count][2] =
    {
        { "Undefined",  "Undefined" },
        { "NTAG213",    "NTAG213" },
        { "NTAG215",    "NTAG215" },
        { "NTAG216",    "NTAG216" },
        { "MFC",        "Mifare_Classic" },
        { "MFC_EV1",    "Mifare_Classic_EV1" },
        { "MFP_S",      "Mifare_Plus_S" },
        { "MFP_X",      "Mifare_Plus_X" },
        { "MFP_SE",     "Mifare_Plus_SE" },
        { "MFP_EV1",    "Mifare_Plus_EV1" },
        { "MFP_EV2",    "Mifare_Plus_EV2" },
        { "MFP",        "Mifare_Plus" }
    };

    pchar ToShortASCII()
    {
        return names_types[Current()][0];
    }

    pchar ToLongASCII()
    {
        return names_types[Current()][1];
    }

    bool IsSupportT_CL()
    {
        return (current >= MFP_S);
    }

    bool IsReadEncripted()
    {
        return (current == MFP_X);
    }

    bool Set(TypeCard::E, CardSize::E, int sl = -1);

    bool DetectSL0();
}


bool TypeCard::Set(TypeCard::E type, CardSize::E size, int sl)
{
    current = type;
    Card::size_card.value = size;
    Card::SL = sl;

    if (sl == 0 || sl == 1)
    {
        Card::SetPasswordAuth(SettingsMaster::PSWD::default_Mifare);
    }

    return true;
}


bool TypeCard::DetectSL0()
{
    Block16 block(0, 0);

    if (Card::_14443::T_CL::InSL0())
    {
        CardSize::E size = CardSize::Undefined;

        if (_GET_BIT(Card::atqa, 2))
        {
            size = CardSize::_2k;
        }
        else if (_GET_BIT(Card::atqa, 1))
        {
            size = CardSize::_4k;
        }

        if (Card::ats.HistoricalBytes_MFP_S())
        {
            return Set(MFP_S, size, 0);
        }
        else if (Card::ats.HistoricalBytes_MFP_X())
        {
            return Set(MFP_X, size, 0);
        }
        else if (Card::ats.HistoricalBytes_MFP_SE())
        {
            return Set(MFP_SE, CardSize::_1k, 0);
        }
        else
        {
            return Set(MFP, size, 0);
        }
    }

    return false;
}


bool TypeCard::Detect()
{
//  NTAG213             +
//  NTAG215             +
//  NTAG216             +
//  Mifare Classic 1k   +
//  MIfare Classic 2k   +
//  Mifare Classic 4k   +
//  Mifare Plus S 2k    +
//  Mifare Plus S 4k    +
//  Mifare Plus X 2k    +
//  Mifare Plus X 4k    +
//  Mifare Plus SE 1k   +
//  Mifare Plus EV1 2k  +
//  Mifare Plus EV1 4k  +
//  Mifare Plus EV2 2k  +
//  Mifare Plus EV2 4k  +

    if (current != Unidentified)
    {
        return true;
    }

    if (IsNTAG())
    {
        Block4 block;

        if (Card::HI_LVL::ReadBlock(3, block))
        {
            if (block.bytes[0] == 0xE1 && block.bytes[1] == 0x10)
            {
                uint8 byte_type = block.bytes[2];

                if (byte_type == 0x12)
                {
                    current = NTAG213;
                    return true;
                }
                else if (byte_type == 0x3E)
                {
                    current = NTAG215;
                    return true;
                }
                else if (byte_type == 0x6D)
                {
                    current = NTAG216;
                    return true;
                }
            }
        }
    }
    else if (IsMifare())
    {
        // According AN10833

        if (Card::sak.GetBit(3))
        {
            if (Card::sak.GetBit(7))
            {
                return Set(MFC_EV1, CardSize::_1k);     // \todo Ќе по документу. ¬ документе на EV1 записано, что sak у EV1 такой же, как у обыч
            }
            if (Card::sak.GetBit(4))
            {
                if (Card::sak.GetBit(0))
                {
                    return Set(Card::sak.GetBit(7) ? MFC_EV1 : MFC, CardSize::_2k);              // Mifare Classic 2k
                }
                else                // Card::sak.GetBit(0)
                {
                    if (!Card::sak.GetBit(5))
                    {
                        if (Card::_14443::RATS())
                        {
                            if (DetectSL0())
                            {
                                return true;
                            }

                            if (Card::_14443::T_CL::DetectCetVersion(Card::sak.GetBit(5)))
                            {
                                return true;
                            }
                            else
                            {
                                if (Card::ats.HistoricalBytes_MFP_S())          return Set(MFP_S, CardSize::_4k, 1);
                                else if (Card::ats.HistoricalBytes_MFP_X())     return Set(MFP_X, CardSize::_4k, 1);
                            }
                        }
                        else        // !Card::_14443::RATS()
                        {
                            return Set(Card::sak.GetBit(7) ? MFC_EV1 : MFC, CardSize::_4k);          // Mifare Classic 4k
                        }
                    }
                }
            }
            else                    // !Card::sak.GetBit(4)
            {
                LOG_WRITE(" ");
                if (!Card::sak.GetBit(0) && !Card::sak.GetBit(5))
                {
                    LOG_WRITE(" ");
                    if (Card::_14443::RATS())
                    {
                        LOG_WRITE(" ");
                        if (DetectSL0())
                        {
                            return true;
                        }

                        if (Card::_14443::T_CL::DetectCetVersion(Card::sak.GetBit(5)))
                        {
                            return true;
                        }
                        else
                        {
                            LOG_WRITE(" ");
                            if (Card::ats.HistoricalBytes_MFP_S())          return Set(MFP_S, CardSize::_2k, 1);
                            else if (Card::ats.HistoricalBytes_MFP_X())     return Set(MFP_X, CardSize::_2k, 1);
                            else if (Card::ats.HistoricalBytes_MFP_SE())
                            {
                                LOG_WRITE(" ");
                                return Set(MFP_SE, CardSize::_1k, 1);
                            }
                        }
                    }
                    else        // !Card::_14443::RATS()
                    {
                        return Set(Card::sak.GetBit(7) ? MFC_EV1 : MFC, CardSize::_1k);               // Mifare Classic 1k
                    }
                }
            }
        }
        else                    // !Card::sak.GetBit(3)
        {
            if (!Card::sak.GetBit(4) && !Card::sak.GetBit(0) && Card::sak.GetBit(5))
            {
                Card::_14443::RATS();

                if (DetectSL0())
                {
                    return true;
                }

                if (Card::_14443::T_CL::DetectCetVersion(Card::sak.GetBit(5)))
                {
                    return true;
                }
                else
                {
                    if (Card::ats.HistoricalBytes_MFP_S())
                    {
                        if      (_GET_BIT(Card::atqa, 2))       return Set(MFP_S, CardSize::_2k, 3);    // Mifare Plus S 2k
                        else if (_GET_BIT(Card::atqa, 1))       return Set(MFP_S, CardSize::_4k, 3);    // Mifare Plus S 4k
                    }
                    else if (Card::ats.HistoricalBytes_MFP_X())
                    {
                        if      (_GET_BIT(Card::atqa, 2))       return Set(MFP_X, CardSize::_2k, 3);    // Mifare Plus X 2k
                        else if (_GET_BIT(Card::atqa, 1))       return Set(MFP_X, CardSize::_4k, 3);    // Mifare Plus X 4k
                    }
                    else if (Card::ats.HistoricalBytes_MFP_SE())
                    {
                        if (_GET_BIT(Card::atqa, 2))            return Set(MFP_SE, CardSize::_1k, 3);   // Mifare Plus SE 1k
                    }
                }
            }
        }
    }

    return false;
}


bool Card::_14443::T_CL::DetectCetVersion(bool sak_5_is_true)
{
    uint8 ver[32];
    int bytes_received = 0;

    if (Card::_14443::T_CL::GetVersion(ver, &bytes_received))
    {
        if (bytes_received > 5)
        {
            CardSize::E size = CardSize::_1k;

            if      (ver[5] == 0x16) size = CardSize::_2k;
            else if (ver[5] == 0x18) size = CardSize::_4k;
            else return false;

            if (sak_5_is_true)
            {
                if      (ver[3] == 0x11) return Set(TypeCard::MFP_EV1, size, 3);
                else if (ver[3] == 0x22) return Set(TypeCard::MFP_EV2, size, 3);
            }
            else
            {
                return Set(TypeCard::MFP_EV1, size, 1);
            }
        }
    }

    return false;
}


void TypeCard::Reset()
{
    current = Unidentified;
}


TypeCard::E TypeCard::Current()
{
    return current;
}


bool TypeCard::IsNTAG()
{
    return (Card::atqa == 0x44) && (Card::sak.Raw() == 0x00);
}


bool TypeCard::IsMifare()
{
    return !IsNTAG();
}


int TypeCard::NTAG::NumberBlockCFG1()
{
    static const int numbers[Count] =
    {
        0,
        42,     // NTAG213
        132,    // NTAG215
        228,    // NTAG216

        // ¬ следующих картах блока CFG1 нету, поэтому просто читаем 4-й блок
        // 4-й блок - это первый блок NTAG, доступный дл€ изменени€

        4,      // MFC
        4,      // MFC EV1
        4,      // MFP S
        4,      // MFP X
        4,      // MFP SE
        4,      // MFP EV1
        4,      // MFP EV2
        4       // MFP
    };

    return numbers[Current()];
}
