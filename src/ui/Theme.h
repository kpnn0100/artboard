#pragma once
#include "../scene/Shapes.h"

namespace artboard
{
    struct BoxStyle
    {
        Paint paint;
        double cornerRadius = 0.0;
    };

    struct TextStyle
    {
        Color color = Color::rgba(244, 244, 244);
        double sizePx = 14.0;
    };

    struct SliderStyle
    {
        BoxStyle track;
        BoxStyle rangeFill;
        BoxStyle thumb;
        double thumbRadius = 8.0;
    };

    struct ButtonStyle
    {
        BoxStyle idle;
        BoxStyle pressed;
        TextStyle label;
    };

    struct CheckboxStyle
    {
        BoxStyle box;
        BoxStyle indicator;
        TextStyle label;
    };

    struct TextBoxStyle
    {
        BoxStyle idle;
        BoxStyle focused;
        TextStyle text;
        TextStyle placeholder;
        Color caretColor = Color::rgba(255, 255, 255);
    };

    struct Theme
    {
        SliderStyle slider;
        ButtonStyle button;
        CheckboxStyle checkbox;
        TextBoxStyle textBox;
        TextStyle label;

        static Theme basicTheme()
        {
            Theme theme;
            const Color canvas = Color::rgba(36, 39, 47);
            const Color accent = Color::rgba(248, 176, 64);
            const Color accentPressed = Color::rgba(230, 144, 22);
            const Color surface = Color::rgba(68, 74, 86);
            const Color outline = Color::rgba(196, 200, 208);
            const Color ink = Color::rgba(245, 242, 235);
            const Color muted = Color::rgba(170, 174, 182);

            theme.slider.track = {Paint::filledStroked(surface, outline, 1.0), 8.0};
            theme.slider.rangeFill = {Paint::filled(accent), 8.0};
            theme.slider.thumb = {Paint::filledStroked(canvas, accent, 2.0), 999.0};
            theme.slider.thumbRadius = 9.0;

            theme.button.idle = {Paint::filledStroked(surface, outline, 1.0), 10.0};
            theme.button.pressed = {Paint::filledStroked(accentPressed, outline, 1.0), 10.0};
            theme.button.label = {ink, 14.0};

            theme.checkbox.box = {Paint::filledStroked(surface, outline, 1.0), 6.0};
            theme.checkbox.indicator = {Paint::filled(accent), 4.0};
            theme.checkbox.label = {ink, 14.0};

            theme.textBox.idle = {Paint::filledStroked(canvas, outline, 1.0), 8.0};
            theme.textBox.focused = {Paint::filledStroked(canvas, accent, 2.0), 8.0};
            theme.textBox.text = {ink, 14.0};
            theme.textBox.placeholder = {muted, 14.0};
            theme.textBox.caretColor = accent;

            theme.label = {ink, 14.0};
            return theme;
        }
    };
}