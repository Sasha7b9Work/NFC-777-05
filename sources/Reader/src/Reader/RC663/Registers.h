// 2022/7/6 9:44:07 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


namespace RC663
{
    namespace Timer
    {
        void SetControl(uint8 timer, uint8 value);
        void SetReload(uint8 timer, uint16 value);
        void SetValue(uint8 timer, uint16 value);
    }

    namespace FIFO
    {
        void Init();

        void Clear();

        void Push(uint8);

        uint8 Pop();

        void Flush();

        int Length();

        int Read(uint8 *buffer, int length);

        void Write(const uint8 *buffer, int lenght);
    };

    namespace IRQ0
    {
//        static const int Set = (1 << 7);
//        static const int HiAlertIRQ = (1 << 6);
//        static const int LoAlertIRQ = (1 << 5);
        static const int IdleIRQ = (1 << 4);
//        static const int TxIRQ = (1 << 3);
//        static const int RxIRQ = (1 << 2);
        static const int ErrIRQ = (1 << 1);
//        static const int RxSOFIRQ = (1 << 0);

        void Clear();

        uint8 GetValue();

//        // Возвращает true, если "железячный" вывод IRQ сигнализирует о готовности данных
//        bool DataReadyHardware();
    };

    namespace IRQ1
    {
        void Clear();

        uint8 GetValue();
    };

    struct RegRxAna
    {
        void Write(uint8);
        // Запомнить текущее значение и записать новое
        void StoreAndWrite(uint8);
        // Восстановить ранее запомненное значение
        void Restore();

    private:
        uint8 stored = 0;
    };

    extern RegRxAna regRxAna;

    struct Register
    {
        Register(uint8 _address, int _data = 0) : address(_address), data(_data) { }
        void Write();
        void Write(uint8 data);
        uint8 Read();
        void ClearBit(int);
        void SetBit(int);
        void Read(const uint8 out[2], uint8 in[2]);
        uint8 address;
        int data;
    };


    struct RegError : public Register
    {
        static const int CollDet = (1 << 2);

        RegError() : Register(0x0A) { }
    };
}
