// 2025/7/6 20:37:32 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Settings/SettingsTypes.h"


struct TypeStep
{
    enum E
    {
        Color,      // Установить цвет свечения. Данная команда переводит текущий цвет в color.
                    //      count100ms - в течение этого времени плавно устанавливается цвет
                    //      color      - требуемый цвет
        Nop,        // Ничего не делать
        Repeat      // Перейти в начало скрипта
    };
};


struct Step
{
    uint value_color;       // Цвет в конце шага
    TypeStep::E type;       // Тип шага
    uint16 duration = 0;    // Длительность шага
};
