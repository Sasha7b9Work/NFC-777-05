// 2022/6/30 23:43:06 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


// Спящий режим, напряжение питания
namespace Power
{
    void Init();

    // Заходим в спящий режим
    void EnterSleepMode();

    // Напряжение ниже нормы
    bool IsLessThanNormal();

    // Сюда постоянно добавляем измеренное напряжение питания, чтобы потом по некоторому алгоритму GetVoltage()
    // вернуло действующее напряжение на входе
    void AppendMeasure(float);

    // Возвращает действующее напряжение на входе
    float GetVoltage();

    extern void *handleTIM;   // TIM_HandleTypeDef
}
