// 2023/09/05 12:49:31 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


struct TypeAuth;


namespace Reader
{
    void Init();

    void Update();

    void SetAuth(const TypeAuth &);

    TypeAuth GetAuth();
}


// Чем управляются сигналы светодиодов, громкоговорителя - внутренним состоянием
// (Internal) или внешним воздействием (External)
namespace ModeSignals
{
    bool IsInternal();

    bool IsExternal();
}


// Это режим, в котором считыватель лишь выдаёт управляющие сигналы на замок и сирену, основываясь на
// битовой карте, занесённой в память.
namespace ModeOffline
{
    void Update();
    void Enable();
    bool IsEnabled();

    // Открыть замок на количество секунд, указанное в настройках
    void OpenTheLock();

    // Включить сигнал тревоги на количестов секнуд, указанное в настройках
    void EnableAlarm();

    uint MaxNumCards();
}


namespace ModeWG
{
    // А в этом режиме посылается GUID
    void EnableGUID();

    // В этом режиме посылается номер карты
    bool IsNumber();
};


struct ModeReader
{
    enum E
    {
        WG,
        UART,
        Read,
        Write,
        Count
    };

    static void Set(E);

    static E Current() { return current; }

    static pchar Name(E);

    static bool IsRead() { return current == Read; }

    // Это режим "Read", дополненный функциями записи на карты
    static bool IsWrite() { return current == Write; }

    static bool IsWG() { return current == WG; }

    static bool IsUART() { return current == UART; }

    // Выводить ли дополнительные сообщения
    static bool IsExtended() { return IsRead() || IsWrite(); }

    // Находимся в нормальном режиме - определение мастер- и пользовательских карт
    static bool IsNormal() { return IsWG() || IsUART(); }

private:
    static E current;
};


// Если "питание" отключено, то карта не обрабатывается
namespace ReaderPowered
{
    void On();
    void Off();
    bool IsEnabled();
}
