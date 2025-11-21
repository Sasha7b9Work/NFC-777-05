// 2023/09/03 15:58:08 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Hardware/HAL/HAL.h"
#include "Reader/Reader.h"
#include "Reader/Events.h"
#include "Reader/Signals.h"
#include "Modules/Indicator/Indicator.h"
#include "Communicator/OSDP.h"
#include "Reader/RC663/Commands.h"
#include "Settings/Settings.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/Cards/CardNTAG.h"
#include "Reader/RC663/Registers.h"
#include "Reader/RC663/RC663_def.h"
#include "Hardware/Timer.h"
#include "Settings/Settings.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"
#include "Settings/SET2.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace Card
{
    using RC663::Register;

    // Результат предыдущей аутентификации. Смысл в том, что если предыдущая авторизация прошла
    // неудачно, то нужно давать сигнал, если текущая авторизация успешна
    static bool     prev_auth_bool = false;
    static bool     inserted = false;
    UID             uid;
    uint16          atqa = 0;
    SAK             sak;
    ATS             ats;
    CardSize        size_card{ CardSize::Undefined };
    int             SL = -1;
    int             number_block_T_CL = 0;
    bool            null_bytes_received = false;
    static Block16  password_auth(0, 0);                // Этим паролем работаем с картой. Он сделан приватным,
                                                        // потому что подменяется в определённых случаях
    static TypeAuth prev_auth;
}


void Card::SetPasswordAuth(const Block16 &password)
{
    password_auth = password;

    if (SL == 0 || SL == 1)
    {
        password_auth = SettingsMaster::PSWD::default_Mifare;
    }
}


Block16 Card::GetPasswordAuth()
{
    return password_auth;
}


void Card::InsertExtendedMode(const TypeAuth &type, bool auth_ok, bool new_auth, uint64 number)
{
    Timer::Delay(100);      // \todo Без этой задержки зависает при определинии карты в CIC

    if (prev_auth != type || !inserted || new_auth)
    {
        inserted = true;

        prev_auth = type;

        if (auth_ok)
        {
            Event::CardReadOK(uid, number);
        }
        else
        {
            Event::CardReadFAIL(uid);
        }

        auth_ok ? Signals::GoodUserCard(false) : Signals::BadUserCard(false);
    }
}


void Card::InsertNormalModeUser(uint64 number, bool auth_ok)
{
    if (!IsInserted() || (auth_ok && !prev_auth_bool))
    {
        if (ModeOffline::IsEnabled())
        {
            if (auth_ok)
            {
                if(SET2::AccessCards::Granted(number))
                {
                    Signals::GoodUserCard(true);

                    ModeOffline::OpenTheLock();
                }
                else
                {
                    Signals::BadUserCard(true);
                }
            }
        }
        else if (ModeWG::IsNumber())
        {
            if (auth_ok)
            {
                OSDP::Card::Insert(number);

                if (ModeReader::IsWG())
                {
                    Block8 bs = { number };

                    HAL_USART::WG::Transmit(bs.bytes[2], bs.bytes[1], bs.bytes[0]);
                }
                else
                {
                    HAL_USART::UART::SendStringF("#CARD USER %s NUMBER %llu", uid.ToASCII().c_str(), number);
                }
            }

            prev_auth_bool = auth_ok;
        }
        else
        {
            if (gset.IsWiegandFullGUID())
            {
                HAL_USART::WG::Transmit(uid.GetRaw());
            }
            else
            {
                uint guid = uid.Get3BytesForm();

                HAL_USART::WG::Transmit((uint8)guid, (uint8)(guid >> 8), (uint8)(guid >> 16));
            }
        }

        inserted = true;
    }
}


void Card::InsertNormalModeMaster(bool auth_ok)
{
    if (!IsInserted())
    {
        inserted = true;

        if (auth_ok)
        {
            HAL_USART::UART::SendString("#CARD MASTER");
        }
    }
}


void Card::Eject(bool generate_event_UARAT)
{
    atqa = 0;
    sak.Reset();
    ats.Clear();
    size_card.value = CardSize::Undefined;
    SL = -1;
    number_block_T_CL = 0;

    TypeCard::Reset();

    if (IsInserted())
    {
        inserted = false;

        if (generate_event_UARAT)
        {
            Event::CardRemove();
        }
    }
}


bool Card::IsInserted()
{
    return inserted;
}


uint ConvertorUID::Get() const
{
    uint8 uc7ByteUID[7] =
    {
        (uint8)(uid >> 48),
        (uint8)(uid >> 40),
        (uint8)(uid >> 32),
        (uint8)(uid >> 24),
        (uint8)(uid >> 16),
        (uint8)(uid >> 8),
        (uint8)uid
    };


    Block4 nuid;

    Convert7ByteUIDTo4ByteNUID(uc7ByteUID, &nuid.bytes[0]);

    Block4 result;
    result.bytes[0] = nuid.bytes[2];
    result.bytes[1] = nuid.bytes[1];
    result.bytes[2] = nuid.bytes[0];
    result.bytes[3] = nuid.bytes[3];

    return result.u32;
};


void ConvertorUID::Convert7ByteUIDTo4ByteNUID(uint8 *uc7ByteUID, uint8 *uc4ByteUID) const
{
    uint16 CRCPreset = 0x6363;
    uint16 CRCCalculated = 0x0000;

    ComputeCrc(CRCPreset, uc7ByteUID, 3, CRCCalculated);

    uc4ByteUID[0] = (uint8)((CRCCalculated >> 8) & 0xFF);   //MSB
    uc4ByteUID[1] = (uint8)(CRCCalculated & 0xFF);          //LSB
    CRCPreset = CRCCalculated;

    ComputeCrc(CRCPreset, uc7ByteUID + 3, 4, CRCCalculated);

    uc4ByteUID[2] = (uint8)((CRCCalculated >> 8) & 0xFF);   //MSB
    uc4ByteUID[3] = (uint8)(CRCCalculated & 0xFF);          //LSB
    uc4ByteUID[0] = (uint8)(uc4ByteUID[0] | 0x0F);
    uc4ByteUID[0] = (uint8)(uc4ByteUID[0] & 0xEF);
}


uint16 ConvertorUID::UpdateCrc(uint8 ch, uint16 *lpwCrc) const
{
    ch = (uint8)(ch ^ (uint8)((*lpwCrc) & 0x00FF));
    ch = (uint8)(ch ^ (ch << 4));
    *lpwCrc = (uint16)((*lpwCrc >> 8) ^ ((uint16)ch << 8) ^ ((uint16)ch << 3) ^ ((uint16)ch >> 4));

    return(*lpwCrc);
}


void ConvertorUID::ComputeCrc(uint16 wCrcPreset, uint8 *Data, int Length, uint16 &usCRC) const
{
    uint8 chBlock;

    do
    {
        chBlock = *Data++;
        UpdateCrc(chBlock, &wCrcPreset);
    } while (--Length);

    usCRC = wCrcPreset;
}


bool Card::WriteKeyPassword(const Block16 &password)
{
    if (TypeCard::IsNTAG())
    {
        static const uint8 numberBlockPassword[TypeCard::MFC] =     // MFC здесь используется лишь как ограничитель. Всё, что ниже - NTAG
        {
            0,
            43,     // NTAG213
            133,    // NTAG215
            229     // NTAG216
        };

        uint8 nbr_block = numberBlockPassword[TypeCard::Current()];

        // Записываем напрямую, без проверки правильности, потому что из этих блоков, куда записываем пароль, считываются всегда нули
        return CardNTAG::WriteBlock(nbr_block, password.bytes) && CardNTAG::WriteBlock(nbr_block + 1, password.bytes + 4);
    }
    else if (TypeCard::IsMifare())
    {
        if (TypeCard::IsSupportT_CL())
        {
            for (int sector = 0; sector < 4; sector++)
            {
                int nbr_key = 0x4000 + sector * 2;

                if (!_14443::T_CL::ChangeKeyAES(nbr_key, Card::GetPasswordAuth(), password))
                {
                    return false;
                }
            }

            return true;
        }
        else
        {
            for (int sector = 0; sector < 4; sector++)          // Пишем только в четыре сектора - где записаны наши данные
            {
                Block16 block(0, 0);

                int nbr_block = sector * 4 + 3;

                if (CardMF::ReadBlock(nbr_block, block))
                {
                    std::memcpy(block.bytes, &password, 6);     //-V1086 Копируем только 6 байт пароля

                    if (!CardMF::WriteBlock(nbr_block, block))
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}


bool Card::EnableCheckPassword()
{
    bool result = false;

    if (TypeCard::IsNTAG())
    {
        static const uint8 numberBlockCFG0[TypeCard::MFC] =     // MFC здесь используется лишь в качестве ограничителя. Всё, что ниже - NTAG
        {
            0,
            41,     // NTAG213
            131,    // NTAG215
            227     // NTAG216
        };

        Block4 block;

        if (Card::HI_LVL::ReadBlock(numberBlockCFG0[TypeCard::Current()], block))
        {
            block.bytes[3] = 0x04;             // Защищаем доступ ко всем блокам, начиная с 4-го

            if (Card::HI_LVL::WriteBlock(numberBlockCFG0[TypeCard::Current()], block.bytes))
            {
                if (Card::HI_LVL::ReadBlock(TypeCard::NTAG::NumberBlockCFG1(), block))
                {
                    block.bytes[0] = 0x80;     // Защита от чтения и от записи

                    if (Card::HI_LVL::WriteBlock(TypeCard::NTAG::NumberBlockCFG1(), block.bytes))
                    {
                        result = true;
                    }
                }
            }
        }
    }

    if (TypeCard::IsMifare())
    {
        result = true;
    }
 
    return result;
}


bool Card::ReadNumber(uint64 *number)
{
    Block8 bit_set{ 0 };

    bool result = Card::HI_LVL::Read2Blocks(4, &bit_set);

    *number = bit_set.u64;

    return result;
}


bool Card::WriteNumber(uint64 number)
{
    Block8 bit_set{ number };

    return Card::HI_LVL::Write2Blocks(4, bit_set.bytes);
}


bool Card::WriteSettings(const SettingsMaster &set)
{
    return Card::HI_LVL::WriteData(4, &set, set.Size());
}


bool Card::ReadBitmapCards(char bitmap_cards[500])
{
    bitmap_cards[0] = '\0';

    int number_block = SettingsMaster::FIRST_BLOCK_BITMAP_CARDS;

    for (int counter = 0; counter < SettingsMaster::NUMBER_BLOCKS_BITMAP_CARDS; counter++)
    {
        Block4 block;

        if (Card::HI_LVL::ReadBlock(number_block++, block))
        {
            for (uint i = 0; i < 4; i++)
            {
                char byte[32];
                std::sprintf(byte, "%02X", block.bytes[i]);
                std::strcat(bitmap_cards, byte);
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}


bool Card::WriteBitmapCards(const char bitmap_cards[160])
{
    bool success = true;

    uint8 buffer[4];

    pchar pointer = bitmap_cards;

    int number_block = SettingsMaster::FIRST_BLOCK_BITMAP_CARDS;    // Номер блока - от 15 до 39
    int number_byte = 0;                                            // Номер байта - от 0 до 3
    const int last_block = SettingsMaster::FIRST_BLOCK_BITMAP_CARDS + SettingsMaster::NUMBER_BLOCKS_BITMAP_CARDS - 1;

    while (*pointer)
    {
        char data[32];

        char byte1 = *pointer++;
        char byte2 = *pointer++;

        std::sprintf(data, "%c%c", byte1, byte2);

        buffer[number_byte++] = (uint8)std::strtoull(data, nullptr, 16);

        if (number_byte == 4)
        {
            if (number_block > last_block)
            {
                return success;
            }

            if (!Card::HI_LVL::WriteBlock(number_block++, buffer))
            {
                success = false;
            }

            number_byte = 0;
        }
    }

    if (number_byte != 0)
    {
        while (number_byte < 4)
        {
            buffer[number_byte++] = 0x00;
        }

        if (number_block > last_block)
        {
            return success;
        }

        if (!Card::HI_LVL::WriteBlock(number_block++, buffer))
        {
            success = false;
        }

        number_byte = 0;
    }

    std::memset(buffer, 0, 4);

    while (number_block <= last_block)
    {
        if (!Card::HI_LVL::WriteBlock(number_block++, buffer))
        {
            success = false;
        }
    }

    return success;
}


uint64 UID::GetRaw() const
{
    return uid;
}


String<> UID::ToString(bool full) const
{
    if (!calculated)
    {
        return String<>("NULL");
    }
    else
    {
        if (Is7BytesUID())
        {
            if (full)
            {
                return String<>("%06X%08X", (uint)(uid >> 32), (uint)(uid));
            }
            else
            {
                return String<>("%06X", Get3BytesForm());
            }
        }
        else
        {
            if (full)
            {
                return String<>("%08X", (uint)uid);
            }
            else
            {
                return String<>("%06X", Get3BytesForm());
            }
        }
    }
}


String<> UID::ToASCII() const
{
    return String<>("%s*%s", ToString(true).c_str(), ToString(false).c_str());
}


uint UID::Get3BytesForm() const
{
    if (!calculated)
    {
        return 0;
    }

    if (Is7BytesUID())
    {
        return ConvertorUID(uid).Get() & 0xFFFFFF;
    }

    if (gset.IsWiegnadDiscard_NUID_LSB())                   // Это настройка работает только для 4-байтных UID
    {
        return (((uint)uid) >> 8) & 0xFFFFFF;
    }
    else
    {
        return ((uint)uid) & 0xFFFFFF;
    }
}


void UID::Clear()
{
    std::memset(bytes, 0, 10);
    uid = 0;
    calculated = false;
}

void UID::Calculate()
{
    calculated = false;

    uid = 0;

    if (bytes[0] != 0)
    {
        if (Is7BytesUID())
        {
            uid = ((uint64)bytes[8]);
            uid |= (((uint64)bytes[7]) << 8);
            uid |= (((uint64)bytes[6]) << 16);
            uid |= (((uint64)bytes[5]) << 24);
            uid |= (((uint64)bytes[3]) << 32);
            uid |= (((uint64)bytes[2]) << 40);
            uid |= (((uint64)bytes[1]) << 48);
        }
        else
        {
            uid = ((uint64)bytes[3]);
            uid |= (((uint64)bytes[2]) << 8);
            uid |= (((uint64)bytes[1]) << 16);
            uid |= (((uint64)bytes[0]) << 24);
        }

        calculated = true;
    }
}


uint8 *UID::GetSerialNumber(uint8 sn[4]) const
{
    if (Is7BytesUID())
    {
        sn[0] = bytes[5];
        sn[1] = bytes[6];
        sn[2] = bytes[7];
        sn[3] = bytes[8];
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            sn[i] = bytes[i];
        }
    }

    return sn;
}


void ATS::Set(uint8 *_bytes, int _length)
{
    Clear();

    length = _length;

    std::memcpy(b, _bytes, (uint)length);
}


bool ATS::IsCorrect() const
{
    if (length > 0)
    {
        return (length == b[0]);
    }

    return false;
}


void ATS::Clear()
{
    std::memset(b, 0, SIZE);
}


bool Card::PasswordAuth()
{
    if (TypeCard::IsNTAG())
    {
        return CardNTAG::PasswordAuth();
    }
    else if (TypeCard::IsMifare())
    {
        uint64 number = 0;

        return Card::ReadNumber(&number);
    }
    return false;
}


String<128> Card::BuildCardInfoFull()
{
    String<128> info("UID=%s TYPE=%s", uid.ToASCII().c_str(), Card::BuildCardInfoShort().c_str());

    if (TypeCard::IsMifare())
    {
        info.Append(String<32>(" SIZE=%s", size_card.ToASCII()).c_str());
    }

    info.Append(String<64>(" ATQA=%04X SAK=%02X", Card::atqa, Card::sak.Raw()).c_str());

    if (TypeCard::IsSupportT_CL())
    {
        info.Append(String<100>(" ATS=%s SL=%d", ats.ToASCII().c_str(), SL).c_str());
    }

    return info;
}


String<64> Card::BuildCardInfoShort()
{
    String<64> info(TypeCard::ToLongASCII());

    if (TypeCard::IsMifare())
    {
        info.Append(String<16>("_%s", Card::size_card.ToASCII()).c_str());

        if (TypeCard::IsSupportT_CL())
        {
            info.Append(String<16>("_SL%d", Card::SL).c_str());
        }
    }

    return info;
}


String<65> ATS::ToASCII() const
{
    String<65> result("");

    char *out = result.c_str();

    for (int i = 0; i < length; i++)
    {
        std::sprintf(out, "%02X", b[i]);
        out += 2;
    }

    return result;
}


uint64 ATS::HistoricalBytes() const
{
    Block8 bs;

    for (int i = 0; i < 7; i++)
    {
        bs.bytes[i] = b[length - 1 - i];
    }

    return bs.u64;
}


bool ATS::HistoricalBytes_MFP_S() const
{
    return HistoricalBytes() == 0xC1052F2F0035C7;
}


bool ATS::HistoricalBytes_MFP_X() const
{
    return HistoricalBytes() == 0xC1052F2F01BCD6;
}


bool ATS::HistoricalBytes_MFP_SE() const
{
    return
        HistoricalBytes() == 0xC105213000F6D1 ||
        HistoricalBytes() == 0xC105213010F6D1 ||
        HistoricalBytes() == 0xC10521300077C1;
}


pchar CardSize::ToASCII() const
{
    static const pchar strings[Count] =
    {
        "Undefined",
        "1K",
        "2K",
        "4k"
    };

    return strings[value];
}


void Block16::FillBytes(uint8 data[16])
{
    std::memcpy(bytes, data, 16);
}


char *Block16::ToReadableASCII(char out[36]) const
{
    std::sprintf(out, "%08X.%08X.%08X.%08X", u32[3], u32[2], u32[1], u32[0]);

    return out;
}


char *Block16::ToRawASCII(char out[33]) const
{
    std::sprintf(out, "%08X%08X%08X%08X", u32[3], u32[2], u32[1], u32[0]);

    return out;
}


void Block16::FillPseudoRandom()
{
    uint8 value = (uint8)std::rand();

    for (int i = 0; i < 16; i++)
    {
        bytes[i] = (uint8)(value++);
    }
}
