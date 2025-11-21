// 2022/04/27 11:48:13 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Device/Device.h"
#include "Hardware/HAL/HAL.h"
#include "Hardware/Timer.h"
#include "Modules/Player/Player.h"
#include "Modules/Indicator/Indicator.h"
#include "Modules/LIS2DH12/LIS2DH12.h"
#include "Modules/Memory/Memory.h"
#include "Hardware/Power.h"
#include "Communicator/OSDP.h"
#include "Reader/Reader.h"
#include "Modules/Sensor/TTP223.h"
#include "Settings/SET2.h"
#include "Communicator/ROMWriter.h"
#include "Modules/Indicator/Scripts/ScriptsLED.h"
#include "system.h"
#include <cstdlib>
#include <limits.h>


namespace Device
{
    static bool is_initialization_completed = false;

    bool IsInitializationCompleted()
    {
        return is_initialization_completed;
    }
}


void Device::Init()
{
    HAL::Init();

    Timer::Init();

    HAL_ADC::EnablePower();

    SettingsMaster::Load();

    SET2::KeysMFP::Init();

    Memory::Erase::FirmwareIfNeed();

    Player::Init();

    Indicator::Init();

    LIS2DH12::Init();

    Memory::Settings::Init();

#ifdef MCU_GD

    TTP223::Init();

#endif

    for (int i = 0; i < 3; i++)
    {
        Indicator::Blink(Color(0xFFFFFF, 1.0f), Color(0, 0), 500, false);
    }

    while (Device::UpdateTasks())
    {
    }

    Reader::Init();

    is_initialization_completed = true;

#ifdef EMULATE_WG

//    ModeWG::EnableGUID();

    ModeReader::Set(ModeReader::UART);

#endif
}


void Device::Update()
{
    Timer::Delay(1);

    LIS2DH12::Update();         // Акселерометр

    if (ROMWriter::IsCompleted() && !Player::IsPlaying())
    {
        Reader::Update();
    }

    ModeOffline::Update();      // Обработка замка и тревоги

    HAL_ADC::Update();          // Обрабаытваем пониженное напряжение

    UpdateTasks();

    ScriptLED::Update();

#ifndef WIN32
    if (TIME_MS > (((uint)-1)) - (((uint)-1) / 16))
    {
        // Периодически перезагружаем считыватель, чтобы избавиться от возможных глюков, вызыванных
        // переполнением счётчика миллисекунд
        // (он вываливался в режим энергосбережения предположительно по этомй причие)
        NVIC_SystemReset();
    }
#endif
}


bool Device::UpdateTasks()
{
    volatile bool beeper = Player::Update();

    volatile bool indicator = Indicator::Update();

    volatile bool usart = HAL_USART::Update();

    return beeper || indicator || usart;
}
