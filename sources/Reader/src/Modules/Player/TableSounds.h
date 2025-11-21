// 2024/03/26 16:02:20 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Modules/Memory/Memory.h"


// Теблица описания хранения звуков во внешней памяти
namespace TableSounds
{
    // По этому адресу расположена таблица в памяти
    inline uint *Begin() { return (uint *)ADDRESS_SOUNDS; }; //-V566

    // Размер таблицы
    inline uint Size() { return NUMBER_SOUDNS * sizeof(int); }

    // Размер звука num_sound в байтах
    inline int SizeSound(int num_sound)
    {
        return Memory::ReadInt((uint)Begin() + (uint)num_sound * 4U);
    }

    // Столько отсчётов содержится в звуке num_sound
    inline int CountSamples(int num_sound) { return SizeSound(num_sound) / 2; }

    // Адрес первого байта звука num_sound
    inline uint AddressSound(int num_sound)
    {
        return (num_sound == 0) ? ((uint)Begin() + Size()) : (AddressSound(num_sound - 1) + (uint)SizeSound(num_sound - 1));
    }
}
