// 2024/01/12 14:48:25 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


namespace SET2
{
    static const int SIZE_BUFFER_ACCESS = 512;

    void Test();

    // Биты разрешения карт для автномного режима
    namespace AccessCards
    {
        // Установить доступ для карт.
        // На входе - битовая карта в hex-формате. Идут байты в шестнадцатиричном виде:
        // "BA23FEAC37"
        void WriteToROM(const char bitmap_HEX[SIZE_BUFFER_ACCESS]);

        // Возвращает true, если у карты с номером number есть доступ
        bool Granted(uint64 number);

        void Clear();
    }

    // Сохранение/чтение ключей для перевода карт Mifare Plus из SL0 в SL3
    namespace KeysMFP
    {
        static const int NUM_KEYS = 5;

        void Init();

        // num = [0...4], что соотвествует 9000...90004
        Block16 *Get(int num);

        // Сохраняет в ROM новые ключи
        void WriteToROM(const Block16 *keys);

        void Reset();
    }
}
