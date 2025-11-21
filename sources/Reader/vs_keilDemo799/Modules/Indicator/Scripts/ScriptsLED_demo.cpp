// 2025/6/19 08:07:22 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Modules/Indicator/Scripts/ScriptsLED.h"
#include "Hardware/Timer.h"
#include "Modules/Indicator/Scripts/StepScript.h"
#include "Modules/Indicator/ColorLED.h"
#include "Reader/Reader.h"
#include "Modules/Player/Player.h"

#ifdef TYPE_BOARD_771
    #include "Modules/Indicator/LP5012/LP5012.h"
#else
    #include "Modules/Indicator/WS2812B/WS2812B.h"
#endif


namespace ScriptLED
{
    static uint time_last_detected_card = 0;
    static bool prev_detect_card = 0;

    static TimeMeterMS meter_execute;                       // Отсчёт времени выполнения
    static TypeScriptLED::E type = TypeScriptLED::Count;
    static TimeMeterMS meter_enable;                        // Отмеряет заданное время до начала выполнения скрипта

    static const int NUM_LEDS = 4;

    struct LED
    {
        LED(int8 _num_led) :
            num_led(_num_led),
            current_step(-1) { }

        const Step *first_step = nullptr;           // Указатель на первый шаг
        uint        time_start_step = 0;            // В это время началось выполнение текущего шага (отсчёт ведёт meter)
        uint        time_finish_step = 0;           // В это время завершается выполнение текущего шага
        Color       color_begin;                    // Цвет в начале шага
        Color       color_end;                      // Таким цвет должен стать в конце шага
        ColorLED    color_step;                     // Это изменение цвета за 1 мс
        Color       last_color;
        ColorLED    last_colorLED;
        const int8  num_led;                        // Этим светодиодом управляет данная структура
        int8        current_step = -1;              // Текущий выполняемый шаг

        void Init(const Step *_first_step);

        void Update();

    private:

        // Установка текущего шага - он будет обрабатываться
        // time_start_MS - время, в которое по расчёту должен начаться шаг
        void SetStep(int8 num_step, uint time_start_MS);

        // Записать цвет в конечное устройство
        void WriteColor(const ColorLED &);
        void WriteColor(const Color &);

        // true, если шаг завершён
        bool StepIsFinished() const;
    };

    static LED leds[NUM_LEDS] =
    {
        LED(0),
        LED(1),
        LED(2),
        LED(3)
    };

    namespace Rainbow
    {
        static const Step script[] =
        {
            { 0x99FF0000, TypeStep::Color, 5000},     // Красный
            { 0x99FFFF00, TypeStep::Color, 5000},     // Жёлтый
            { 0x9900FF00, TypeStep::Color, 5000},     // Зелёный
            { 0x991CAAD6, TypeStep::Color, 5000},     // Голубой
            { 0x990000FF, TypeStep::Color, 5000},     // Синий
            { 0x99FF00Ff, TypeStep::Color, 5000},     // Пурпурный
            { 0xFFFFFFFF, TypeStep::Repeat }
        };

        static void Init();
    }

    namespace RunningWave
    {
        static const Step script1[] =
        {
            { 0x00FF0000, TypeStep::Color, 0 },     // Минимум - начало     красный
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },   //                      жёлтый
            { 0x6000FF00, TypeStep::Color, 100 },   // Масимум              зелёный
            { 0x6000FFFF, TypeStep::Color, 100 },   //                      голубой
            { 0xFF0000FF, TypeStep::Color, 100 },   // Минимум - конец      синий
            { 0x6000FFFF, TypeStep::Color, 100 },
            { 0x6000FF00, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x00FF0000, TypeStep::Color, 100 },
            { 0x00111111, TypeStep::Nop,   4600  },
            { 0x00111111, TypeStep::Repeat }
        };

        static const Step script2[] =
        {
            { 0x00FF0000, TypeStep::Color, 0 },     // Минимум - начало
            { 0x60111111, TypeStep::Nop, 900 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },   //                      жёлтый
            { 0x6000FF00, TypeStep::Color, 100 },   // Масимум              зелёный
            { 0x6000FFFF, TypeStep::Color, 100 },   //                      голубой
            { 0xFF0000FF, TypeStep::Color, 100 },   // Минимум - конец      синий
            { 0x6000FFFF, TypeStep::Color, 100 },
            { 0x6000FF00, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x00FF0000, TypeStep::Color, 100 },
            { 0x60111111, TypeStep::Nop,  2700 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },   //                      жёлтый
            { 0x6000FF00, TypeStep::Color, 100 },   // Масимум              зелёный
            { 0x6000FFFF, TypeStep::Color, 100 },   //                      голубой
            { 0xFF0000FF, TypeStep::Color, 100 },   // Минимум - конец      синий
            { 0x6000FFFF, TypeStep::Color, 100 },
            { 0x6000FF00, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x00FF0000, TypeStep::Color, 100 },
            { 0x00111111, TypeStep::Nop, 0 },
            { 0x00111111, TypeStep::Repeat }
        };

        static const Step script3[] =
        {
            { 0x00FF0000, TypeStep::Color, 0 },     // Минимум - начало
            { 0x60111111, TypeStep::Nop, 1800 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },   //                      жёлтый
            { 0x6000FF00, TypeStep::Color, 100 },   // Масимум              зелёный
            { 0x6000FFFF, TypeStep::Color, 100 },   //                      голубой
            { 0xFF0000FF, TypeStep::Color, 100 },   // Минимум - конец      синий
            { 0x6000FFFF, TypeStep::Color, 100 },
            { 0x6000FF00, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x00FF0000, TypeStep::Color, 100 },
            { 0x60111111, TypeStep::Nop,   900 },
            { 0x00FF0000, TypeStep::Color, 0 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },   //                      жёлтый
            { 0x6000FF00, TypeStep::Color, 100 },   // Масимум              зелёный
            { 0x6000FFFF, TypeStep::Color, 100 },   //                      голубой
            { 0xFF0000FF, TypeStep::Color, 100 },   // Минимум - конец      синий
            { 0x6000FFFF, TypeStep::Color, 100 },
            { 0x6000FF00, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x00FF0000, TypeStep::Color, 100 },
            { 0x00111111, TypeStep::Nop, 900 },
            { 0x00111111, TypeStep::Repeat }
        };

        static const Step script4[] =
        {
            { 0x00FF0000, TypeStep::Color, 0 },     // Минимум - начало
            { 0x60111111, TypeStep::Nop, 2700 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },   //                      жёлтый
            { 0x6000FF00, TypeStep::Color, 100 },   // Масимум              зелёный
            { 0x6000FFFF, TypeStep::Color, 100 },   //                      голубой
            { 0xFF0000FF, TypeStep::Color, 100 },   // Минимум - конец      синий
            { 0x6000FFFF, TypeStep::Color, 100 },
            { 0x6000FF00, TypeStep::Color, 100 },
            { 0x60FFFF00, TypeStep::Color, 100 },
            { 0x60FF0000, TypeStep::Color, 100 },
            { 0x00FF0000, TypeStep::Color, 100 },
            { 0x00111111, TypeStep::Nop, 1900 },
            { 0x00111111, TypeStep::Repeat }
        };

        static void Init();
    }

    namespace BreathingSystem
    {
        static const Step script[] =
        {
            { 0x19FF4000, TypeStep::Color, 0  },    // 10%
            { 0x4cFF4000, TypeStep::Color, 500, },    // 30%
            { 0x80FF4000, TypeStep::Color, 500, },    // 50%
            { 0xb2FF4000, TypeStep::Color, 500, },    // 70%
            { 0xccFF4000, TypeStep::Color, 500, },    // 80%
            { 0x99FF4000, TypeStep::Color, 500, },    // 60%
            { 0x66FF4000, TypeStep::Color, 500, },    // 40%
            { 0x33FF4000, TypeStep::Color, 500, },    // 20%
            { 0x19FF4000, TypeStep::Color, 500, },    // 10%
            { 0xFFFF4000, TypeStep::Repeat }
        };

        static void Init();
    }

    namespace SleepMode
    {
        static const Step script1[] =
        {
            { 0x33FFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Color, 200  },
            { 0x33FFFFFF, TypeStep::Color, 200  },
            { 0xFFFFFFFF, TypeStep::Nop,   3600 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static const Step script2[] =
        {
            { 0xFFFFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Nop,   400  },
            { 0xFFFFFFFF, TypeStep::Color, 200  },
            { 0x33FFFFFF, TypeStep::Color, 200  },
            { 0xFFFFFFFF, TypeStep::Nop,   3200 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static const Step script3[] =
        {
            { 0xFFFFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Nop,   800  },
            { 0xFFFFFFFF, TypeStep::Color, 200  },
            { 0x33FFFFFF, TypeStep::Color, 200  },
            { 0xFFFFFFFF, TypeStep::Nop,   2800 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static const Step script4[] =
        {
            { 0xFFFFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Nop,   1200 },
            { 0xFFFFFFFF, TypeStep::Color, 200, },
            { 0x33FFFFFF, TypeStep::Color, 200, },
            { 0xFFFFFFFF, TypeStep::Nop,   2400 },
            { 0xFFFFFFFF, TypeStep::Repeat }
        };

        static void Init();
    }

    namespace WaveWithAttenuation
    {
        static const Step script1[] =
        {
            { 0x33FFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Color, 200  },
            { 0x33FFFFFF, TypeStep::Color, 200, },
            { 0xFFFFFFFF, TypeStep::Nop,   3600 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static const Step script2[] =
        {
            { 0xFFFFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Nop,   400  },
            { 0xFFFFFFFF, TypeStep::Color, 200  },
            { 0x33FFFFFF, TypeStep::Color, 200  },
            { 0xFFFFFFFF, TypeStep::Nop,   3200 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static const Step script3[] =
        {
            { 0x33FFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Nop,   800  },
            { 0xFFFFFFFF, TypeStep::Color, 200, },
            { 0x33FFFFFF, TypeStep::Color, 200, },
            { 0xFFFFFFFF, TypeStep::Nop,   2800 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static const Step script4[] =
        {
            { 0xFFFFFFFF, TypeStep::Color, 000  },
            { 0xFFFFFFFF, TypeStep::Nop,   1200 },
            { 0xFFFFFFFF, TypeStep::Color, 200, },
            { 0x33FFFFFF, TypeStep::Color, 200, },
            { 0xFFFFFFFF, TypeStep::Nop,   2400 },
            { 0xFFFFFFFF, TypeStep::Repeat    }
        };

        static void Init();
    }
}


void ScriptLED::Set(TypeScriptLED::E type_script)
{
    type = type_script;

    if (type == TypeScriptLED::RunningWave)
    {
        RunningWave::Init();
    }
    else if (type == TypeScriptLED::BreathingSystem)
    {
        BreathingSystem::Init();
    }
    else if (type == TypeScriptLED::Rainbow)
    {
        Rainbow::Init();
    }
    else if (type == TypeScriptLED::SleepMode)
    {
        SleepMode::Init();
    }
    else if (type == TypeScriptLED::WaveWithAttenuation)
    {
        WaveWithAttenuation::Init();
    }
}


void ScriptLED::Enable()
{
}


void ScriptLED::Disable()
{
    type = TypeScriptLED::Count;
}


void ScriptLED::ResetTimer()
{
    meter_enable.Reset();

    Set(type);
}


void ScriptLED::DetectCard(bool detect)
{
    if (detect)
    {
        if (!prev_detect_card)
        {
            prev_detect_card = true;

            time_last_detected_card = TIME_MS;

            ResetTimer();

            Player::PlayFromMemory(0, 3);
        }
    }
    else
    {
        prev_detect_card = false;
    }
}


void ScriptLED::Update()
{
    if (!ModeReader::IsNormal())
    {
        return;
    }

    if (type == TypeScriptLED::Count)
    {
        Set(TypeScriptLED::BreathingSystem);
    }
    else if (TIME_MS > time_last_detected_card + 60000 &&
        type != TypeScriptLED::BreathingSystem)
    {
        Set(TypeScriptLED::BreathingSystem);
    }
    else if(TIME_MS < time_last_detected_card + 6000 &&
        type == TypeScriptLED::BreathingSystem &&
        time_last_detected_card != 0)
    {
        Set(TypeScriptLED::RunningWave);
    }

    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i].Update();
    }
}


void ScriptLED::RunningWave::Init()
{
    leds[0].Init(script1);
    leds[1].Init(script2);
    leds[2].Init(script3);
    leds[3].Init(script4);
}


void ScriptLED::BreathingSystem::Init()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i].Init(script);
    }
}


void ScriptLED::Rainbow::Init()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i].Init(script);
    }
}


void ScriptLED::SleepMode::Init()
{
    leds[0].Init(script1);
    leds[1].Init(script2);
    leds[2].Init(script3);
    leds[3].Init(script4);
}


void ScriptLED::WaveWithAttenuation::Init()
{
    leds[0].Init(script1);
    leds[1].Init(script2);
    leds[2].Init(script3);
    leds[3].Init(script4);
}


void ScriptLED::LED::Init(const Step *_steps)
{
    first_step = _steps;
    current_step = -1;      // Признак того, что будет начало исполнения
}


void ScriptLED::LED::Update()
{
    if (current_step == -1)
    {
        SetStep(0, meter_execute.ElapsedMS());
    }

    const Step *step = first_step + current_step;

    uint dT = meter_execute.ElapsedMS() - time_start_step;

    switch (step->type)
    {
    case TypeStep::Color:
        if (StepIsFinished())
        {
            WriteColor(color_end);
            SetStep(current_step + 1, time_finish_step);
        }
        else
        {
            ColorLED color = ColorLED::FromColor(color_begin) + color_step * dT;
            WriteColor(color);
        }
        break;

    case TypeStep::Nop:
        if (StepIsFinished())
        {
            SetStep(current_step + 1, time_finish_step);
        }
        break;

    case TypeStep::Repeat:
        SetStep(0, time_finish_step);
        break;
    }
}


bool ScriptLED::LED::StepIsFinished() const
{
    return meter_execute.ElapsedMS() >= time_finish_step;
}


void ScriptLED::LED::WriteColor(const ColorLED &color)
{
    last_colorLED = color;

#ifdef TYPE_BOARD_771

    LP5012::Fire(num_led, color);

#else

    WS2812B::Fire(num_led, color);

#endif
}


void ScriptLED::LED::WriteColor(const Color &color)
{
    last_color = color;

    WriteColor(ColorLED::FromColor(color));
}


void ScriptLED::LED::SetStep(int8 num_step, uint time_start_MS)
{
    current_step = num_step;

    const Step *step = first_step + current_step;

    time_start_step = time_start_MS;
    time_finish_step = time_start_step + step->duration * 2;

    uint dT = time_finish_step - time_start_step;

    if (dT < 1)
    {
        dT = 1;
    }

    if (step->type == TypeStep::Color)
    {
        color_begin = last_color;
        color_end.value = step->value_color;

        color_step = ColorLED::CalculateStep(ColorLED::FromColor(color_begin), ColorLED::FromColor(color_end), dT);
    }
}
