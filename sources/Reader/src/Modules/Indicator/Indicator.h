// 2023/11/18 16:21:10 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Settings/SettingsTypes.h"


struct ModeIndicator
{
    enum E
    {
        External,       // В этом режиме индикатор загарается по сигналам LG, LR. Режим по умолчанию
        Internal        // В этом режме индикатор не проверяет состояние LG, LR. Управляется командами
    };

    static void Set(ModeIndicator::E mode)
    {
        current = mode;
    }

    static bool IsExternal()
    {
        return (current == External);
    }

private:

    static E current;
};


namespace Indicator
{
    void Init();

    // Вызывается в главном цикле. Возвращает IsRunning()
    bool Update();

    // Данную функцию нужно вызывать после выхода из режима тревоги, например, чтобы привести состояния входов LR, LG в исходное состояние,
    // дабы они могли отслеживать изменения входных сигналов
    void Reset();

    // Возвращает true, если есть невыполненные задания
    bool IsRunning();

    // Возвращает true, если установленный цвет - не ноль, т.е. индикатор светится
    bool IsFired();

    // time - время, за которое нужно выполнить включение
    // если stopped == true, то перед выполнением нужно отменить текущие задачи
    void TurnOn(const Color &, uint time, bool stopped);

    // time - время от начала зажигания до конца выключения
    // если stopped == true, то перед выполнением нужно отменить текущие задачи
    void Blink(const Color &color1, const Color &color2, uint time, bool stopped);

    // time - время, за которое нужно произвести выключение
    // если stopped == true, то перед выполнением нужно отменить текущие задачи
    void TurnOff(uint time, bool stopped);

    // Зажечь в соответствии с состояниями пинов LG, LR
    void FireInputs(bool lg, bool lr);

    // Горит в данный момент
    Color FiredColor();

    namespace TaskOSDP
    {
        struct TemporaryControlCode
        {
            enum E
            {
                DoNotAlter,         // Не изменять временные настройки
                CancelAndDisplay,   // Отменить любую временную операцию и немедленно отобразить состояние
                SetAndStart         // Установить временное состояние, как указано, и запустить таймер обратного отсчета немедленно
            };
        };

        struct PermanentControlCode
        {
            enum E
            {
                DoNotAlter,     // NOP - do not alter this LED's permanent settings
                SetAsGiven,     // Set the permanent state as given
            };
        };

        void Set(
            TemporaryControlCode::E,    // The mode to enter temporarily
            uint temp_time_on,          // The ON duration of the flash, in units of 100 ms
            uint temp_time_off,         // The OFF duration of the flash, in units of 100 ms
            Color &temp_color_on,       // The color to set during the ON time
            Color &temp_color_off,      // The color to set during the OFF time
            uint temp_timer,
            PermanentControlCode::E,    // The mode to return to after the timer expires
            uint perm_time_on,          // The ON duration of the flash, in units of 100 ms
            uint perm_time_off,         // The OFF duration of the flash, in units of 100 ms
            Color &perm_color_on,       // The color to set during the ON time
            Color &perm_color_off       // The color to set during the OFF time
        );

        // Зажечь цвет, установленный в качестве постоянного последней командой
        void FirePermanentColor();
    }
}
