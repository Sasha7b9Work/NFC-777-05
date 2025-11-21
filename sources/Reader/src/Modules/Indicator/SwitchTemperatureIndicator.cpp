// 2024/07/25 10:35:59 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Modules/Indicator/SwitchTemperatureIndicator.h"
#include "Modules/LIS2DH12/LIS2DH12.h"
#include "Modules/Memory/Memory.h"
#include "Modules/Indicator/WS2812B/WS2812B.h"


namespace SwitchTemperatureIndicator
{
    static bool prev_min = false;       // Если true - в прошлый раз была низкая температура
    static bool prev_max = false;       // Если true - в прошлый раз была высокая температура
}


void SwitchTemperatureIndicator::Update()
{
    float temp = LIS2DH12::GetTemperature();

    bool too_low = temp < (float)Memory::Settings::GetTempMin();
    bool too_hi = temp > (float)Memory::Settings::GetTempMax();

    if ((too_low && !prev_min) || (too_hi && !prev_max))       // В данном цикле случился выход за границы
    {
#ifndef TYPE_BOARD_771

        WS2812B::Disable();

#endif
    }
    else if ((!too_low && prev_min) || (!too_hi && prev_max))   // В данном случае вернулись в температурный диапазон
    {
#ifndef TYPE_BOARD_771

        WS2812B::Enable();

#endif
    }

    prev_min = too_low;
    prev_max = too_hi;
}
