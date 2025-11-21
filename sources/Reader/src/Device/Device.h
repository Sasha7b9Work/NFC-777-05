// 2022/04/27 11:48:09 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Utils/String.h"


namespace Device
{
    void Init();

    void Update();

    // Обработка динамика, индикакатора, USART
    bool UpdateTasks();

    // Окончена инициализация. Работаем
    bool IsInitializationCompleted();

    // Здесь будем проверять состояние входных проводов
    namespace StartState
    {
        // Если true, то нужно делать сброс на зводские настройки
        bool NeedReset();

        // Если true, то нужен режим режим минимального WG - когда по WG передаётся лишь GUID карты
        bool NeedMinimalWG();

        // Если true, то нужно включать режим автономного считывателя
        bool NeedOffline();
    }

    // Всяческая информация о считывателе и прошивках
    namespace Info
    {
        namespace Hardware
        {
            String<> Version_ASCII_HEX();           // Аппаратная версия
            String<> DateManufactured();            // Дата производства
            uint64 MacUint64();
            String<> SerialNumber_ASCII_HEX();      // Серийный номер изделия
            uint   SerialNumber_UINT();             // Серийный номер изделия

            uint   Vendor();
            uint8  ManufacturedModelNumber();
            uint8  ManufacturedVersion();
        }

        namespace Loader
        {
            String<> Version_ASCII_HEX();           // Версия загрузчика
        }

        namespace Firmware
        {
            String<> TypeID_ASCII_HEX();            // Идентификатор типа изделия
            String<> VersionMajorMinor_ASCII();     // Мажорная/минорная версии
            uint16   VersionBuild();
            String<> TextInfo();
        }
    }
}
