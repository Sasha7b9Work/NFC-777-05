// 2023/09/03 23:51:12 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/Commands.h"
#include "Hardware/HAL/HAL.h"
#include "Utils/StringUtils.h"
#include "Reader/Cards/CardTasks.h"
#include "Reader/Cards/Card.h"
#include "Reader/RC663/RC663.h"
#include "Reader/Cards/TypeCard.h"
#include "Communicator/Communicator.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>


namespace Task
{
    struct MasterFull
    {
        SettingsMaster set;
        char bitmap[161];
        void Run();
    };

    struct MasterPassword
    {
        uint64 old_pass;
        uint64 new_pass;
        void Run();
    };

    struct UserFull
    {
        uint64 password;
        uint64 number;
        void Run();
    };

    struct UserPassword
    {
        uint64 password;
        void Run();
    };

    struct WriteKeysMFSL0
    {
        Block16 keys[SET2::KeysMFP::NUM_KEYS] =
        {
            Block16{0, 0},
            Block16{0, 0},
            Block16{0, 0},
            Block16{0, 0},
            Block16{0, 0}
        };
        void Run();
    };

    union UTask
    {
        UTask() { }
        MasterFull      masterFull;
        MasterPassword  masterPassword;
        UserFull        userFull;
        UserPassword    userPassword;
        WriteKeysMFSL0  writeKeys;
    };

    static UTask utask;

    static uint8 master_full = 0;
    static uint8 master_password = 0;
    static uint8 user_full = 0;
    static uint8 user_password = 0;
    static uint8 write_keys = 0;
}


void Task::Update()
{
    if (master_full)
    {
        utask.masterFull.Run();
    }

    if (master_password)
    {
        utask.masterPassword.Run();
    }

    if (user_full)
    {
        utask.userFull.Run();
    }

    if (user_password)
    {
        utask.userPassword.Run();
    }

    if (write_keys)
    {
        utask.writeKeys.Run();
    }

    master_full = 0;
    master_password = 0;
    user_full = 0;
    user_password = 0;
    write_keys = 0;
}


void Task::CreateMasterFull(const SettingsMaster &set, char bitmap_cards[161])
{
    utask.masterFull.set = set;
    std::strcpy(utask.masterFull.bitmap, bitmap_cards);
    master_full = 1;
}


void Task::CreateMasterPasswords(uint64 old_pass, uint64 new_pass)
{
    utask.masterPassword.old_pass = old_pass;
    utask.masterPassword.new_pass = new_pass;
    master_password = 1;
}


void Task::CreateUserFull(uint64 password, uint64 number)
{
    utask.userFull.password = password;
    utask.userFull.number = number;
    user_full = 1;
}


void Task::CreateUserPassword(uint64 password)
{
    utask.userPassword.password = password;
    user_password = 1;
}


void Task::WriteKeysCardMFSL0(Block16 keys[SET2::KeysMFP::NUM_KEYS])
{
    std::memcpy(utask.writeKeys.keys, keys, sizeof(keys[0]) * SET2::KeysMFP::NUM_KEYS);
    write_keys = 1;
}


void Task::UserPassword::Run()
{
    bool result =
        Card::_14443::T_CL::SwitchToSL3() &&                // Для карт с уровнем SL3 производим переход на этот уровень
        Card::WriteKeyPassword(Block16(0, password)) &&
        Card::EnableCheckPassword();

    Card::_14443::T_CL::ResetAuth();

    HAL_USART::UART::TransmitF("#MAKE USER PASSWORD %llu %s", password, result ? "OK" : "FAIL");

    RC663::RemoveCard();
}


void Task::UserFull::Run()
{
    bool res_switch = Card::_14443::T_CL::SwitchToSL3();            // Для карт с уровнем SL3 производим переход на этот уровень
    bool res_number = Card::WriteNumber(number);
    bool res_key = Card::WriteKeyPassword(Block16(0, password));
    bool res_enable = Card::EnableCheckPassword();

    Card::_14443::T_CL::ResetAuth();

    LOG_WRITE("switch=%d number=%d key=%d enable=%d", res_switch, res_number, res_key, res_enable);

    bool result = res_switch && res_number && res_key && res_enable;

    HAL_USART::UART::TransmitF("#MAKE USER PASSWORD %llu NUMBER %llu %s", password, number, result ? "OK" : "FAIL");

    RC663::RemoveCard();
}


void Task::MasterFull::Run()
{
    set.s04.u32 = (uint)-1;

    set.CalculateAndWriteCRC32();

    bool result = false;

    for (int i = 0; i < 10; i++)                     // Повторяем 10 раз на случай сбоя при процедуре
    {
        if (Card::_14443::T_CL::SwitchToSL3()) {                            // Для карт с уровнем SL3 производим переход на этот уровень
            if (Card::WriteBitmapCards(bitmap)) {                          // Пишем битовую карту разрещённых карт
                if (Card::WriteSettings(set)) {                             // Пишем все данные в карту за раз
                    if (Card::WriteKeyPassword(set.OldPassword())) {        // Установить пароль на карту
                        if (Card::EnableCheckPassword())                    // Закрываем сектора от чтения/записи
                        {
                            result = true;
                            break;
                        } else {
                            LOG_ERROR("can't enable check password");
                            RC663::RecoonectCard();
                        }
                    } else {
                        LOG_ERROR("can't write check password");
                        RC663::RecoonectCard();
                    }
                } else {
                    LOG_ERROR("can't write settings");
                    RC663::RecoonectCard();
                }
            } else {
                LOG_ERROR("can't write bitmap cards");
                RC663::RecoonectCard();
            }
        } else {
            LOG_ERROR("can't switch to sl3");
            RC663::RecoonectCard();
        }
    }

    Card::_14443::T_CL::ResetAuth();

    HAL_USART::UART::TransmitF("#MAKE MASTER OLD_PASS=%llu NEW_PASS=%llu %s",
        set.OldPassword().u64[0],
        set.Password().u64[0],
        result ? "OK" : "FAIL");

    RC663::RemoveCard();
}


void Task::MasterPassword::Run()
{
    SettingsMaster set;

    set.PrepareMasterOnlyPassword(new_pass);                // Подготавливаем карту к записи только пароля

    bool result =
        Card::_14443::T_CL::SwitchToSL3() &&                // Для карт с уровнем SL3 производим переход на этот уровень
        Card::WriteSettings(set) &&                         // Записываем все настройки за раз
        Card::WriteKeyPassword(Block16(0, old_pass)) &&     // Паролим карту старым паролем - этот пароль в считывателе, который нужно перезаписать
        Card::EnableCheckPassword();                        // И включаем защиту на карте

    Card::_14443::T_CL::ResetAuth();

    HAL_USART::UART::TransmitF("#MAKE MASTER OLD_PASS=%llu NEW_PASS=%llu %s",
        set.OldPassword().u64[0],
        set.Password().u64[0],
        result ? "OK" : "FAIL");

    RC663::RemoveCard();
}


void Task::WriteKeysMFSL0::Run()
{
    HAL_USART::UART::SendStringF("#KEYSMFSL0 WRITE %s",
        (Card::_14443::T_CL::WriteKeys(utask.writeKeys.keys, 4) ? "OK" : "FAIL"));
}
