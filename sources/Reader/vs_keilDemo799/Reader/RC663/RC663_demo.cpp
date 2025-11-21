// 2022/7/3 11:16:16 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/RC663.h"
#include "Reader/RC663/Commands.h"
#include "Reader/RC663/Registers.h"
#include "Reader/RC663/LPCD.h"
#include "Device/Device.h"
#include "Communicator/Communicator.h"
#include "Settings/Settings.h"
#include "Hardware/Timer.h"
#include "Hardware/HAL/HAL.h"
#include "Hardware/HAL/HAL_PINS.h"
#include "Reader/Events.h"
#include "Modules/Indicator/Indicator.h"
#include "Modules/Player/Player.h"
#include "Reader/Signals.h"
#include "Utils/StringUtils.h"
#include "Reader/Cards/CardNTAG.h"
#include "Reader/Cards/CardMF.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/CardTasks.h"
#include "Reader/Cards/Card.h"
#include "Reader/Reader.h"
#include "Reader/Cards/CardsEntities.h"
#include "Settings/SET2.h"
#include "Modules/Indicator/Scripts/ScriptsLED.h"
#include "system.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace RC663
{
    namespace Power
    {
        void Off()
        {
            pinENN.ToHi();
        }

        void On()
        {
            pinENN.ToLow();
        }

        void Init()
        {
            pinENN.Init();
            Off();
        }
    }

    namespace RF
    {
        void On()
        {
            Command::Idle();

            uint8 reg = Register(MFRC630_REG_DRVMOD).Read();
            _SET_BIT(reg, 3);
            Register(MFRC630_REG_DRVMOD).Write(reg);
        }

        void Off()
        {
            uint8 reg = Register(MFRC630_REG_DRVMOD).Read();
            _CLEAR_BIT(reg, 3);
            Register(MFRC630_REG_DRVMOD).Write(reg);

            CardMF::ResetReadWriteOperations();
        }
    }

    // Возвращает false, если в процессе обработки произошла ошибка чтения/записи
    static bool ProcessMasterCard();

    // Возвращает false, если в процессе обработки произошла ошибка чтения/записи
    static bool ProcessUserCard();

    static bool DetectCard();

    // Проверяет, надёжна ли связь с картой TCL
    // \warn при проверке читается/пишется 15-й сектор. Может повредиться находящаяся в нём информация
    bool CheckTheTeliabilityOfTheConnectionTCL();

    static bool prev_detected = false;
}


void RC663::PreparePoll()
{
    pinIRQ_TRX.Init();

    Power::Init();

    Power::On();

    RF::Off();

    HAL_FLASH::RC663::LoadAntennaConfiguration106();

    HAL_FLASH::RC663::LoadProtocol();

    FIFO::Init();

    Register(MFRC630_REG_IRQ1EN).Write(0xC0);      // IRQ1En    // Включаем пин прерывание в Push-Pull

    while ((IRQ0::GetValue() & IRQ0::IdleIRQ) == 0)              // Ждём, пока отработают внутренние схемы
    {
    }

    Register(MFRC630_REG_IRQ0EN).Write(0x84);      // IRQ0En    // Включаем "железное прерываниие" IRQ на чтение данных. Инвертируем

    while ((IRQ0::GetValue() & IRQ0::IdleIRQ) == 0)          // Ждём, пока отработают внутренние схемы
    {
    }
}


bool RC663::Anticollision()
{
    bool detected = false;

    for (int i = 0; i < 3; i++)
    {
        PreparePoll();

        RF::On();

        Card::uid.Clear();

        Command::Idle();

        TimeMeterUS meterUS;

        while (meterUS.ElapsedUS() < 5100)
        {
            Device::UpdateTasks();
        }

        Card::atqa = Command::ReqA();

        if (Card::atqa != 0)
        {
            if (Command::AnticollisionCL(1, &Card::uid))
            {
                if (Command::SelectCL(1, &Card::uid))
                {
                    if (!Card::uid.Calcualted())
                    {
                        if (Command::AnticollisionCL(2, &Card::uid))
                        {
                            Command::SelectCL(2, &Card::uid);
                        }
                    }
                }
            }
        }

        detected = Card::uid.Calcualted();

        if(Card::IsInserted() == detected)      // Здесь должно быть ((Card::IsInserted() && detected) || (!Card::IsInserted() && !detected))
        {                                       // Упростили
            break;
        }
    }

    return detected;
}


bool RC663::CheckTheTeliabilityOfTheConnectionTCL()
{
    Card::_14443::T_CL::ResetAuth();            // \todo Не работает без этого

    // Делаем аутентификацию без проверки пароля. Просто чтобы получить ответ от карты
    Card::_14443::T_CL::FirstAuthentication(1, SettingsMaster::PSWD::default_Mifare);

    // И если ответ не получен - свззь не надёжна
    if (Card::null_bytes_received)
    {
        return false;
    }

    Block16 block(0, 0);

    const int nbr_block = 4 * 15;                   // Будем работать с первым блоком 15-го (последнего из мелких) сектора

    Block16 password = Card::GetPasswordAuth();     // Cохраняем пароль

    if (!Card::_14443::T_CL::ReadBlock(nbr_block, block) ||
        Card::null_bytes_received) //-V560
    {
        Card::SetPasswordAuth(password);

        return false;
    }

    if (!Card::_14443::T_CL::WriteBlock(nbr_block, block) ||
        Card::null_bytes_received) //-V560
    {
        Card::SetPasswordAuth(password);

        return false;
    }

    Card::SetPasswordAuth(password);                // Восстанавливаем пароль

    return true;
}


void RC663::RemoveCard()
{
    Card::Eject(true);

    prev_detected = false;
}


bool RC663::DetectCard()
{
    bool detected = Anticollision();

    if (detected && !prev_detected)
    {
        TypeCard::Detect();

        if (gset.GetOnlySL3() && Card::SL < 3)      // Если разрешены только карты SL3 и уровень безопасности карты меньше, чем 3
        {
            detected = false;                       // то делаем вид, что карты нету
        }

        if (detected)
        {
            Event::CardDetected();

            while (Indicator::IsRunning())
            {
                Device::UpdateTasks();
            }

            if (ModeSignals::IsExternal())
            {
                Indicator::FireInputs(pinLG.IsLow(), pinLR.IsLow());
            }
            else
            {
                if (!ModeReader::IsExtended())
                {
                    Indicator::TurnOn(gset.ColorRed(), TIME_RISE_LEDS, false);
                }
            }
        }
    }

    if (prev_detected && detected && TypeCard::IsSupportT_CL())     // Если в прошлом цилке карта была определена, то нужно послать RATS
    {
        Card::_14443::RATS();
    }

    prev_detected = detected;

    ScriptLED::DetectCard(detected);

    return detected;
}


bool RC663::UpdateExtendedMode(const TypeAuth &type_auth, bool new_auth)
{
    bool result = false;

    if (DetectCard())
    {
        // Здесь известны SAK, ATQA, etc. Определён тип карты

        bool auth_ok = false;

        if (type_auth.enabled)
        {
            auth_ok = result = Card::PasswordAuth();
        }
        else
        {
            Block4 block;

            auth_ok = Card::HI_LVL::ReadBlock(TypeCard::NTAG::NumberBlockCFG1(), block);

            result = auth_ok;
        }

        uint64 number = 0;

        auth_ok = Card::ReadNumber(&number);

        Card::InsertExtendedMode(type_auth, auth_ok, new_auth, number);

        Task::Update();
    }
    else
    {
        Card::Eject(true);
    }

    RF::Off();

    return result;
}


bool RC663::UpdateNormalMode()
{
    bool result = false;

    if (DetectCard()) 
    {
        Card::SetPasswordAuth(SettingsMaster::PSWD::Get());             // Этим паролем будем пользоваться для аутентификации

        if (!Card::IsInserted())
        {
            uint64 number = 0;
            char message_fail[128];
            std::sprintf(message_fail, "#CARD %s AUTHENTICATION FAILED", Card::uid.ToASCII().c_str());

            if (Card::PasswordAuth() &&
                Card::ReadNumber(&number))
            {
                result = true;

                ProtectionBruteForce::Reset();

                if ((uint)number == (uint)-1)                       // В четвёртом секторе FF FF FF FF - мастер-карта
                {
                    if (!ProcessMasterCard())
                    {
                        HAL_USART::UART::SendString(message_fail);
                    }
                }
                else                                                // Пользовательская карта
                {
                    if (!ProcessUserCard())
                    {
                        HAL_USART::UART::SendString(message_fail);
                    }
                }
            }
            else
            {
                if (!Card::IsInserted())
                {
                    HAL_USART::UART::SendString(message_fail);

                    ProtectionBruteForce::FailAttempt();
                }

                Card::InsertNormalModeUser(0, false);
            }
        }
    }
    else
    {
        Card::Eject(true);
    }

    RF::Off();

    return result;
}


bool RC663::ProcessMasterCard()
{
    SettingsMaster settings;

    bool result = false;

    if (Card::HI_LVL::ReadData(4, (uint8 *)&settings, settings.Size()))
    {
        if (settings.CRC32IsMatches())
        {
            if (settings.OldPassword().u64[0] == (uint64)-1)                    // Меняем пароль
            {
                SettingsMaster::PSWD::Set(settings.Password());

                result = true;
            }
            else if (settings.OldPassword() == SettingsMaster::PSWD::Get())     // Меняем всю конфигурацию
            {
                bool osdp_enabled = gset.IsEnabledOSDP();                       // Сохраняем состояние режима ОСДП - выключить его картой нельяз
                gset = settings;
                if (osdp_enabled && !HAL::IsDebugBoard())
                {
                    gset.EnableOSDP(true);                                      // Включаем режим ОСДП. если он был ранее установлен
                }
                gset.Save();

                char access_card[500];

                if (Card::ReadBitmapCards(access_card))
                {
                    SET2::AccessCards::WriteToROM(access_card);

                    result = true;
                }
            }
        }
    }

    bool need_led = !Card::IsInserted();

    Card::InsertNormalModeMaster(result);

    if (need_led)
    {
        if (result)
        {
            Communicator::WriteConfitToUSART();

            Signals::GoodMasterCard();
        }
        else
        {
            Signals::BadMasterCard();
        }
    }

    return result;
}


bool RC663::ProcessUserCard()
{
    uint64 number = 0;

    bool result = false;

    if (Card::ReadNumber(&number))
    {
        result = true;
    }

    Card::InsertNormalModeUser(number, result);

    return result;
}


bool RC663::RecoonectCard()
{
    RC663::RF::Off();
    ::Timer::Delay(100);
    Card::Eject(false);
    bool anti = Anticollision();

    LOG_WRITE("Anticollision = %d", anti);

    if (TypeCard::Detect())
    {
        LOG_WRITE("detect card");

        if (TypeCard::IsSupportT_CL())
        {
//            LOG_WRITE("Card is supported T_CL");

//            bool rats = Card::_14443::RATS();

//            LOG_WRITE("RATS is %d", rats);

            return true;
        }
        else
        {
            LOG_WRITE("Card is NOT supported T_CL");
        }

        return true;
    }
    else
    {
        LOG_ERROR("Not detect card");
    }

    LOG_ERROR("Not reconnect card");

    return false;
}
