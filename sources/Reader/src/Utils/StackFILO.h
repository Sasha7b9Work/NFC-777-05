// 2024/12/09 10:54:02 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


template<int SIZE = 64>
class StackFILO
{
public:
    void Push(uint8 value)
    {
        buffer[size++] = value;
    }
    uint8 Pop()
    {
        if (size > 0)
        {
            size--;
        }

        return buffer[size];
    }
    int Size() const
    {
        return size;
    }
private:
    uint8 buffer[SIZE];
    int size = 0;
};
