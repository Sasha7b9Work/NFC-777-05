// 2023/09/03 15:57:58 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Utils/String.h"


/*
*   Карты памяти карт
*   Общий принцип размещения SetttingsMaster по секторам :
*   BitSet32 от SettingsMaster размещаются последовательно во всех свободных областях, начиная с
*   первого байта, доступного для записи. Области Key B не используются.
*/

/*
    Mifare Classic UID4 1k

    16 секторов по 4 блока по 16 байт
    Каждый 4-й блок служебный (Sector Trailer), в котором находятся два ключа (А и B) и биты доступа
    В нулевом блоке находится служебная информация - UID и Manufactured Data.

    Размещение SettingsMaster

    +-----------+---------------------------------------------------------+
    | Блок MF   | Блоки SettingsMaster                                    |
    +-----------+---------------------------------------------------------+
    |  0:1 ( 1) | s04...s07                                               |
    |  0:2 ( 2) | s08...s11                                               |
    +-----------+                                                         +
    |  1:0 ( 4) | s12...s15                                               |
    |  1:1 ( 5) | s16...s19                                               |
    |  1:2 ( 6) | s20... 23 Здесь уже начинается битовая карта с блока 21 |
    +-----------+                                                         +
    |  2:0 ( 8) |  24... 27                                               |
    |  2:1 ( 9) |  28... 31                                               |
    |  2:2 (10) |  32... 35                                               |
    +-----------+                                                         +
    |  3:0 (12) |  36....39                                               |
    +-----------+---------------------------------------------------------+
*/


struct SettingsMaster;
struct SAK;
struct ATS;
struct UID;
struct CardSize;
struct TypeAuth;


namespace Card
{
    extern UID      uid;                    // UID карты, с которой работаем
    extern uint16   atqa;                   // ATQA карты, с которой работаем
    extern SAK      sak;                    // SAK карты, с которой работаем
    extern ATS      ats;                    // ATS карты, с которой работаем
    extern CardSize size_card;              // Для Mifare
    extern int      SL;                     // Security Level карты, с которой работаем (для Mifare Plus)
    extern int      number_block_T_CL;      // Для работы с картами Mifare Plus
    extern bool     null_bytes_received;    // Если на последний запрос принято ноль байт ответа. Нужно для проверки того, есть ли связь с картой

    void SetPasswordAuth(const Block16 &);
    Block16 GetPasswordAuth();

    // Вставить в рабочем режиме
    void InsertNormalModeUser(uint64 number, bool auth_ok);
    void InsertNormalModeMaster(bool auth_ok);

    // Вставить в расширенном режиме
    void InsertExtendedMode(const TypeAuth &, bool auth_ok, bool new_auth, uint64 number);

    // Если generate_event, то посыалем событие в UART
    void Eject(bool generate_event_UART);

    bool IsInserted();

    // Аутентификация паролем вставленной карты
    bool PasswordAuth();

    // Записать пароль в предназначенные системные сектора на карте
    bool WriteKeyPassword(const Block16 &password);

    // Включить проверку пароля
    bool EnableCheckPassword();

    // Читает номер пользовательсой карты
    bool ReadNumber(uint64 *);

    // Записывает номер пользовательской карты
    bool WriteNumber(uint64);

    bool WriteSettings(const SettingsMaster &);

    // Читать карту разрешённых карт из мастер-карты
    // На выходе последовательность байт
    bool ReadBitmapCards(char bitmap_cards_HEX[500]);

    // Записать карту разрешённых карт в мастер-карту
    bool WriteBitmapCards(const char bitmap_cards[160]);

    // Полная информация о карте
    String<128> BuildCardInfoFull();

    // Сокращеённая информация о карте для передачи в CIC
    String<64> BuildCardInfoShort();

    // Здесь параметры (номера блоков) приведены к карте NTAG
    namespace HI_LVL
    {
        bool ReadBlock(int nbr_block_NTAG, Block4 &);

        bool Read2Blocks(int nbr_block_NTAG, Block8 *);

        // size должно быть кратно 4
        bool ReadData(int nbr_block_NTAG, void *data, int size);

        // nbr_block_NTAG = [0x02...0x2c]
        bool WriteBlock(int nbr_block_NTAG, const uint8[4]);

        // Если verify == false - не проверять правильность чтения - записываем пароль
        bool Write2Blocks(int nbr_block_NTAG, const uint8[8]);

        // size должно бтыь кратно 4
        bool WriteData(int nbr_block_NTAG, const void *data, int size);
    }

    namespace _14443
    {
        // Заслать данные в карту и инициировать ожидание ответа
        bool Transceive(const uint8 *tx, int len_tx, bool crc_tx, bool crc_rx);

        // Возвращает ноль в случае ошибки. Иначе - количество принятых байт
        int TransceiveReceive(const uint8 *tx, int len_tx, uint8 *rx_buf, int rx_size, bool crc_tx, bool crc_rx);

        bool RATS();

        // Здесь подмножестов команд 14443-4, на котором работают карты Mifare Plus
        namespace T_CL
        {
            // Возвращает true, если команда поддерживается картой
            // По указателю verHW вовращается версия аппаратного обеспечения из ответа
            bool GetVersion(uint8 answer[32], int *bytes_received);
            bool DetectCetVersion(bool sak_5_is_true);

            // В tx не резервируется место для CRC
            // Возвращает 0 в случае ошибки. Иначе - количество принятых байт
            int TransceiveReceive(const uint8 *tx, int len_tx, uint8 *rx, int len_rx);

            // Находимся в SL0
            bool InSL0();

            bool WriteKeys(const Block16 *keys, int num_keys);

            // Переключение в SL3 из SL0 или SL3
            bool SwitchToSL3();

            // Должно вызываться перед каждой следующей первой аутентификацией
            bool ResetAuth();

            // nbr_block - блок, который нужно прочитать
            // Сюда же можно подавать номер ключа AES, который надо изменить
            bool FirstAuthentication(int nbr_block, const Block16 &key);

            bool FollowingAutentication(int nbr_block, const Block16 &key);

            // nbr_keyAES = 0x4000...0x403F
            // Если результат - true, то пароль изменён.
            // Если результат - false, то пароль либо остался старым, либо изменился на новый.
            bool ChangeKeyAES(int nbr_keyAES, const Block16 &old_key, const Block16 &new_key);

            // Авторизация к этому моменту должна быть пройдена
            // Если карта поддерживает шифрование на чтении, то оно включается автоматически
            // 31 : encripted data on, MAC on - Mifare Plus X
            // 33 : encripted data off, MAC on - Mifare Plus S
            bool ReadBlock(int nbr_block, Block16 &);

            // Авторизация к этому моменту должна быть пройдена
            // Если идёт запись ключа - то запись в режиме Encripted
            // Если запись блока данных, то режим записи в зависимости от того, поддерживат ли карта шифрование при записи
            // A1 : encripted data on, MAC on - Mifare Plus X
            // A3 : encripted data off, MAC on - Mifare Plus S
            bool WriteBlock(int nbr_block, const Block16 &);
        }
    }

    static const uint timeout = 100;
}
