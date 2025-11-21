// 2024/09/13 09:39:08 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


namespace RC663
{
    namespace LPCD
    {
        bool IsEnabled();

        // Калибровка перед входом в спящий режим
        void Calibrate();

        // Включаем LPСВ, входим в спящий режим
        void Enter();

        // true, если произошло определение карты
        bool IsCardDetected();
    }
}
