// 2023/11/22 08:09:23 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


#ifdef TYPE_BOARD_799
    #define MCU_GD
#endif


// Если определено, то вместо WG данные передаются в UART - для контроля
//#define EMULATE_WG


#define MS_IN_SEC                   60000
#define TIME_RISE_LEDS              250                 // Время нарастания света от нуля до полного
#ifdef MCU_GD
    #define MEMORY_FOR_SOUNDS       (872 * 1024)        // Зарезервировано места для звуков
#else
    #define MEMORY_FOR_SOUNDS       (360 * 1024)        // Зарезервировано места для звуков
#endif
#define NUMBER_SOUDNS               10                  // Количество звуков во внешней памяти
#define SIZE_FIRMWARE_STORAGE       ((128 - 4) * 1024)  // 4 кБ в конце идёт на некоторые настройки
#define ADDRESS_SOUNDS              (128 * 1024)


/*
    Настройки в секторе 31 (адрес 123 * 1024):
    0 (int8) - минимальная температура
    1 (int8) - максимальная температура
*/
