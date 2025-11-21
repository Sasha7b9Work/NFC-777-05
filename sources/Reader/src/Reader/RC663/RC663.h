// 2022/7/3 11:16:31 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Reader/RC663/Commands.h"
#include "Reader/RC663/RC663_def.h"


struct TypeAuth;


/*

    Функции работы с чипом CLRC663 plus

*/


namespace RC663
{
    // Подготовить считыватель к работе с картой в Pool - режиме
    void PreparePoll();

    bool UpdateNormalMode();

    // new_auth true говорит о том, что нужно выводить сообщение, даже если пароль не изменился
    bool UpdateExtendedMode(const TypeAuth &, bool new_auth);

    // Если есть карта, то делает антиколлизию и возвращает true
    bool Anticollision();

    // Если в процессе работы с картой происходит ошибка, дальнейшая работа с картой невозможна.
    // Чтобы продолжить работу с картой, нужно переподключить карту с помощью этой функции
    bool RecoonectCard();

    // Программное удаление карты. После вызова этой функции в следующем цикле карта будет обнаружена.
    // Нужно в основном для переобнаружения карты после записи программой CIC
    void RemoveCard();

    // Подача питания на чип
    namespace Power
    {
        void Init();

        void On();

        void Off();
    }

    // Включение электромагнитного поля радара
    namespace RF
    {
        void On();

        void Off();
    }
}
