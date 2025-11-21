// 2024/12/16 07:58:25 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


/*

    Функции идентификации типа.
    Используются в том числе и для определения типа вставленной карты.

*/


namespace TypeCard
{
    enum E
    {
        Unidentified,   // Неизвестная карта
        NTAG213,
        NTAG215,
        NTAG216,        //                      14443-3
        MFC,            // Mifare Classic       14443-3
        MFC_EV1,        // Mifare Classic EV1   14443-3

        // Все карты, начиная отсюда, поддерживают T=CL

        MFP_S,          // Mifare Plus S        14443-4   T=CL

        // После этого идут карты с уровнем безопасности 3

        MFP_X,          // Mifare Plus X        14443-4   T=CL
        MFP_SE,         // Mifare Plus SE       14443-4   T=CL
        MFP_EV1,        // Mifare Plus EV1      14443-4   T=CL
        MFP_EV2,        // Mifare Plus EV2      14443-4   T=CL
        MFP,            // Какая-то Mifare Plus (если нельзя определить точный тип)
        Count
    };

    // Определить тип карты
    bool Detect();

    E Current();

    // Сброс типа карты при изьятии её из картоприёмника
    void Reset();

    // Возвращает название типа карты
    pchar ToShortASCII();
    pchar ToLongASCII();

    bool IsNTAG();

    // Mifare Classic, Mifare Plus S, Mifare Plus X
    bool IsMifare();

    // Поддерживает протокол 14443-4 - Mifare Plus S, Mifare Plus X
    bool IsSupportT_CL();

    // Поддерживает шифрованную запись - Mifare Plus X
    bool IsReadEncripted();

    namespace NTAG
    {
        int NumberBlockCFG1();
    };
}

