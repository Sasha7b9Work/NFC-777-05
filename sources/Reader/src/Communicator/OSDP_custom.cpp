// 2025/05/08 10:25:30 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Communicator/OSDP.h"
#include "Communicator/AnswersOSDP.h"


// Здесь пользовательские команды


namespace OSDP
{
    struct Command
    {
        enum E
        {
            SETWGFMT = 0x01,        // Установка параметров Wiegand
            GETWGFMT = 0x02,        // Запрос параметров Wiegand
            SETAUTHKEY = 0x03,      // Установка ключа аутентификации
            CHGCOMSPEED = 0x04,     // Изменение скорости работы на текущий сеанс
            SETADDR = 0x81
        };
    };

    struct Answer
    {
        enum E
        {
            RDWGFMT = 0x02
        };
    };

    // Эта функция вызывается, когда приходит команда OSDP_REQ_MFG
    void RequestMFG(const BufferOSDP &);

    static void OnWiegandSet(const BufferOSDP &);
    static void OnWiegandGet(const BufferOSDP &);
    static void OnSetAuthKey(const BufferOSDP &);
    static void OnChangeSpeedCOM(const BufferOSDP &);
    static void OnSetAddress(const BufferOSDP &);

    struct StructOSDP
    {
        void (*func)(const BufferOSDP &);
        uint8 command;
    };

    static StructOSDP handlers[] =
    {
        { OnWiegandSet,     Command::SETWGFMT },
        { OnWiegandGet,     Command::GETWGFMT },
        { OnSetAuthKey,     Command::SETAUTHKEY },
        { OnChangeSpeedCOM, Command::CHGCOMSPEED },
        { OnSetAddress,     Command::SETADDR },
        { nullptr, 0xFF }
    };
}


void OSDP::RequestMFG(const BufferOSDP &buffer)
{
    StructOSDP *handler = &handlers[0];

    Command::E command = (Command::E)buffer.buffer[0];

    while (handler->func)
    {
        if (handler->command == command)
        {
            handler->func(buffer);      // ВНИМАНИЕ! В отличие от стандартных команд сейчас в буфере нулевым байтом идёт код команды

            break;
        }

        handler++;
    }

    if (!handler->func)                         // Команда не реализована
    {
        buffer.SendAnswerNAK(NAK::COMMAND_NOT_IMPLEMENTED);
    }
}


void OSDP::OnWiegandSet(const BufferOSDP &data)
{
    int num_bits = data.buffer[1];
    bool control_bits = data.buffer[2] != 0;
    bool inverse_control_bits = data.buffer[3] != 0;
    bool full_GUID = data.buffer[4] != 0;
    bool delete_MSB = data.buffer[5] != 0;              // Отбрасывать старший байт в UID4
    bool inverse_order_bits = data.buffer[6] != 0;

    uint value = (uint)num_bits;                    // В младших 7 битах количество передаваемых бит (вместе с контрольными)

    if (full_GUID)
    {
        _SET_BIT(value, 8);
    }

    if (control_bits)
    {
        _SET_BIT(value, 9);
    }

    if (inverse_control_bits)
    {
        _SET_BIT(value, 10);
    }

    if (inverse_order_bits)
    {
        _SET_BIT(value, 11);
    }

    if (delete_MSB)
    {
        _SET_BIT(value, 12);
    }

    gset.SetWiegand(value);
}


void OSDP::OnWiegandGet(const BufferOSDP &buffer)
{
    AnswerOSDP_USER_SetWiegand(buffer.Address(), buffer.ControlCode()).Transmit();
}


void OSDP::OnSetAuthKey(const BufferOSDP &)
{

}


void OSDP::OnChangeSpeedCOM(const BufferOSDP &)
{

}


void OSDP::OnSetAddress(const BufferOSDP &)
{

}


AnswerOSDP_USER_SetWiegand::AnswerOSDP_USER_SetWiegand(uint8 addr, uint8 cntrl_code) :
    AnswerOSDP_USER(addr, OSDP::Command::GETWGFMT, cntrl_code)
{
    AppendByte(gset.GetWiegandValue());
    AppendByte((uint8)(gset.IsWiegandControlBitsEnabled() ? 1 : 0));
    AppendByte((uint8)(gset.IsWiegandControlBitsParityStandard() ? 0 : 1));
    AppendByte((uint8)(gset.IsWiegandFullGUID() ? 1 : 0));
    AppendByte((uint8)(gset.IsWiegnadDiscard_NUID_LSB() ? 1 : 0));
    AppendByte((uint8)(gset.IsWiegandReverseOrderBits() ? 1 : 0));

    CommonEnd();
}
