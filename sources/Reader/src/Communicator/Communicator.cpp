// 2023/08/29 19:55:32 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Communicator/Communicator.h"
#include "Hardware/HAL/HAL.h"
#include "Modules/LIS2DH12/LIS2DH12.h"
#include "Utils/StringUtils.h"
#include "Reader/RC663/RC663.h"
#include "Reader/Reader.h"
#include "Hardware/Timer.h"
#include "Modules/Memory/Memory.h"
#include "Modules/Indicator/Indicator.h"
#include "Reader/Events.h"
#include "Modules/Player/Player.h"
#include "Device/Device.h"
#include "Modules/Player/TableSounds.h"
#include "Utils/Math.h"
#include "Utils/StringUtils.h"
#include "Hardware/Power.h"
#include "Reader/RC663/RC663.h"
#include "Reader/Cards/CardTasks.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"
#include "Communicator/ROMWriter.h"
#include "Settings/SET2.h"
#include "Modules/Indicator/Scripts/ScriptsLED.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>



/*

    #KEYSMFSL0 WRITE CARD KEY0 KEY1 KEY2 KEY3 KEY4 CRC32=XXXX
    #KEYSMFSL0 WRITE READER KEY0 KEY1 KEY2 KEY3 KEY4 CRC32=XXXX
    #KEYSMFSL0 WRITE [OK/FAIL]

*/


namespace Communicator
{
    // Нормальная работа
    static void UpdateNormal(BufferUSART &);

    // Закачка прошивки
    static void FuncUpdateFirmware(BufferUSART &);

    typedef void (*func)(BufferUSART &);

    static func funcUpdate = UpdateNormal;

    namespace FuncUpdate
    {
        static void Set(func _func)
        {
            funcUpdate = _func;
        }

        static void Restore()
        {
            funcUpdate = UpdateNormal;
        }
    }

    static bool Com_AUTH_REQ(BufferUSART &);
    static bool Com_AUTH(BufferUSART &);          // Команда действует в режимах Read и Write
    static bool Com_CARD(BufferUSART &);
    static bool Com_CONFIG(BufferUSART &);
    static bool Com_ERASE(BufferUSART &);
    static bool Com_ID(BufferUSART &);
    static bool Com_INFO(BufferUSART &);
    static bool Com_KEYSMFSL0(BufferUSART &);
    static bool Com_LED(BufferUSART &);
    static bool Com_MAKE(BufferUSART &);
    static bool Com_MODE_REQ(BufferUSART &);
    static bool Com_MODE(BufferUSART &);
    static bool Com_MSR(BufferUSART &);
    static bool Com_PASSWORD(BufferUSART &);     // Команда делает то же, что и мастер-карта
    static bool Com_READER(BufferUSART &);
    static bool Com_RESET(BufferUSART &);
    static bool Com_SOUND(BufferUSART &);
    static bool Com_TEMP(BufferUSART &);
    static bool Com_TEMP_REQ(BufferUSART &);
    static bool Com_UPDATE(BufferUSART &);
    static bool Com_VER(BufferUSART &);

    typedef bool (*funcProcess)(BufferUSART &);

    struct Elem
    {
        Elem(pchar _key, funcProcess _func, uint8 vWG, uint8 vUART, uint8 vRead, uint8 vWrite) :
            key(_key), func(_func)
        {
            is_valid[ModeReader::WG] = vWG;
            is_valid[ModeReader::UART] = vUART;
            is_valid[ModeReader::Read] = vRead;
            is_valid[ModeReader::Write] = vWrite;
        }
        pchar       key;
        funcProcess func;
        uint8       is_valid[ModeReader::Count];  // 0 означает, что в данном режиме команда не обрабатывается

        bool NeedExecute() const
        {
            return  (is_valid[ModeReader::Current()] != 0);
        }
    };

    static const Elem handlers[] =
    {  //                                  WG UART Rd Wr
        Elem("#AUTH?",     Com_AUTH_REQ,   0, 0,   1, 1),
        Elem("#AUTH",      Com_AUTH,       0, 0,   1, 1),
        Elem("#CONFIG",    Com_CONFIG,     1, 1,   1, 1),
        Elem("#CARD",      Com_CARD,       1, 1,   1, 1),
        Elem("#ERASE",     Com_ERASE,      0, 1,   0, 1),
        Elem("#ID",        Com_ID,         1, 1,   1, 1),
        Elem("#INFO",      Com_INFO,       1, 1,   1, 1),
        Elem("#KEYSMFSL0", Com_KEYSMFSL0,  0, 0,   0, 1),
        Elem("#LED",       Com_LED,        1, 1,   1, 1),
        Elem("#MAKE",      Com_MAKE,       0, 0,   0, 1),
        Elem("#MODE?",     Com_MODE_REQ,   1, 1,   1, 1),
        Elem("#MODE",      Com_MODE,       1, 1,   1, 1),
        Elem("#MSR",       Com_MSR,        1, 1,   1, 1),
        Elem("#PASSWORD",  Com_PASSWORD,   1, 1,   1, 1),
        Elem("#READER",    Com_READER,     1, 1,   1, 1),
        Elem("#RESET",     Com_RESET,      1, 1,   1, 1),
        Elem("#SOUND",     Com_SOUND,      1, 1,   1, 1),
        Elem("#TEMP?",     Com_TEMP_REQ,   1, 1,   1, 1),
        Elem("#TEMP",      Com_TEMP,       1, 1,   1, 1),
        Elem("#UPDATE",    Com_UPDATE,     1, 1,   1, 1),
        Elem("#VER",       Com_VER,        1, 1,   1, 1),
        Elem("",           nullptr,        1, 0,   0, 0)
    };
}


void Communicator::Update(BufferUSART &buffer)
{
    funcUpdate(buffer);
}


void Communicator::UpdateNormal(BufferUSART &buffer)
{
    if (buffer.Size())
    {
        while (true)
        {
            int pos_end = buffer.ExtractCommand();

            if (pos_end >= 0)
            {
                buffer[pos_end] = '\0';

                const Elem *elem = &handlers[0];

                bool processed = false;

                do
                {
                    uint length = std::strlen(elem->key);

                    if (std::memcmp(elem->key, buffer.DataChar(), length) == 0)
                    {
                        char symbol = buffer.DataChar()[length];

                        if (symbol == '\0' ||           // Если командная строка состоит из одного слова
                            symbol == ' ')              // Или за ней идут параметры после пробела
                        {
                            processed = elem->NeedExecute() ? elem->func(buffer) : true;

                            break;
                        }
                    }

                    elem++;

                } while (elem->key[0]);

                if (!processed)
                {
                    HAL_USART::UART::SendStringF("#ERROR command \"%s\"", (const char *)buffer.Data());
                }

                buffer.RemoveFirst(pos_end + 1);
            }
            else
            {
                break;
            }
        }
    }
}


bool Communicator::Com_AUTH(BufferUSART &buffer)
{
    if (buffer.WordIs(2, "DISABLE"))
    {
        Reader::SetAuth(TypeAuth(false, 0));

        HAL_USART::UART::SendOK();

        return true;
    }
    else
    {
        uint64 pass = 0;

        if (buffer.GetUInt64(2, &pass, 10))
        {
            Reader::SetAuth(TypeAuth(true, pass));

            HAL_USART::UART::SendOK();

            return true;
        }
    }

    return false;
}


bool Communicator::Com_AUTH_REQ(BufferUSART &)
{
    TypeAuth type = Reader::GetAuth();

    if (type.enabled)
    {
        HAL_USART::UART::SendStringF("%llu", type.password);
    }
    else
    {
        HAL_USART::UART::SendString("DISABLED");
    }

    return true;
}


bool Communicator::Com_CARD(BufferUSART &buffer)
{
    int num_words = buffer.GetCountWords();

    if (num_words == 2)
    {
        ParseBuffer word;

        buffer.GetWord(2, word);

        if (word.IsEqual("INFO?"))
        {
            if (Card::IsInserted())
            {
                HAL_USART::UART::SendStringF("#CARD INFO %s", Card::BuildCardInfoFull().c_str());
            }
            else
            {
                HAL_USART::UART::SendString("#CARD NOT DETECTED");
            }

            return true;
        }
    }

    return false;
}


bool Communicator::Com_KEYSMFSL0(BufferUSART &buffer)
{
    bool result = false;

    if (buffer.Crc32IsMatches() && buffer.GetCountWords() == 9)
    {
        ParseBuffer word;

        buffer.GetWord(2, word);

        if (!word.IsEqual("WRITE"))
        {
            return false;
        }

        Block16 keys[SET2::KeysMFP::NUM_KEYS] =
        {
            Block16(0, 0),
            Block16(0, 0),
            Block16(0, 0),
            Block16(0, 0),
            Block16(0, 0)
        };

        for (int i = 0; i < SET2::KeysMFP::NUM_KEYS; i++)
        {
            if (!buffer.GetUInt128hex(i + 4, &keys[i]))
            {
                return false;
            }
        }

        buffer.GetWord(3, word);

        if (word.IsEqual("CARD"))
        {
            Task::WriteKeysCardMFSL0(keys);

            result = true;
        }
        else if (word.IsEqual("READER"))
        {
            SET2::KeysMFP::WriteToROM(keys);

            result = true;

            HAL_USART::UART::SendString("#KEYSMFSL0 WRITE OK");
        }
    }

    return result;
}


bool Communicator::Com_MAKE(BufferUSART &buffer)
{
    bool result = false;

    uint64 new_pass = 0;

    int num_words = buffer.GetCountWords();

    ParseBuffer word;

    buffer.GetWord(2, word);

    if (word.IsEqual("MASTER"))                              // Делаем мастер-карту
    {
        if (num_words == 5)                             // только с паролем
        {
            uint64 old_pass = 0;

            if (buffer.GetParamUInt64dec("FPWD", &old_pass) && buffer.GetParamUInt64dec("NPWD", &new_pass) && buffer.Crc32IsMatches())
            {
                Task::CreateMasterPasswords(old_pass, new_pass);

                result = true;
            }
        }
        else                                            // полную
        {
            SettingsMaster set;

            if (buffer.ReadSettings(set))
            {
                char bits[512];

                Task::CreateMasterFull(set, SU::ExtractBitMapCards(buffer.DataChar(), bits));

                result = true;
            }
        }
    }
    else if (word.IsEqual("USER"))                       // Делаем пользовательскую карту
    {
        if (num_words == 5)                             // полную
        {
            uint64 number = 0;

            if (buffer.GetParamUInt64dec("PWD", &new_pass) && buffer.GetParamUInt64dec("NUMBER", &number) && buffer.Crc32IsMatches())
            {
                Task::CreateUserFull(new_pass, number);

                result = true;
            }
        }
        else if (num_words == 4)                            // только пароль
        {
            if (buffer.GetParamUInt64dec("PWD", &new_pass) && buffer.Crc32IsMatches())
            {
                Task::CreateUserPassword(new_pass);

                result = true;
            }
        }
    }

    return result;
}


bool BufferUSART::ReadSettings(SettingsMaster &set) const
{
    uint64 old_pass = 0;
    uint64 new_pass = 0;
    uint wiegand = 0;
    uint uint_red = 0;
    uint uint_green = 0;
    int melody0 = 0;
    int melody1 = 0;
    int melody2 = 0;
    int volume0 = 0;
    int volume1 = 0;
    int volume2 = 0;
    int size_number = 0;
    int begin_number = 0;
    int time_lock = 0;
    int time_alarm = 0;
    int security_mode_enabled = 0;
    int offline_mode_allowed = 0;
    int osdp_address = 0;
    int osdp_enabled = 0;
    int lpcd_enabled = 0;
    uint osdp_bautdrate = 0;
    int antibreak_sens = 0;
    uint antibreak_number = 0;
    int only_SL3 = 0;

    if (GetParamUInt64dec("FPWD", &old_pass) &&
        GetParamUInt64dec("NPWD", &new_pass) &&
        GetParamUInt("WG", &wiegand, 16) &&
        GetParamUInt("CRED", &uint_red, 16) &&
        GetParamUInt("CGREEN", &uint_green, 16) &&
        GetParamIntDec("M1", &melody0) &&
        GetParamIntDec("M2", &melody1) &&
        GetParamIntDec("M3", &melody2) &&
        GetParamIntDec("V1", &volume0) &&
        GetParamIntDec("V2", &volume1) &&
        GetParamIntDec("V3", &volume2) &&
        GetParamIntDec("NSIZE", &size_number) &&
        GetParamIntDec("NBEGIN", &begin_number) &&
        GetParamIntDec("TLOCK", &time_lock) &&
        GetParamIntDec("TRUN", &time_alarm) &&
        GetParamIntDec("AMODE", &security_mode_enabled) &&
        GetParamIntDec("OMODE", &offline_mode_allowed) &&
        GetParamIntDec("OSDPA", &osdp_address) &&
        GetParamUInt("OSDPBR", &osdp_bautdrate, 10) &&
        GetParamIntDec("OSDPEN", &osdp_enabled) &&
        GetParamIntDec("ECO", &lpcd_enabled) &&
        GetParamIntDec("SENS", &antibreak_sens) &&
        GetParamUInt("NUMBERSENS", &antibreak_number, 10) &&
        GetParamIntDec("ONLY_SL3", &only_SL3) &&
        Crc32IsMatches())
    {
        set.SetOldPassword(Block16(0, old_pass));
        set.SetPassword(Block16(0, new_pass));
        set.SetWiegand(wiegand);
        set.SetColorRed(Color::FromUint(uint_red));
        set.SetColorGreen(Color::FromUint(uint_green));
        set.SetMelody(TypeSound::Beep, (uint8)melody0);
        set.SetMelody(TypeSound::Green, (uint8)melody1);
        set.SetMelody(TypeSound::Red, (uint8)melody2);
        set.SetVolume(TypeSound::Beep, (uint8)volume0);
        set.SetVolume(TypeSound::Green, (uint8)volume1);
        set.SetVolume(TypeSound::Red, (uint8)volume2);
        set.SetSizeNumber((uint8)size_number);
        set.SetBeginNumber((uint8)begin_number);
        set.SetTimeLock((uint8)time_lock);
        set.SetTimeAlarm((uint8)time_alarm);
        set.SetSecurityModeEnabled(security_mode_enabled != 0);
        set.SetOfflineModeAllowed(offline_mode_allowed != 0);
        set.SetAddressOSDP((uint8)osdp_address);
        set.SetBaudRateOSDP(BaudRate::FromUInt(osdp_bautdrate));
        set.SetAntibreakSens((uint8)antibreak_sens);
        set.SetAntibreakNumber(antibreak_number);
        set.EnableOSDP(osdp_enabled != 0);
        set.SetEnabledLPCD(lpcd_enabled != 0);
        set.SetOnlySL3(only_SL3 != 0);

        return true;
    }

    return false;
}


bool BufferUSART::Crc32IsMatches() const
{
    pchar entry_crc = SU::GetSubString(DataChar(), "CRC32=");

    if (entry_crc == nullptr)
    {
        return false;
    }

    uint crc32 = 0;

    GetParamUInt("CRC32", &crc32, 16);

    return Math::CalculateCRC32(Data(), entry_crc - (pchar)Data() + 6) == crc32;
}


bool Communicator::Com_MODE(BufferUSART &buffer)
{
    bool result = true;

    ParseBuffer word;

    buffer.GetWord(2, word);

    if (word.IsEqual("WIEGAND"))
    {
        ModeReader::Set(ModeReader::WG);
    }
    else if (word.IsEqual("UART"))
    {
        ModeReader::Set(ModeReader::UART);
    }
    else if (word.IsEqual("READ"))
    {
        ModeReader::Set(ModeReader::Read);
    }
    else if (word.IsEqual("WRITE"))
    {
        ModeReader::Set(ModeReader::Write);
    }
    else
    {
        result = false;
    }

    if (result)
    {
        HAL_USART::UART::SendOK();
    }

    return result;
}


bool Communicator::Com_ERASE(BufferUSART &buffer)
{
    if (buffer.WordIs(2, "FULL"))
    {
        Memory::Erase::Full();

        return true;
    }
    else
    {
        int nbr_block = 0;

        if (buffer.GetIntDec(2, &nbr_block) && nbr_block >= 0 && nbr_block < (int)(Memory::Capasity() / Memory::SizeBlock()))
        {
            Memory::Erase::Block4kB(nbr_block);

            HAL_USART::UART::SendOK();

            return true;
        }
    }

    return false;
}


bool Communicator::Com_RESET(BufferUSART &)
{
    SettingsMaster::ResetToFactory();

    HAL_USART::UART::SendOK();

    return true;
}


bool Communicator::Com_MSR(BufferUSART &)
{
    HAL_USART::UART::SendStringF("OK;MEMORY:%s;%3.1fV;%3.1fC",
        Memory::Test::Run() ? "OK" : "FAIL",
        (double)Power::GetVoltage(),
        (double)LIS2DH12::GetTemperature()
    );

    return true;
}


bool Communicator::Com_UPDATE(BufferUSART &buffer)
{
    bool result = true;

    int size = 0;

    if (buffer.GetParamIntDec("SIZE", &size))
    {
        Memory::Erase::Firmware();

        ROMWriter::SetParameters(0, size);

        Indicator::TurnOn(Color(0x008000, 1.0f), 0, true);

        FuncUpdate::Set(FuncUpdateFirmware);
    }
    else
    {
        result = false;
    }

    return result;
}


bool Communicator::Com_SOUND(BufferUSART &buffer)
{
    ParseBuffer word;

    buffer.GetWord(2, word);

    if (word.IsEqual("LOAD"))
    {
        int number = 0;
        int size = 0;

        if (buffer.GetParamIntDec("NUMBER", &number) && buffer.GetParamIntDec("SIZE", &size))
        {
            if (number == 0)                                                        // При получении первого звука стираем всю память
            {                                                                       // Звуки всегда идут по порядку
                Memory::Erase::Sounds();
            }

            Memory::WriteInt((uint)(TableSounds::Begin() + number), size);

            ROMWriter::SetParameters(TableSounds::AddressSound(number), size);

            FuncUpdate::Set(FuncUpdateFirmware);

            return true;
        }
    }
    else if (word.IsEqual("PLAY"))
    {
        int number = 0;
        int volume = 3;

        if (buffer.GetParamIntDec("NUMBER", &number) && buffer.GetParamIntDec("VOLUME", &volume))
        {
            if (number < 0 || number >= NUMBER_SOUDNS)
            {
                HAL_USART::UART::SendString("#ERROR Sound number must be from 0 to 9");
            }
            else if (volume < 0 || volume > 3)
            {
                HAL_USART::UART::SendString("#ERROR Volume must be from 0 to 3");
            }
            else
            {   
                Player::PlayFromMemory((uint8)number, (uint8)volume);

                HAL_USART::UART::SendOK();

                return true;
            }
        }
    }

    return false;
}


bool Communicator::Com_READER(BufferUSART &buffer)
{
    ParseBuffer word;

    buffer.GetWord(2, word);

    if (word.IsEqual("ON"))
    {
        ReaderPowered::On();

        HAL_USART::UART::SendOK();
    }
    else if (word.IsEqual("OFF"))
    {
        ReaderPowered::Off();

        HAL_USART::UART::SendOK();
    }

    return false;
}


bool Communicator::Com_TEMP(BufferUSART &buffer)
{
    int min = -25;
    int max = 60;

    if (buffer.GetIntDec(2, &min) && buffer.GetIntDec(3, &max))
    {
        Memory::Settings::SetTempMin((int8)min);

        Memory::Settings::SetTempMax((int8)max);

        HAL_USART::UART::SendOK();

        return true;
    }

    return false;
}


bool Communicator::Com_TEMP_REQ(BufferUSART &)
{
    HAL_USART::UART::SendStringF("Temperature range is %d...%d",
        Memory::Settings::GetTempMin(),
        Memory::Settings::GetTempMax());

    return true;
}


bool Communicator::Com_INFO(BufferUSART &)
{
    HAL_USART::UART::SendStringF(
        "HW=%s\n"           // Идентификатор типа изделия
        "SN=%s\n"           // Серийный номер    boot:200h, 4 байта
        "VH=%s\n"           // Аппаратная версия boot:206h, 2 байта
        "VB=%s\n"           // Версия загрузчика boot:204h, 2 байта
        "VS=%s\n"           // Версия программы  firm:204h, 2 байта
        "BUILD=%d\n"        // Версия сборки
        "DT=%s\n"           // Дата производства boot:210h, 4 байта - количество секунд
        "INFO=%s\n"         // Текстовое название программы firm:214h - адрес строки
        "TIME BUILD=%s\n"
        "#END",
        Device::Info::Firmware::TypeID_ASCII_HEX().c_str(),
        Device::Info::Hardware::SerialNumber_ASCII_HEX().c_str(),
        Device::Info::Hardware::Version_ASCII_HEX().c_str(),
        Device::Info::Loader::Version_ASCII_HEX().c_str(),
        Device::Info::Firmware::VersionMajorMinor_ASCII().c_str(),
        Device::Info::Firmware::VersionBuild(),
        Device::Info::Hardware::DateManufactured().c_str(),
        Device::Info::Firmware::TextInfo().c_str(),
        DATE_BUILD
    );

    return true;
}


bool Communicator::Com_ID(BufferUSART &)
{
    HAL_USART::UART::SendString(Device::Info::Hardware::SerialNumber_ASCII_HEX().c_str());

    return true;
}


bool Communicator::Com_CONFIG(BufferUSART &)
{
    WriteConfitToUSART();

    return true;
}


void Communicator::WriteConfitToUSART()
{
    char buffer[320];

    std::sprintf(buffer,
        "VER=%s\n"
        "BUILD=%d\n"
        "TIME BUILD=%s\n"
        "PASSWORD=%llu\n"
        "RED=%08X\n"
        "GREEN=%08X\n"
        "WIEGAND=%08X\n"
        "MELODY BEEP=%d/%d\n"
        "MELODY GREEN=%d/%d\n"
        "MELODY RED=%d/%d\n"
        "NUMBER SIZE=%d\n",
        VERSION_SCPI,
        VERSION_BUILD,
        DATE_BUILD,
        SettingsMaster::PSWD::Get().u64[0],
        gset.ColorRed().value,
        gset.ColorGreen().value,
        gset.GetWiegandFullSettings(),
        gset.Melody(TypeSound::Beep) + 1, gset.Volume(TypeSound::Beep),
        gset.Melody(TypeSound::Green) + 1, gset.Volume(TypeSound::Green),
        gset.Melody(TypeSound::Red) + 1, gset.Volume(TypeSound::Red),
        gset.SizeNumber()
    );

    HAL_USART::UART::SendStringRAW(buffer);

    HAL_USART::UART::SendStringF(
        "NUMBER BEGIN=%d\n"
        "TIME LOCK=%d\n"
        "TIME ALARM=%d\n"
        "OFFLINE MODE=%d\n"
        "SECURITY MODE=%d\n"
        "OSDP ADDRESS=%d\n"
        "OSDP BAUDRATE=%d\n"
        "OSDP ENABLED=%d\n"
        "ANTIBREAK SENS=%d\n"
        "ANTIBREAK NUMBER=%d\n"
        "ECO=%d\n"
        "#END",
        gset.BeginNumber(),
        gset.TimeLock() / 1000,
        gset.TimeAlarm() / 1000,
        gset.IsOfflineModeAldowed() ? 1 : 0,
        gset.IsSecurityModeEnabled() ? 1 : 0,
        gset.AddressOSDP(),
        gset.BaudRateOSDP().ToRAW(),
        gset.IsEnabledOSDP() ? 1 : 0,
        gset.GetAntibreakSensRAW(),
        gset.GetAntibreakNumber(),
        (gset.IsEnabledLPCD() ? 1 : 0)

    );
}


void Communicator::FuncUpdateFirmware(BufferUSART &buffer)
{
    ROMWriter::Update(buffer);

    if (ROMWriter::IsCompleted())
    {
        FuncUpdate::Restore();
    }
}


bool Communicator::Com_VER(BufferUSART &)
{
    HAL_USART::UART::SendString(VERSION_SCPI);

    return true;
}


bool Communicator::Com_PASSWORD(BufferUSART &buffer)
{
    if (buffer.GetCountWords() == 2)
    {
        uint64 pass = 0;

        if (buffer.GetUInt64(2, &pass, 10))
        {
            SettingsMaster::PSWD::Set(Block16(0, pass));

            HAL_USART::UART::SendOK();

            return true;
        }
    }

    return false;
}


bool Communicator::Com_LED(BufferUSART &buffer)
{
    struct TypeColor
    {
        enum E
        {
            _None,
            Red,
            Green
        };
    };

    static TypeColor::E type = TypeColor::_None;

    ParseBuffer word2;

    buffer.GetWord(2, word2);

    if (word2.IsEqual("ON"))
    {
        if (buffer.GetCountWords() == 3)
        {
            ParseBuffer value;

            buffer.GetWord(3, value);

            ModeIndicator::Set(ModeIndicator::Internal);

            if (value.IsEqual("RED"))
            {
                type = TypeColor::Red;

                Indicator::TurnOn(gset.ColorRed(), 0, true);
            }
            else if (value.IsEqual("GREEN"))
            {
                type = TypeColor::Green;

                Indicator::TurnOn(gset.ColorGreen(), 0, true);
            }
            else
            {
                uint color = 0;

                SU::AtoUInt(value.data, &color, 16);

                Indicator::TurnOn(Color::FromUint(color), 0, true);
            }

            HAL_USART::UART::SendOK();

            return true;
        }
    }
    else if(word2.IsEqual("OFF"))
    {
        ModeIndicator::Set(ModeIndicator::External);

        type = TypeColor::_None;

        Indicator::TurnOff(0, true);

        HAL_USART::UART::SendOK();

        return true;
    }
    else if (word2.IsEqual("SET"))
    {
        ParseBuffer word3;

        buffer.GetWord(3, word3);

        ParseBuffer word4;

        buffer.GetWord(4, word4);

        uint color = 0;

        SU::AtoUInt(word4.data, &color, 16);

        if (word3.IsEqual("RED"))
        {
            gset.SetColorRed(Color::FromUint(color));

            if (type == TypeColor::Red)
            {
                Indicator::TurnOn(gset.ColorRed(), 0, true);
            }

            HAL_USART::UART::SendOK();

            return true;
        }
        else if(word3.IsEqual("GREEN"))
        {
            gset.SetColorGreen(Color::FromUint(color));

            if (type == TypeColor::Green)
            {
                Indicator::TurnOn(gset.ColorGreen(), 0, true);
            }

            HAL_USART::UART::SendOK();

            return true;
        }
    }
    else if (word2.IsEqual("SCRIPT"))
    {
        int num_script = 0;

        if (buffer.GetIntDec(3, &num_script))
        {
            if (num_script <= 0)
            {
                ScriptLED::Disable();

                return true;
            }
            else if (num_script <= TypeScriptLED::Count)
            {
                ScriptLED::Set((TypeScriptLED::E)(num_script - 1));

                ScriptLED::Enable();

                return true;
            }
        }
    }

    return false;
}


bool Communicator::Com_MODE_REQ(BufferUSART &)
{
    HAL_USART::UART::SendString(ModeReader::Name(ModeReader::Current()));

    return true;
}


bool BufferUSART::WordIs(int num_word, pchar string) const
{
    ParseBuffer word;

    SU::GetWord(DataChar(), num_word, &word);

    return std::strcmp(word.data, string) == 0;
}


void BufferUSART::GetWord(int num_word, ParseBuffer &word) const
{
    SU::GetWord(DataChar(), num_word, &word);
}


int BufferUSART::GetCountWords() const
{
    if (size == 0 || buffer[0] == '\0')
    {
        return 0;
    }

    int num_words = 0;

    const uint8 *pointer = buffer;

    while (true)
    {
        if (*pointer == '\0')
        {
            num_words++;
            break;
        }
        else if (*pointer == ' ' || *pointer == '\n')
        {
            num_words++;

            while (*pointer == ' ')
            {
                pointer++;

                if (*pointer == '\0')
                {
                    break;
                }
            }
        }

        pointer++;
    }

    return num_words;
}


bool BufferUSART::GetIntDec(int num_word, int *value) const
{
    ParseBuffer word;

    SU::GetWord(DataChar(), num_word, &word);

    return SU::AtoIntDec(word.data, value);
}


bool BufferUSART::GetParamIntDec(pchar param, int *value) const
{
    int num_words = GetCountWords();

    for (int i = 2; i < num_words + 1; i++)
    {
        ParseBuffer word;

        SU::GetWord(DataChar(), i, &word);

        if (word.IsParameter(param))
        {
            *value = Parameter(word).GetInt();

            return true;
        }
    }

    return false;
}


bool BufferUSART::GetParamUInt128hex(pchar param, Block16 *value) const
{
    int num_words = GetCountWords();

    for (int i = 2; i < num_words + 1; i++)
    {
        ParseBuffer word;

        SU::GetWord(DataChar(), i, &word);

        if (word.IsParameter(param))
        {
            *value = Parameter(word).GetUint128hex();

            return true;
        }
    }

    return false;
}


bool BufferUSART::GetParamUInt(pchar param, uint *value, int base) const
{
    int num_words = GetCountWords();

    for (int i = 2; i < num_words + 1; i++)
    {
        ParseBuffer word;

        SU::GetWord(DataChar(), i, &word);

        if (word.IsParameter(param))
        {
            return Parameter(word).GetUInt(value, base);
        }
    }

    return false;
}


bool BufferUSART::GetUInt(int num_word, uint *value, int base) const
{
    if (base == 10)
    {
        ParseBuffer word;

        SU::GetWord(DataChar(), num_word, &word);

        return SU::AtoUInt(word.data, value, 10);
    }
    else if (base == 16)
    {
        ParseBuffer word;

        SU::GetWord(DataChar(), num_word, &word);

        *value = (uint)std::strtoull(word.data, nullptr, 16);

        return true;
    }

    return false;
}


bool BufferUSART::GetUInt64(int num_word, uint64 *value, int base) const
{
    ParseBuffer word;

    SU::GetWord(DataChar(), num_word, &word);

    return SU::AtoUInt64(word.data, value, base);
}


bool BufferUSART::GetUInt128hex(int num_word, Block16 *value) const
{
    ParseBuffer word;

    SU::GetWord(DataChar(), num_word, &word);

    return SU::AtoUInt128hex(word.data, value);
}


bool BufferUSART::GetParamUInt64dec(pchar param, uint64 *value) const
{
    int num_words = GetCountWords();

    for (int i = 2; i < num_words + 1; i++)
    {
        ParseBuffer word;

        SU::GetWord(DataChar(), i, &word);

        if (word.IsParameter(param))
        {
            *value = Parameter(word).GetUInt64dec();

            return true;
        }
    }

    return false;
}
