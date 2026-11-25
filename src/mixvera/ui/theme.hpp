#pragma once

#include <QString>

class QWidget;

namespace mixvera::ui::theme {

/// Three production-oriented themes. Preview orange and program green stay
/// functional signal colours in every theme; the surfaces and the control
/// accent change.
enum class ThemeId {
    Charcoal, ///< vMix-like charcoal with an orange accent
    Yami,     ///< OBS-like blue-grey with a blue accent
    Daylight, ///< High-contrast light surfaces for bright rooms
};

struct Palette {
    QString window;
    QString surface;
    QString surfaceAlt;
    QString raised;
    QString hover;
    QString field;
    QString fieldDeep;
    QString monitor;
    QString border;
    QString borderStrong;
    QString borderHover;
    QString text;
    QString textStrong;
    QString textSecondary;
    QString textSoft;
    QString textMuted;
    QString textDim;
    QString accent;
    QString accentHover;
    QString accentSoft;
    QString preview;
    QString program;
    QString danger;
    QString dangerDark;
    QString dangerSoft;
    QString info;
    QString infoHover;
    QString infoSoft;
    QString infoDark;
    QString infoTextDim;
    QString selection;
    QString disabled;
    QString black;
    QString success;
    QString warning;
    QString meterLow;
    QString meterMid;
    QString meterHigh;
    QString meterPeak;
    QString onAccent;
    QString onDanger;
    QString onInfo;
    QString onPreview;
    QString onProgram;
    QString checkIcon;
    QString chevronDown;
    QString chevronUp;
};

ThemeId themeFromKey(const QString& key);
QString themeKey(ThemeId id);
QString themeName(ThemeId id);
QString themeDescription(ThemeId id);

const Palette& palette(ThemeId id);
const Palette& palette();

void setTheme(ThemeId id);
ThemeId currentTheme();

QString styleSheet(ThemeId id);
QString styleSheet();

/// Applies the current theme stylesheet to a widget and its descendants.
void apply(QWidget* widget);

} // namespace mixvera::ui::theme
