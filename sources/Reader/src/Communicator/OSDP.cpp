// 2023/12/15 14:43:55 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Communicator/OSDP.h"
#include "Utils/Math.h"
#include "Communicator/AnswersOSDP.h"
#include "Modules/Player/Player.h"
#include "Device/Device.h"
#include "Hardware/Timer.h"
#include "Modules/Indicator/Indicator.h"
#include "Hardware/HAL/HAL.h"
#include "Communicator/Communicator.h"
#include "Reader/Reader.h"


/*
*   Реализовать:

 +  osdp_POLL       osdp_RAW                    // Запрос данных
    osdp_ID         osdp_PDID                   // Запрос отчёта ID
    osdp_CAP        osdp_PDCAP                  // Запрос возможностей периферийных устройств
    osdp_LSTAT      osdp_LSTATR                 // Запрос отчета о локальном статусе
    osdp_RSTAT      osdp_RSTATR                 // Запрос отчета о состоянии считывателя
    osdp_LED        osdp_ACK        osdp_NAK    // Команда управления светодиодом считывателя
    osdp_BUZ                                    // Команда управления зуммером считывателя
    osdp_COMSET     osdp_COM                    // Команда настройки связи

    osdp_ACK                                    // Общее признание, нечего сообщать
    osdp_NAK                                    // Отрицательное подтверждение — ответ об ошибке обработчика связи SIO
    osdp_PDID                                   // Отчет об идентификации устройства
    osdp_PDCAP                                  // Отчет об идентификации устройства
    osdp_LSTATR                                 // Отчет о локальном статусе
    osdp_RSTATR                                 // Reader Tamper Status Report
 +  osdp_RAW                                    // Отчет о данных карты, необработанный битовый массив
    osdp_COM                                    // Communication Configuration Report
    osdp_BUSY                                   // Переферийное устройство занято

*/



namespace OSDP
{
    struct StructOSDP
    {
        void (*func)(const BufferOSDP &);
        uint8 command;
    };

    static void RequestPoll(const BufferOSDP &);
    static void RequestID(const BufferOSDP &);
    static void RequestCAP(const BufferOSDP &);
    static void RequestLSTAT(const BufferOSDP &);
    void RequestRSTAT(const BufferOSDP &);
    static void RequestLED(const BufferOSDP &);
    static void RequestBUZ(const BufferOSDP &);
    static void RequestCOMSET(const BufferOSDP &);
    void RequestMFG(const BufferOSDP &);

    static StructOSDP handlers[] =
    {
        { RequestPoll,   OSDP_REQ_POLL },
        { RequestID,     OSDP_REQ_ID },
        { RequestCAP,    OSDP_REQ_CAP },
        { RequestLSTAT,  OSDP_REQ_LSTAT },
//        { RequestRSTAT,  OSDP_REQ_RSTAT },
        { RequestLED,    OSDP_REQ_LED },
        { RequestBUZ,    OSDP_REQ_BUZ },
        { RequestCOMSET, OSDP_REQ_COMSET },
        { RequestMFG,    OSDP_REQ_MFG },
        { nullptr,       0xFF }
    };

    namespace Card
    {
        static bool exist_number = false;
        static uint64 number = 0;

        void Insert(uint64 _number)
        {
            number = _number;
            exist_number = true;
        }
    }

    static bool is_enabled = false;

    bool IsEnabled()
    {
        return is_enabled;
    }
}


void OSDP::BufferOSDP::SendAnswerNAK(NAK::E nak) const
{
    AnswerNAK(address, cntrl_code, nak).Transmit();
}


void OSDP::BufferOSDP::RemoveFirstBytes(int num_bytes)
{
    buffer.RemoveFirst(num_bytes);
    bytes_left -= num_bytes;
}


void OSDP::BufferOSDP::Process()
{
    if (AllDatas())                         // У нас есть все байты команды
    {
        if (ProcessCheckSumAndAddress())    // Берём адрес и проверяем, совпадает ли контрольная сумма
        {
            DeleteBytesForCommand();        // Удаляем все байты до команды. После выполнения нулевой байт в буфере - код команды

            ProcessRequest();               // И непосредственно обрабатываем запрос
        }
    }
}


bool OSDP::BufferOSDP::AllDatas()
{
    bytes_left = (int)Block2(buffer[2], buffer[3]).u16;

    return (buffer.Size() >= bytes_left);           // Если true, то в буфере всё сообщение
}


bool OSDP::BufferOSDP::ProcessCheckSumAndAddress()
{
    address = buffer[1];

    if (address != gset.AddressOSDP() && address != 0x7F)
    {
        RemoveFirstBytes(bytes_left);

        return false;                                            // Сообщение предназначено не нам
    }

    Block2 crc(buffer[bytes_left - 2], buffer[bytes_left - 1]);

    if (Math::CalculateCRC_OSDP(buffer.Data(), bytes_left - 2) == crc.u16)
    {
        return true;
    }

    cntrl_code = buffer[4];

    SendAnswerNAK(NAK::_ERROR_CRC_);

    RemoveFirstBytes(bytes_left);

    return false;
}


void OSDP::BufferOSDP::DeleteBytesForCommand()
{
    cntrl_code = buffer[4];

    int bytes_for_delete = 5;               // Пятым байтом идёт команда, если нету Security Control Block

    if (_GET_BIT(cntrl_code, 3))            // Присутствует Security Control Block
    {
        bytes_for_delete += buffer[5];      // Тогда в пятом байте хранится длина SCB целиком - 
    }                                       // со служебными байтами

    RemoveFirstBytes(bytes_for_delete);   // Удаляем все байты до команды
}


void OSDP::BufferOSDP::ProcessRequest()
{
    StructOSDP *handler = &handlers[0];

    uint8 command = buffer[0];

    while (handler->func)
    {
        if (handler->command == command)
        {
            RemoveFirstBytes(1);          // Удаляем код команды из буфера. В буфере лишь параметры запроса

            handler->func(*this);

            break;
        }

        handler++;
    }

    if (!handler->func)                         // Команда не реализована
    {
        SendAnswerNAK(NAK::COMMAND_NOT_IMPLEMENTED);
    }

    RemoveAllBytes();
}


void OSDP::Update(BufferUSART &data)
{
    if (data[0] == OSDP_SOM)
    {
        if(data.Size() > 7)             // 8 - минимальная длина сообщения :
        {                               // 0     : SOM
            BufferOSDP buffer(data);    // 1     : ADDR
                                        // 2...3 : LEN
            buffer.Process();           // 4     : CTRL
        }                               // 5     : CMND
                                        // 6...7 : CRC
    }
    else
    {
        data.RemoveFirst(1);
    }
}


void OSDP::RequestPoll(const BufferOSDP &buffer)
{
    if (Card::exist_number)
    {
        Card::exist_number = false;
        AnswerNumberCard(buffer.Address(), buffer.ControlCode(), Card::number).Transmit();
    }
    else
    {
        AnswerACK(buffer.Address(), buffer.ControlCode()).Transmit();
    }
}


void OSDP::RequestID(const BufferOSDP &buffer)
{
    AnswerPDID(
        buffer.Address(),
        buffer.ControlCode(),
        Device::Info::Hardware::Vendor(),
        Device::Info::Hardware::ManufacturedModelNumber(),
        Device::Info::Hardware::ManufacturedVersion(),
        Device::Info::Hardware::SerialNumber_UINT(),
        (uint)(VERSION_MAJOR + (VERSION_MINOR << 8) + (VERSION_BUILD << 16))
    ).Transmit();
}


void OSDP::RequestCAP(const BufferOSDP &buffer)
{
    AnswerCAP(buffer.Address(), buffer.ControlCode()).Transmit();
}


void OSDP::RequestLSTAT(const BufferOSDP &buffer)
{
    AnswerLSTATR(buffer.Address(), buffer.ControlCode()).Transmit();
}


void OSDP::RequestRSTAT(const BufferOSDP &buffer)
{
    AnswerRSTAT(buffer.Address(), buffer.ControlCode()).Transmit();
}


void OSDP::RequestLED(const BufferOSDP &buffer)
{
    AnswerACK(buffer.Address(), buffer.ControlCode()).Transmit();

    struct Color colors[8] =
    {
        Color(0x000000, 0.0f),  // Black
        gset.ColorRed(),        // Red
        gset.ColorGreen(),      // Green
        Color(0xFFFF00),        // Amber
        Color(0x0000FF),        // Blue
        Color(0xFF00FF),        // Magenta Пурпурный
        Color(0x00FFFF),        // Cyan    Голубой
        Color(0xFFFFFF)         // White
    };

    BufferUSART &message = buffer.buffer;

    Indicator::TaskOSDP::Set(
        (Indicator::TaskOSDP::TemporaryControlCode::E)message[2],
        (uint)(message[3] * 100),
        (uint)(message[4] * 100),
        colors[message[5]],
        colors[message[6]],
        Block2(message[7], message[8]).u16,
        (Indicator::TaskOSDP::PermanentControlCode::E)message[9],
        (uint)(message[10] * 100),
        (uint)(message[11] * 100),
        colors[message[12]],
        colors[message[13]]
    );
}


void OSDP::RequestBUZ(const BufferOSDP &buffer)
{
    AnswerACK(buffer.Address(), buffer.ControlCode()).Transmit();

    BufferUSART &message = buffer.buffer;

    uint8 sound = message[1];

    if (sound > 2)
    {
        sound = 0;
    }

    uint time_on = (uint)(message[2] * 100);
    uint time_off = (uint)(message[3] * 100);
    int count = message[4];

    Player::Task::Set((TypeSound::E)sound, time_on, time_off, count);
}


void OSDP::RequestCOMSET(const BufferOSDP &buffer)
{
    BufferUSART &message = buffer.buffer;

    gset.SetAddressOSDP(message[0]);
    gset.SetBaudRateOSDP(BaudRate::FromUInt((uint)(message[1] | (message[2] << 8) | (message[3] << 16) | (message[4] << 24))));

    SettingsMaster::Save();

    AnswerCOM(buffer.Address(), buffer.ControlCode()).Transmit();

    HAL_USART::Init();
}


void OSDP::Enable()
{
    is_enabled = true;

    ModeReader::Set(ModeReader::UART);
    HAL_USART::EnableFiltering(false);
    HAL_USART::Init();

    for (int i = 0; i < 3; i++)
    {
        Indicator::Blink(Color(0x0000FF, 1.0f), Color(0, 0), 500, false);
    }

    HAL_USART::Init();
}


void OSDP::AntibreakAlarm()
{
    // \todo
    // Передастся автоматом в ответ на запрос LSTAT
}
