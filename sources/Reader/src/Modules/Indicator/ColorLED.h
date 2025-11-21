// 2024/03/27 11:48:07 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


struct ColorLED
{
    explicit ColorLED(uint8 r = 0, uint8 g = 0, uint8 b = 0)
    {
        chan_rel[0] = (float)r / 255.0f;
        chan_rel[1] = (float)g / 255.0f;
        chan_rel[2] = (float)b / 255.0f;
    }

    uint8 Red()   const { return (uint8)(chan_rel[0] * 255.0f); }
    uint8 Green() const { return (uint8)(chan_rel[1] * 255.0f); }
    uint8 Blue()  const { return (uint8)(chan_rel[2] * 255.0f); }

    static ColorLED FromColor(const Color &color)
    {
        float brightness = color.GetBrightnessRelativeF();

        return ColorLED
        (
            (uint8)(color.GetRed() * brightness),
            (uint8)(color.GetGreen() * brightness),
            (uint8)(color.GetBlue() * brightness)
        );
    }

    Color ToColor() const
    {
        return Color((uint8)(chan_rel[0] * 255.0f), (uint8)(chan_rel[1] * 255.0f), (uint8)(chan_rel[2] * 255.0f), 1.0f);
    }

    float chan_rel[3];  // red, green, blue [0...1]

    static ColorLED CalculateStep(const ColorLED &start, const ColorLED &end, uint time);

    char *Log(char buffer[64]) const;

//    bool operator==(const ColorLED &) const;
};


ColorLED operator*(const ColorLED &color, uint mul);
ColorLED operator+(const ColorLED &color1, const ColorLED &color2);


struct ColorF
{
    static ColorF FromColor(const Color &color)
    {
        ColorF result;
        result.char_rel[0] = color.GetRedRelativeF();
        result.char_rel[1] = color.GetGreenRelativeF();
        result.char_rel[2] = color.GetBlueRelativeF();
        result.char_rel[3] = color.GetBrightnessRelativeF();
        return result;
    }

    static ColorF CalculateStep(const ColorF &start, const ColorF &end, uint time);

    ColorLED ToColorLED() const;
    Color ToColor() const;

    float char_rel[4];   // red, green, blue, bright [0...1]
};


ColorF operator*(const ColorF &color, uint mul);
ColorF operator+(const ColorF &color1, const ColorF &color2);
