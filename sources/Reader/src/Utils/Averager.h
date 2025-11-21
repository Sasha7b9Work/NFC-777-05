// 2022/11/06 19:23:41 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include <cstring>


// Структура содержит нативные данные, считанные из акселерометра
union StructDataRaw
{
    StructDataRaw(int16 val = 0) : raw(val) { }

    struct
    {
        uint8 lo;
        uint8 hi;
    };

    int16 raw;

    float ToTemperatrue()
    {
        return 25.0f + (float)raw / 256.0f;
    }

    float ToAccelearation()
    {
        return (float)raw / 16.0f / 1000.0f;
    }
};


template<class T, int size_buffer>
class AveragerRaw //-V730
{
public:
    void Push(T value)
    {
        if (num_elements == size_buffer)
        {
            std::memmove(buffer, buffer + 1, sizeof(T) * (size_buffer - 1));
            num_elements--;
        }
        buffer[num_elements++] = value;
    }

    T Get()
    {
        int64 sum = 0;

        for (int i = 0; i < num_elements; i++)
        {
            sum += (int64)buffer[i].raw;
        }

        return StructDataRaw((int16)(sum / (int64)num_elements));
    }

private:
    T buffer[size_buffer];
    int num_elements = 0;
};


template<class T, int size_buffer>
class Averager
{
public:
    void Push(T value)
    {
        if (num_elements == size_buffer)
        {
            std::memmove(buffer, buffer + 1, sizeof(T) * (size_buffer - 1));
            num_elements--;
        }
        buffer[num_elements++] = value;
    }

    T Get() const
    {
        T sum = T(0);

        for (int i = 0; i < num_elements; i++)
        {
            sum += buffer[i];
        }

        return sum / (T)num_elements;
    }

private:
    T buffer[size_buffer];
    int num_elements = 0;
};
