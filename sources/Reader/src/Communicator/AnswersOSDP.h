// 2023/12/17 15:49:19 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Utils/Buffer.h"
#include "Communicator/OSDP.h"


/*
* Родительский класс для всех ответов OSDP
*/
struct AnswerOSDP
{
    AnswerOSDP(uint8 addr, OSDP_ANS::E ans, uint8 recv_control_byte)
    {
        // Формируем ответный контрольный байт в соотвествии с п3.4 Open Supervised Device Protocol (OSDP) Version 2.1.5 от September 2012

        control_byte = (uint8)(recv_control_byte & 3);  // SQN The sequence number of the message is used for message delivery confirmation and for error recovery

        _SET_BIT(control_byte, 2);                      // Признак того, что 16-bit CRC is contained in the last 2 bytes of the message

        CommonStart(addr, ans);
    }

    void Transmit();

protected:

    Buffer<uint8, 256> buffer;

    uint8 GetControlByte()
    {
        return control_byte;
    }

    // Общая завершающая последовательность
    void CommonEnd();

    // Добавить в конец буфера
    void AppendByte(uint8);
    void AppendUInt16(uint16);
    void AppendThreeBytes(uint);
    void AppendUInt(uint);
    void AppendUInt64(uint64);

    // Записать по определённому адресу в буфер
    void WriteUInt16(int address, uint16);

private:

    uint8 control_byte;     // Контрольный байт, который будем добавлять в ответ

    // Общая стартовая последовательность
    void CommonStart(uint8 address, OSDP_ANS::E);
};


struct AnswerACK : public AnswerOSDP
{
    AnswerACK(uint8 address, uint8 cntrl_code);
};


struct AnswerNAK : public AnswerOSDP
{
    AnswerNAK(uint8 address, uint8 cntrl_code, NAK::E);
};


// Номер карты будем посылать в ответ на REQ POLL
struct AnswerNumberCard : public AnswerOSDP
{
    AnswerNumberCard(uint8 address, uint8 cntrl_code, uint64 number);
};


// Ответ на REQ ID
struct AnswerPDID : public AnswerOSDP
{
    AnswerPDID(uint8 address, uint8 cntrl_code, uint vendor, uint8 number, uint8 version, uint serial_number, uint firmware);
};


// Ответ на REQ CAP
struct AnswerCAP : public AnswerOSDP
{
    AnswerCAP(uint8 address, uint8 cntrl_code);

private:

    void AppendCode(uint8 code, uint8 compilance = 1, uint8 number = 1);
};


// Ответ на REQ LSTAT
struct AnswerLSTATR : public AnswerOSDP
{
    AnswerLSTATR(uint8 address, uint8 cntrl_code);
};


// Ответ на REQ RSTAT
struct AnswerRSTAT : public AnswerOSDP
{
    AnswerRSTAT(uint8 address, uint8 cntrl_code);
};


// Ответ на REQ COMSET
struct AnswerCOM : public AnswerOSDP
{
    AnswerCOM(uint8 address, uint8 cntrl_code);
};


// Родительский класс для пользовательских ответов
struct AnswerOSDP_USER : public AnswerOSDP
{
    AnswerOSDP_USER(uint8 addr, uint8 command_user, uint8 cntrl_code) :
        AnswerOSDP(addr, OSDP_ANS::MFGREP, cntrl_code)
    {
        AppendByte(command_user);
    }
};


struct AnswerOSDP_USER_SetWiegand : public AnswerOSDP_USER
{
    AnswerOSDP_USER_SetWiegand(uint8 addr, uint8 cntrl_code);
};
