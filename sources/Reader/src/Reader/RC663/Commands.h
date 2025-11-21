// 2022/7/6 10:32:31 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include <cstring>


struct UID;


namespace Command
{
    void Idle();

    void Reset();

    // Возвращает atqa
    uint16 ReqA();

    // Посылают команду антиколлизии CL1 и ожидает ответ. Возвращает true в случае получения ответа
    bool AnticollisionCL(int cl, UID *uid);

    bool SelectCL(int cl, UID *uid);

    // Передаёт и запускает процесс приёма
    void Transceive1(uint8);
    void Transceive(uint8, uint8);
    void Transceive(const uint8 *buffer, int size);
}
