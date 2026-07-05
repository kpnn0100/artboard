#pragma once
#include "../../scene/Shapes.h"

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
        std::string fontFamily;      // "" = adapter default (generic sans)
        double letterSpacingPx = 0.0;
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

    struct KnobStyle
    {
        BoxStyle dial;
        Color trackColor = Color::rgba(68, 74, 86);
        Color valueColor = Color::rgba(248, 176, 64);
        Color indicatorColor = Color::rgba(248, 176, 64);
        double arcWidth = 3.0;
        TextStyle label;
    };

    struct ToggleStyle
    {
        BoxStyle trackOff;
        BoxStyle trackOn;
        BoxStyle thumb;
    };

    struct ProgressStyle
    {
        BoxStyle track;
        BoxStyle fill;
    };

    struct ComboStyle
    {
        BoxStyle field;
        BoxStyle popup;
        BoxStyle rowSelected;
        TextStyle text;
        Color caretColor = Color::rgba(248, 176, 64);
    };

    struct TabStyle
    {
        BoxStyle tabIdle;
        BoxStyle tabActive;
        TextStyle label;
        TextStyle labelActive = label;  // active tab's title colour (size unused; label's applies)
        // Optional thin bar across the top edge of the active tab only (0 height =
        // none, the default, so existing themes are unaffected until they opt in).
        Color activeIndicatorColor = Color::rgba(0, 0, 0, 0);
        double activeIndicatorHeight = 0.0;
    };

    struct ScrollStyle
    {
        BoxStyle viewport;
        BoxStyle track;
        BoxStyle thumb;
    };

    struct GraphStyle
    {
        BoxStyle background;
        Color gridColor = Color::rgba(68, 74, 86);
        Color lineColor = Color::rgba(248, 176, 64);
        Color fillColor = Color::rgba(248, 176, 64, 40);
        double lineWidth = 2.0;
    };

    struct Theme
    {
        SliderStyle slider;
        ButtonStyle button;
        CheckboxStyle checkbox;
        TextBoxStyle textBox;
        KnobStyle knob;
        ToggleStyle toggle;
        ProgressStyle progress;
        ComboStyle combo;
        TabStyle tab;
        ScrollStyle scroll;
        GraphStyle graph;
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

            theme.knob.dial = {Paint::filledStroked(canvas, outline, 2.0), 999.0};
            theme.knob.trackColor = surface;
            theme.knob.valueColor = accent;
            theme.knob.indicatorColor = accent;
            theme.knob.arcWidth = 3.0;
            theme.knob.label = {muted, 11.0};

            theme.toggle.trackOff = {Paint::filledStroked(surface, outline, 1.0), 999.0};
            theme.toggle.trackOn = {Paint::filledStroked(accent, accent, 1.0), 999.0};
            theme.toggle.thumb = {Paint::filledStroked(ink, outline, 1.0), 999.0};

            theme.progress.track = {Paint::filledStroked(surface, outline, 1.0), 6.0};
            theme.progress.fill = {Paint::filled(accent), 6.0};

            theme.combo.field = {Paint::filledStroked(canvas, outline, 1.0), 8.0};
            theme.combo.popup = {Paint::filledStroked(canvas, accent, 1.0), 8.0};
            theme.combo.rowSelected = {Paint::filled(accentPressed), 0.0};
            theme.combo.text = {ink, 14.0};
            theme.combo.caretColor = accent;

            theme.tab.tabIdle = {Paint::filledStroked(surface, outline, 1.0), 8.0};
            theme.tab.tabActive = {Paint::filledStroked(accent, outline, 1.0), 8.0};
            theme.tab.label = {ink, 13.0};
            theme.tab.labelActive = {ink, 13.0};

            theme.scroll.viewport = {Paint::filledStroked(canvas, outline, 1.0), 8.0};
            theme.scroll.track = {Paint::filled(surface), 4.0};
            theme.scroll.thumb = {Paint::filled(accent), 4.0};

            theme.graph.background = {Paint::filledStroked(canvas, outline, 1.0), 8.0};
            theme.graph.gridColor = surface;
            theme.graph.lineColor = accent;
            theme.graph.fillColor = Color{accent.r, accent.g, accent.b, 0.16};
            theme.graph.lineWidth = 2.0;

            theme.label = {ink, 14.0};
            return theme;
        }
    };
}