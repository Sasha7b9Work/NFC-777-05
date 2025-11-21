// 2022/08/05 20:41:56 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


/*
*   Флеш-память внешний микросхемы
* 
*   W25Q80DV
*/


namespace Memory
{
    const int SIZE_BLOCK = 256;

    inline int SizeBlock() { return 4 * 1024; }

    inline uint AddressBlock(int nbr_block)
    {
        return (uint)(nbr_block * SizeBlock());
    }

    void WriteBuffer(uint address_ROM, const void *buffer, int size);

    void ReadBuffer(uint address, void *buffer, int size);

    // Перед buffer должно быть пять байт, которые будут испорчены этой процедурой. После отработки
    // их нужно восстановить в случае необходимости
    void ReadBufferFast(uint address, void *buffer, int size);

#ifdef MCU_GD
#else
    // !!! ВНИМАНИЕ !!! Функция портит 5 байт перед buffer. Вызывающая фукнция должна их сохранять перед вызовом
    // и восстанавливать в callback_on_complete
    void ReadBufferFastDMA(uint address, void *buffer, int size, void (*callback_on_complete)());
#endif

    int WriteBufferRelible512(uint address, const void *buffer, int size, int number_attempts = 10);

    void WriteInt16(uint address, int16 value);

    int16 ReadInt16(uint address);

    void WriteInt(uint address, int value);

    int ReadInt(uint address);

    // Сколько байт в наличии
    inline int Capasity()
    {
        return 1024 * 1024;
    }

    inline int GetBlocksCount()
    {
        return (int)(Capasity() / SizeBlock());
    }

    namespace Erase
    {
        // Стирает блок размером 4к
        void Block4kB(int nbr_block);

        void Full();

        void Firmware();

        // Стереть, если что-то записано
        void FirmwareIfNeed();

        void Sounds();
    }

    // Во внешней памяти хранятся некоторые настройки:
    // 1. Максимальная и минимальная допустимые температуры.
    namespace Settings
    {
        void Init();

        void SetTempMin(int8);
        void SetTempMax(int8);

        int8 GetTempMin();
        int8 GetTempMax();
    }

    // Функциии тестирования памяти. В прошивке не задействованы.
    // Могут быть подкллючены для тестов
    namespace Test
    {
        bool Run();

        // Тест скорости чтения. Возвращает количество байт в секунду
        float SpeedRead();
    }
}
