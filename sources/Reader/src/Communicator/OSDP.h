// 2023/12/15 14:44:07 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Communicator/Communicator.h"


/*
*   Примеры сообщений

*   osdp_POLL
*       0xFF
*       0x53    SOM
*       0x01    ADDRESS
*       0x08    LEN_LSB     LEN == 8
*       0x00    LEN_MSB
*       0x04    CTRL        CRC16
*       0x60    osdp_POLL
*       0xBA    CRC_LSB
*       0x00    CRC_MSB
*/


struct NAK
{
    enum E
    {
        _NONE,
        _ERROR_CRC_,                    // Message check character(s) error (bad cks/crc/mac[4])
        ERROR_COMMAND_LENGTH,           // Command length error
        COMMAND_NOT_IMPLEMENTED,        // Unknown Command Code – Command not implemented by PD
        ERROR_HEADER,                   // Unexpected sequence number detected in the header
        NOT_SUPPORTED_SECURRITY_BLOCK,  // This PD does not support the security block that was received
        REQUIRED_ENCRYPTED,             // Encrypted communication is required to process this command
        NOT_SUPPORTED_BIO_TYPE,         // BIO_TYPE not supported
        NOT_SUPPORTED_BIO_FORMAT        // BIO_FORMAT not supported
    };
};


// Запросы
#define OSDP_SOM        0x53

#define OSDP_REQ_POLL   0x60
#define OSDP_REQ_ID     0x61
#define OSDP_REQ_CAP    0x62
#define OSDP_REQ_LSTAT  0x64
#define OSDP_REQ_RSTAT  0x67
#define OSDP_REQ_LED    0x69
#define OSDP_REQ_BUZ    0x6A
#define OSDP_REQ_COMSET 0x6E
#define OSDP_REQ_MFG    0x80    // Используется для пользовательских команд


// Ответы
struct OSDP_ANS
{
    enum E
    {
        ACK    = 0x40,
        NAK    = 0x41,
        PDID   = 0x45,
        PDCAP  = 0x46,
        LSTATR = 0x48,
        RSTATR = 0x4B,
        RAW    = 0x50,
        COM    = 0x54,
        BUSY   = 0x79,
        MFGREP = 0x90       // Используетсяд для пользовательских ответов
    };
};


namespace OSDP
{
    // Инициализация OSDP. Вызывается, если при включении определено, что работать нужно в режиме OSDP
    void Enable();

    bool IsEnabled();

    // Эта функция должна вызываться в главном цикле для обработки входящей информации
    void Update(BufferUSART &);

    // Срабатывание датчика положения
    void AntibreakAlarm();

    namespace Card
    {
        // Эта функция должна вызываться при обнаружении пользовательской карты для передачи номера
        void Insert(uint64 number);
    }

    struct BufferOSDP
    {
        BufferOSDP(BufferUSART &buf) : buffer(buf), bytes_left(-1), cntrl_code(0), address(0)
        {
        }

        // Обработка байт, находящихся в буфере.
        // После обработки из BufferUSART &buf конструктора удалены обработанные байты
        void Process();

        uint8 ControlCode() const
        {
            return cntrl_code;
        }
        uint8 Address() const
        {
            return address;
        }

        // Удаление всех байт запроса
        void RemoveAllBytes()
        {
            RemoveFirstBytes(bytes_left);
        }

        void SendAnswerNAK(NAK::E nak) const;

        void RemoveFirstBytes(int);

        BufferUSART &buffer;

    private:

        int     bytes_left;    // Осталось байт из принятого сообщения
        uint8   cntrl_code;
        uint8   address;

        // В буфере все данные для обработки
        bool AllDatas();

        // Если контрольная сумма не совпадает, то посылает ответ об этом и очищает буфер и возвращает false.
        // Если соощение предназначено не нам, то просто игнорируем
        bool ProcessCheckSumAndAddress();

        // Удалить все байты до команды.
        void DeleteBytesForCommand();

        // Теперь конкретно обрабатываем запрос
        void ProcessRequest();
    };
}
