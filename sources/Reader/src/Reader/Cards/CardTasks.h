// 2023/09/03 23:51:25 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Settings/Settings.h"
#include "Settings/SET2.h"


/*

    ƒл€ команды записи карты с внешней программы CIC

*/


namespace Task
{
    // «апись полной мастер-карты - с настройками
    // ƒлина строки - не более 161. 160 символов
    void CreateMasterFull(const SettingsMaster &, char bitmap_cards[161]);

    // Ёто дл€ создани€ мастер-карты, котора€ только мен€ет пароль
    // old_pass - этим паролем только паролим карту
    // new_pass - этот пароль будет установлен на считыватель после отработки карты
    void CreateMasterPasswords(uint64 old_pass, uint64 new_pass);

    void CreateUserFull(uint64 password, uint64 number);

    void CreateUserPassword(uint64 password);

    // «аписать ключи 9000...9004 в карту Mifare в режиме SL0
    void WriteKeysCardMFSL0(Block16 keys[SET2::KeysMFP::NUM_KEYS] );

    void Update();
}
