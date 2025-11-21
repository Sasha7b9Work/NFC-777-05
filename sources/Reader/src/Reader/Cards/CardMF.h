// 2024/12/04 11:52:37 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once

/*
* 
*    арты Mifare
* 
*/


// ¬ картах Mifare данные расположены в блоках по 16 байт, которые, в свою очередь,
// размещаютс€ в секторах. ¬ первых 16-ти секторах размещаетс€ по 4 блока (по крайней
// мере, дл€ Mifare)
namespace CardMF
{
    bool ReadBlock(int block, Block16 &);

    bool WriteBlock(int block, const Block16 &);

    // ¬озвращает номер первого блока в секторе (вс€ пам€ть делитс€ на сектора с некоторым количеством блоков в каждом)
    int NumFirstBlockInSector(int sector);

    // ¬озвращает номер сектора, в котором находитс€ данный блок. Ќужно дл€ аутентификации
    int NumSectorForBlock(int block);

    // ¬озвращает местоположение в карте Mifare блока с адресацией NTAG
    // nbr_block - здесь будет номер блока Mifare
    // first_byte - здесь будет смещение первого байта блока NTAG в блоке Mifare. ќно кратно 4 и может быть 0, 4, 8, 12
    bool GetAddressBlockNTAG(int nbr_block_NTAG, int *nbr_block, int *first_byte);

    // ¬озвращает true, если это служебный блок с паролем
    // (кратен трЄм дл€ секторов 0...31, кратен 16 дл€ секторов 32...39
    bool BlockIsPassword(int nbr_block_MF);

    //  аждый раз после выключени€ пол€ нужно вызывать эту функцию, чтобы обнулить внутренее состо€ние карты дл€
    // операций чтени€/записи
    void ResetReadWriteOperations();
}
