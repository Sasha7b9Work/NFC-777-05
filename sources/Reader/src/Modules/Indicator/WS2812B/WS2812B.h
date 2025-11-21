// 2022/6/15 22:17:53 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Modules/Indicator/Indicator.h"
#include "Modules/Indicator/ColorLED.h"

/*
*   Индикаторы
*/


#ifndef TYPE_BOARD_771


namespace WS2812B
{
    void Init();

    // Если num_led == -1, один цвет для всех
    void Fire(int num_led, const ColorLED &color);

    inline int NumLeds() { return 4; }

    // После вызова этой функции индикаторы светиться не будут
    void Disable();

    // После вызова этой функции восстановится свечение индикаторов
    void Enable();
}


#endif
