// 2024/08/14 16:11:18 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


/*
*  Класс выдаёт bool в зависимости от того, попадает ли входное значение в
*  заданный диапазон. При это можно задать некоторый гистерезис
*/


template<class T>
class RangeVerifierHysteresis
{
public:
    // border - граничное значение
    // delta_neg - отклонение "вниз". Т.е. срабатывание "вниз" произойдёт на (border - delta_neg)
    // delta_pos - отклонение "вверх". Т.е. срабатывание "вверх" произойдёт на (border + delta_pos)
    RangeVerifierHysteresis(T _border, T _delta_neg, T _delta_pos, T _value) :
        border(_border), delta_neg(_delta_neg), delta_pos(_delta_pos)
    {
        prev_resolve = (_value < _border) ? false : true;
    }
    bool Resolve(T value)
    {
        if (value < border - delta_neg)
        {
            prev_resolve = false;
        }
        else if (value > border + delta_pos)
        {
            prev_resolve = true;
        }

        return prev_resolve;
    }
private:
    const T border;
    const T delta_neg;
    const T delta_pos;
    bool prev_resolve;
};

