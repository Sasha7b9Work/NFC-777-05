// 2022/6/24 15:52:45 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


namespace LIS2DH12
{
    void Init();

    // Вызывается в главном цикле
    void Update();

    // Обнаружен угол отклонения, бОльший допустимого
    bool IsAlarmed();

    // Возвращает мгновенную температуру, измеренную в последний раз
    float GetTemperature();
}
