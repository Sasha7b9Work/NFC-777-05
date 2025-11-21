// 2023/09/18 09:16:54 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/Events.h"
#include "Reader/Reader.h"
#include "Hardware/HAL/HAL.h"
#include "Modules/Indicator/Indicator.h"
#include "Modules/Player/Player.h"
#include "Communicator/OSDP.h"
#include "Communicator/Communicator.h"
#include "Reader/Cards/CardNTAG.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"


namespace Event
{
    static UID last_readed_uid;
}


void Event::CardDetected()
{
    if (ModeReader::IsExtended())
    {
        HAL_USART::UART::SendStringF("#CARD DETECTED UID=%s TYPE=%s", Card::uid.ToASCII().c_str(), Card::BuildCardInfoShort().c_str());
    }
    else
    {
        if (!ModeOffline::IsEnabled())
        {
            Indicator::Blink(Color(0x0000FF, 1.0f), Color(0, 0.0f), 50, true);
        }

        if (OSDP::IsEnabled())
        {
            Indicator::TaskOSDP::FirePermanentColor();
        }
    }
}


void Event::CardReadOK(const UID &uid, uint64 number)
{
    last_readed_uid = uid;

    if (ModeReader::IsExtended())
    {
        if ((number & 0xFFFFFFFF) == 0xFFFFFFFF)                            // Прочитана мастер-карта
        {
            char message[256];

            std::sprintf(message, "#CARD READ %s TYPE=%s AUTH OK MASTER ",
                uid.ToASCII().c_str(),
                Card::BuildCardInfoShort().c_str());

            SettingsMaster settings;

            Card::HI_LVL::ReadData(4, (uint8 *)&settings, sizeof(settings));

            uint *pointer = (uint *)&settings;

            for (uint i = 0; i < sizeof(settings) / sizeof(uint); i++)
            {
                char buffer[32];

                std::sprintf(buffer, "%08X", *pointer);
                std::strcat(message, buffer);

                pointer++;
            }

            std::strcat(message, "\n");

            HAL_USART::UART::SendStringRAW(message);
        }
        else                                                                // Прочитана пользовательская карта
        {
            HAL_USART::UART::SendStringF("#CARD READ %s TYPE=%s AUTH OK NUMBER=%llu",
                uid.ToASCII().c_str(),
                Card::BuildCardInfoShort().c_str(),
                number
            );
        }
    }
}


void Event::CardReadFAIL(const UID &uid)
{
    last_readed_uid = uid;

    if (ModeReader::IsExtended())
    {
        HAL_USART::UART::SendStringF("#CARD READ %s AUTH FAIL", uid.ToASCII().c_str());
    }
}


void Event::CardRemove()
{
    if (ModeReader::IsExtended())
    {
        HAL_USART::UART::SendStringF("#CARD REMOVE %s", last_readed_uid.ToASCII().c_str());
    }
}
