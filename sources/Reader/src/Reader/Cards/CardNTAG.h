// 2024/12/04 12:02:48 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


/*

    Карты NTAG

*/


namespace CardNTAG
{
    bool PasswordAuth();

    bool ReadBlock(int nbr_block, uint8[4]);

    bool WriteBlock(int nbr_block, const uint8[4]);
}
