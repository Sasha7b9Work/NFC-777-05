// 2025/6/19 08:07:12 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


struct TypeScriptLED
{
    enum E
    {
        RunningWave,            // Бегущая строка
        BreathingSystem,        // Дыхание системы
        Rainbow,                // Радуга
        SleepMode,              // Режим сна
        WaveWithAttenuation,    // Волна с затуханием
        Count
    };
};


namespace ScriptLED
{
    // Устанавливает сценарий, который будет отрабатывать
    void Set(TypeScriptLED::E);

    // Запускает ранее установленный функцией Set() сценарий на выполнение
    void Enable();

    // Останавливает выполнение скрипта
    void Disable();

    // Вызывается в главном цикле
    void Update();

    // Сброс внутреннего таймера. После того, как таймер досчитает до нуля, автоматически запустится скрипт, установленный функцией Set()
    void ResetTimer();

    void DetectCard(bool detect);
}

