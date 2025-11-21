// 2023/09/05 12:49:45 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/Reader.h"
#include "Reader/RC663/RC663.h"
#include "Settings/Settings.h"
#include "Hardware/Timer.h"
#include "Hardware/HAL/HAL.h"
#include "Hardware/Power.h"
#include "Modules/Indicator/Indicator.h"
#include "Modules/Player/Player.h"
#include "Reader/RC663/LPCD.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"


ModeReader::E ModeReader::current = ModeReader::WG;


namespace ReaderPowered
{
    static bool powered = true;

    void On()
    {
        powered = true;
    }

    void Off()
    {
        powered = false;
    }

    bool IsEnabled()
    {
        return powered;
    }
}


namespace ModeOffline
{
    enum E
    {
        Enabled,
        Disabled
    };

    static E current_mode = Disabled;
    static uint time_lock = 0;          // Время открытия замка
    static uint time_alarm = 0;         // Время включения сирены
}


namespace ModeWG
{
    enum E
    {
        Normal,     // Пересылаем номер карты
        OnlyGUID    // Пересылаем GUID карты
    };

    static E current_mode = Normal;

    bool IsNumber()
    {
        return current_mode == Normal;
    }

    void EnableGUID()
    {
        current_mode = OnlyGUID;

        for (int i = 0; i < 3; i++)
        {
            Indicator::Blink(Color(0xFF0000, 1.0f), Color(0, 0), 500, false);
        }
    }
}


namespace Reader
{
    static TypeAuth type_auth(false, 0);

    static bool new_auth = false;
}


void Reader::Init()
{
    RC663::PreparePoll();

    RC663::LPCD::Calibrate();

    RC663::LPCD::Enter();
}


void Reader::Update()
{
    static uint next_time = 0;      // Следующе время, когда можно обновлять

    if (TIME_MS < next_time)
    {
        return;
    }

    next_time = TIME_MS + 25;       // Это ограничение вызвано тем, что Mifare Plus требуется не менее 25 мс после сброса энергии (POR) перед следующим REQA

    static TimeMeterMS meter_update;

    static bool prev_detected = false;              // true, если в предыдущем цикле детектирована карта

    bool inserted_before = Card::IsInserted();

    {
        if (!Card::IsInserted() &&                  // Если карта ранее не была вставлена
            !RC663::LPCD::IsCardDetected())         // И не определено наличие в данном цикле
        {
            return;                                 // То ничего делать не будем
        }

        if (!ReaderPowered::IsEnabled())
        {
            return;
        }
    }

    if (meter_update.ElapsedMS() < (prev_detected ? 500U : 50U))
    {
        return;
    }

    meter_update.Reset();

    if (ModeReader::IsNormal())
    {
        static TimeMeterMS meter;

        if (meter.ElapsedMS() >= ProtectionBruteForce::TimeWait())
        {
            meter.Reset();

            prev_detected = RC663::UpdateNormalMode();
        }
    }
    else
    {
        prev_detected = RC663::UpdateExtendedMode(type_auth, new_auth);
    }

    new_auth = false;

    if (!Card::IsInserted())                // Если в конце цикла потеряно соедиение с картой либо не установлено новое
    {
        if (!inserted_before)               // Если карта не была вставлена перед циклом
        {                                   // Значит, было ложное срабатывание
//            RC663::LPCD::Calibrate(); // Нужно перекалибровать
        }

        RC663::LPCD::Calibrate();

        RC663::LPCD::Enter();         // Опять запускаем LPCD
    }
}


void Reader::SetAuth(const TypeAuth &type)
{
    type_auth = type;
    new_auth = true;

    if (type.enabled)
    {
        Card::SetPasswordAuth(Block16(0, type.password));
    }
    else
    {
        Card::SetPasswordAuth(SettingsMaster::PSWD::default_Mifare);
    }
}


TypeAuth Reader::GetAuth()
{
    return type_auth;
}


void ModeReader::Set(E m)
{
    current = m;

    Indicator::Reset();
}


pchar ModeReader::Name(E v)
{
    static const pchar names[Count] =
    {
        "WIEGAND",
        "UART",
        "READ",
        "WRITE"
    };

    return names[v];
}


void ModeOffline::Enable()
{
    current_mode = Enabled;

    pinTXD1.Init();
    pinTXD1.ToLow();

    pinLOCK.Init();         // D0 Замок
    pinLOCK.ToLow();
    
    pinALARM.Init();
    pinALARM.ToLow();

    for (int i = 0; i < 3; i++)
    {
        Indicator::Blink(Color(0x00FF00, 1.0f), Color(0, 0), 500, false);
    }

    Indicator::TurnOn(gset.ColorRed(), TIME_RISE_LEDS, false);
}


void ModeOffline::Update()
{
    if (!IsEnabled())
    {
        return;
    }

    if (time_alarm > 0)
    {
        if (TIME_MS - time_alarm >= gset.TimeAlarm())
        {
            time_alarm = 0;
        }
    }

    if (time_lock > 0)
    {
        if (TIME_MS - time_lock >= gset.TimeLock())
        {
            time_lock = 0;

            pinLOCK.ToLow();
        }
    }
}


bool ModeOffline::IsEnabled()
{
    return (current_mode == Enabled);
}


void ModeOffline::OpenTheLock()
{
    if (!IsEnabled() || gset.TimeLock() == 0)
    {
        return;
    }

    time_lock = TIME_MS;

    pinLOCK.ToHi();
}


void ModeOffline::EnableAlarm()
{
    if (!IsEnabled() || gset.TimeAlarm() == 0)
    {
        return;
    }

    time_alarm = TIME_MS;
}


uint ModeOffline::MaxNumCards()
{
    return 600;
}


bool ModeSignals::IsInternal()
{
    return ModeOffline::IsEnabled();
}


bool ModeSignals::IsExternal()
{
    return !IsInternal();
}
