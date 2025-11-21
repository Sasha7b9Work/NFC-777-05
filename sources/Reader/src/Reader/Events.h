// 2023/09/18 09:16:51 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


struct UID;


/*

    Функции вызываются после обнаружения всяких событий карт

*/

namespace Event
{
    // В поле считывателя обнаружена карта
    void CardDetected();

    // Карта аутентифицирована или не аутентицфицирована
    void CardReadOK(const UID &, uint64 number);

    void CardReadFAIL(const UID &);

    // Карта удалена
    void CardRemove();
}
