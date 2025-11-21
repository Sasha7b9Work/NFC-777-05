// 2023/12/17 15:49:07 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Communicator/AnswersOSDP.h"
#include "Utils/Math.h"
#include "Modules/LIS2DH12/LIS2DH12.h"
#include "Hardware/HAL/HAL.h"
#include "Hardware/Power.h"


void AnswerOSDP::Transmit()
{
    HAL_USART::OSDP::Transmit(buffer.Data(), buffer.Size());
}


void AnswerOSDP::AppendUInt16(uint16 data)
{
    AppendByte((uint8)data);
    AppendByte((uint8)(data >> 8));
}


void AnswerOSDP::AppendByte(uint8 data)
{
    buffer.Append(data);
}


void AnswerOSDP::AppendThreeBytes(uint data)
{
    for (int i = 0; i < 3; i++)
    {
        AppendByte((uint8)data);
        data >>= 8;
    }
}


void AnswerOSDP::AppendUInt(uint data)
{
    for (int i = 0; i < 4; i++)
    {
        AppendByte((uint8)data);
        data >>= 8;
    }
}


void AnswerOSDP::AppendUInt64(uint64 data)
{
    for (int i = 0; i < 8; i++)
    {
        AppendByte((uint8)data);
        data >>= 8;
    }
}


void AnswerOSDP::WriteUInt16(int address, uint16 data)
{
    buffer[address] = (uint8)data;
    buffer[address + 1] = (uint8)(data >> 8);
}


void AnswerOSDP::CommonStart(uint8 address, OSDP_ANS::E command)
{
    AppendByte(OSDP_SOM);                   // Начало посылки
    AppendByte((uint8)(address | 0x80));    // Куда шлём

    AppendUInt16(0);                        // Резервируем место для размера

    AppendByte(GetControlByte());           // Добавляем управляющий байт

    AppendByte((uint8)command);             // Номер команды
}


void AnswerOSDP::CommonEnd()
{
    AppendUInt16(0);                // Резервируем место для CRC

    WriteUInt16(2, (uint16)buffer.Size());

    WriteUInt16(buffer.Size() - 2, Math::CalculateCRC_OSDP(buffer.Data(), buffer.Size() - 2));
}


AnswerNumberCard::AnswerNumberCard(uint8 address, uint8 cntrl_code, uint64 number) :
    AnswerOSDP(address, OSDP_ANS::RAW, cntrl_code)
{
    AppendByte(1);                  // Reader Number
    AppendByte(0);                  // Format Code
    AppendUInt16(8 * 8);            // BitCount == 8 байт == uint64
    AppendUInt64(number);

    CommonEnd();
}


AnswerACK::AnswerACK(uint8 address, uint8 cntrl_code) :
    AnswerOSDP(address, OSDP_ANS::ACK, cntrl_code)
{
    CommonEnd();
}


AnswerNAK::AnswerNAK(uint8 address, uint8 cntrl_code, NAK::E nak) :
    AnswerOSDP(address, OSDP_ANS::NAK, cntrl_code)
{
    AppendByte((uint8)nak);

    CommonEnd();
}


AnswerPDID::AnswerPDID(uint8 address, uint8 cntrl_code, uint vendor, uint8 number, uint8 version, uint serial_number, uint firmware) :
    AnswerOSDP(address, OSDP_ANS::PDID, cntrl_code)
{
    AppendThreeBytes(vendor);
    AppendByte(number);
    AppendByte(version);
    AppendUInt(serial_number);
    AppendThreeBytes(firmware);

    CommonEnd();
}


AnswerCOM::AnswerCOM(uint8 address, uint8 cntrl_code) :
    AnswerOSDP(address, OSDP_ANS::COM, cntrl_code)
{
    AppendByte(gset.AddressOSDP());
    AppendUInt(gset.BaudRateOSDP().ToRAW());  

    CommonEnd();
}


AnswerCAP::AnswerCAP(uint8 address, uint8 cntrl_code) :
    AnswerOSDP(address, OSDP_ANS::PDCAP, cntrl_code)
{
    /*
    *  0 : 
    *  1 : 
    *  2 : 
    */

    AppendCode(4, 1, 1);        // LED Control
                                // 1 - the PD support on/off control only
                                // 2 - the PD supports timed commands
                                // 3 - like 02, plus bi-color LEDs
                                // 4 - like 02, plus tri-color LEDs

    AppendCode(5, 1, 1);        // Audible Output
                                // 1 - the PD support on/off control only
                                // 2 - the PD supports timed commands

    CommonEnd();
}


void AnswerCAP::AppendCode(uint8 code, uint8 compilance, uint8 number)
{
    AppendByte(code);           // Function Code
    AppendByte(compilance);     // compilance
    AppendByte(number);         // Number of
}


AnswerLSTATR::AnswerLSTATR(uint8 address, uint8 cntrl_code) :
    AnswerOSDP(address, OSDP_ANS::LSTATR, cntrl_code)
{
    AppendByte(LIS2DH12::IsAlarmed() ? 1U : 0U);

    AppendByte(Power::IsLessThanNormal() ? 1U : 0U);

    CommonEnd();
}


AnswerRSTAT::AnswerRSTAT(uint8 address, uint8 cntrl_code) :
    AnswerOSDP(address, OSDP_ANS::RSTATR, cntrl_code)
{
    // \todo Не реализовано
}
