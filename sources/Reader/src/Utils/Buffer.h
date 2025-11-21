#pragma once
#include "Utils/StringUtils.h"
#include "Utils/Log.h"
#include <cstring>


template <class T, int size_buffer>
class Buffer
{
public:

    Buffer() : size(0)
    {
        Clear();
    }  //-V730

    bool IsFull() const
    {
        return Size() == size_buffer;
    }

    // Удвалить первые n байт
    void RemoveFirst(int n)
    {
        if (n >= size)
        {
            Clear();
        }
        else
        {
            size -= n;

            std::memmove(buffer, buffer + (uint)n * sizeof(T), (uint)size * sizeof(T));
        }
    }

    const T *Data() const { return buffer; }

    pchar DataChar() const { return (pchar)Data(); }

    // Возвращает количество элементов в буфере
    int Size() const
    {
        return size;
    }

    bool Append(T elem)
    {
        if (size == size_buffer)
        {
            return false;
        }

        buffer[size++] = elem;

        return true;
    }

    bool Append(const void *data, int _size)
    {
        if (Size() + _size > size_buffer)
        {
            LOG_ERROR("Override buffer");
            return false;
        }

        std::memcpy(&buffer[size], data, (uint)_size * sizeof(T));
        size += _size;

        return true;
    }

    void Clear()
    {
        size = 0;
        std::memset(buffer, 0x00, size_buffer);
    }

    // Возвращает позицию первого элемента последовательности data в buffer, если таковая имеется. Иначе : -1.
    int Position(const void *data, int num_bytes) const
    {
        if (num_bytes > size)
        {
            return -1;
        }

        for (int i = 0; i <= size - num_bytes; i++)
        {
            void *pointer = (void *)&buffer[i];

            if (std::memcmp(pointer, data, (uint)num_bytes) == 0)
            {
                return i;
            }
        }

        return -1;
    }

    // Если в буфере есть команда, возвращает позицию завершающего '\n'
    int ExtractCommand() const
    {
        int position = Position("\n", 1);

        if (position == -1)
        {
            return position;
        }

        if (!BeginWith("#MAKE MASTER\n"))
        {
            return position;
        }

        pchar entry = SU::GetSubString(DataChar(), "#END\n");

        if (entry == nullptr)
        {
            return -1;
        }

        return entry - DataChar() + 4;
    }

    bool BeginWith(pchar symbols) const
    {
        if (Size() < (int)std::strlen(symbols))
        {
            return false;
        }

        return std::memcmp(buffer, symbols, std::strlen(symbols)) == 0;
    }

    const T &operator[](int i) const
    {
        if (i >= 0 && i < Size())
        {
            return buffer[i];
        }

        static T null(0);

        return null;
    }

    T &operator[](int i)
    {
        if (i >= 0 && i < Size())
        {
            return buffer[i];
        }

        static T null(0);

        return null;
    }

protected:

    int size;

    T buffer[size_buffer];
};


class Buffer512 : public Buffer<uint8, 512>
{
public:
    Buffer512() : Buffer<uint8, 512>() { }
};
