#include "mixvera/ui/theme.hpp"

#include "mixvera/ui/theme_stylesheet.hpp"

#include <QCoreApplication>
#include <QWidget>

namespace mixvera::ui::theme {

namespace {

Palette charcoal()
{
    Palette p;
    p.window = QStringLiteral("#1E2328");
    p.surface = QStringLiteral("#262C33");
    p.surfaceAlt = QStringLiteral("#2C333A");
    p.raised = QStringLiteral("#333B44");
    p.hover = QStringLiteral("#3A434D");
    p.field = QStringLiteral("#1A1F24");
    p.fieldDeep = QStringLiteral("#14181C");
    p.monitor = QStringLiteral("#0B0D12");
    p.border = QStringLiteral("#3E4853");
    p.borderStrong = QStringLiteral("#4A5561");
    p.borderHover = QStringLiteral("#5A6572");
    p.text = QStringLiteral("#E7EAED");
    p.textStrong = QStringLiteral("#FFFFFF");
    p.textSecondary = QStringLiteral("#D7DDE3");
    p.textSoft = QStringLiteral("#AEB7C0");
    p.textMuted = QStringLiteral("#8E98A3");
    p.textDim = QStringLiteral("#6E7781");
    p.accent = QStringLiteral("#FF8C00");
    p.accentHover = QStringLiteral("#FFB45C");
    p.accentSoft = QStringLiteral("#3B2A17");
    p.preview = QStringLiteral("#FF8C00");
    p.program = QStringLiteral("#006400");
    p.danger = QStringLiteral("#C01C37");
    p.dangerDark = QStringLiteral("#A1172E");
    p.dangerSoft = QStringLiteral("#E85E75");
    p.info = QStringLiteral("#2D6CDF");
    p.infoHover = QStringLiteral("#4A85F0");
    p.infoSoft = QStringLiteral("#8FB3FF");
    p.infoDark = QStringLiteral("#244F9E");
    p.infoTextDim = QStringLiteral("#B7C4F0");
    p.selection = QStringLiteral("#343C45");
    p.disabled = QStringLiteral("#2A3037");
    p.black = QStringLiteral("#000000");
    p.success = QStringLiteral("#37D247");
    p.warning = QStringLiteral("#E5AF24");
    p.meterLow = QStringLiteral("#25A231");
    p.meterMid = QStringLiteral("#37D247");
    p.meterHigh = QStringLiteral("#E5AF24");
    p.meterPeak = QStringLiteral("#C01C37");
    p.onAccent = QStringLiteral("#14181C");
    p.onDanger = QStringLiteral("#FFFFFF");
    p.onInfo = QStringLiteral("#FFFFFF");
    p.onPreview = QStringLiteral("#14181C");
    p.onProgram = QStringLiteral("#FFFFFF");
    p.checkIcon = QStringLiteral("url(:/icons/check.svg)");
    p.chevronDown = QStringLiteral("url(:/icons/chevron-down.svg)");
    p.chevronUp = QStringLiteral("url(:/icons/chevron-up.svg)");
    return p;
}

Palette yami()
{
    Palette p;
    p.window = QStringLiteral("#13141A");
    p.surface = QStringLiteral("#1D1F26");
    p.surfaceAlt = QStringLiteral("#22252D");
    p.raised = QStringLiteral("#272A33");
    p.hover = QStringLiteral("#323540");
    p.field = QStringLiteral("#101116");
    p.fieldDeep = QStringLiteral("#0B0C10");
    p.monitor = QStringLiteral("#08090C");
    p.border = QStringLiteral("#323540");
    p.borderStrong = QStringLiteral("#3C404D");
    p.borderHover = QStringLiteral("#4A4E5C");
    p.text = QStringLiteral("#E1E1E1");
    p.textStrong = QStringLiteral("#FFFFFF");
    p.textSecondary = QStringLiteral("#C9CFD8");
    p.textSoft = QStringLiteral("#A8B0BC");
    p.textMuted = QStringLiteral("#969696");
    p.textDim = QStringLiteral("#6E7480");
    p.accent = QStringLiteral("#284CB8");
    p.accentHover = QStringLiteral("#476BD7");
    p.accentSoft = QStringLiteral("#1B2745");
    p.preview = QStringLiteral("#F59E0B");
    p.program = QStringLiteral("#15803D");
    p.danger = QStringLiteral("#C01C37");
    p.dangerDark = QStringLiteral("#A1172E");
    p.dangerSoft = QStringLiteral("#E85E75");
    p.info = QStringLiteral("#476BD7");
    p.infoHover = QStringLiteral("#5B7FE8");
    p.infoSoft = QStringLiteral("#9DB4FF");
    p.infoDark = QStringLiteral("#2A4A9E");
    p.infoTextDim = QStringLiteral("#C3D0FF");
    p.selection = QStringLiteral("#2B303B");
    p.disabled = QStringLiteral("#24272F");
    p.black = QStringLiteral("#000000");
    p.success = QStringLiteral("#37D247");
    p.warning = QStringLiteral("#E5AF24");
    p.meterLow = QStringLiteral("#25A231");
    p.meterMid = QStringLiteral("#37D247");
    p.meterHigh = QStringLiteral("#E5AF24");
    p.meterPeak = QStringLiteral("#C01C37");
    p.onAccent = QStringLiteral("#FFFFFF");
    p.onDanger = QStringLiteral("#FFFFFF");
    p.onInfo = QStringLiteral("#FFFFFF");
    p.onPreview = QStringLiteral("#14181C");
    p.onProgram = QStringLiteral("#FFFFFF");
    p.checkIcon = QStringLiteral("url(:/icons/check-light.svg)");
    p.chevronDown = QStringLiteral("url(:/icons/chevron-down-light.svg)");
    p.chevronUp = QStringLiteral("url(:/icons/chevron-up-light.svg)");
    return p;
}

Palette daylight()
{
    Palette p;
    p.window = QStringLiteral("#EEF1F5");
    p.surface = QStringLiteral("#F7F9FB");
    p.surfaceAlt = QStringLiteral("#E7ECF2");
    p.raised = QStringLiteral("#DCE3EB");
    p.hover = QStringLiteral("#D2DAE3");
    p.field = QStringLiteral("#FFFFFF");
    p.fieldDeep = QStringLiteral("#E3E8EF");
    p.monitor = QStringLiteral("#0B0D12");
    p.border = QStringLiteral("#C7D0DB");
    p.borderStrong = QStringLiteral("#AEB9C6");
    p.borderHover = QStringLiteral("#8E9BAA");
    p.text = QStringLiteral("#1B2430");
    p.textStrong = QStringLiteral("#0B1118");
    p.textSecondary = QStringLiteral("#33404F");
    p.textSoft = QStringLiteral("#5A6878");
    p.textMuted = QStringLiteral("#5F6C7B");
    p.textDim = QStringLiteral("#7C8896");
    p.accent = QStringLiteral("#C2410C");
    p.accentHover = QStringLiteral("#EA580C");
    p.accentSoft = QStringLiteral("#FDE7D8");
    p.preview = QStringLiteral("#C2410C");
    p.program = QStringLiteral("#15803D");
    p.danger = QStringLiteral("#B91C1C");
    p.dangerDark = QStringLiteral("#991B1B");
    p.dangerSoft = QStringLiteral("#DC2626");
    p.info = QStringLiteral("#1D4ED8");
    p.infoHover = QStringLiteral("#2563EB");
    p.infoSoft = QStringLiteral("#60A5FA");
    p.infoDark = QStringLiteral("#1E40AF");
    p.infoTextDim = QStringLiteral("#BFDBFE");
    p.selection = QStringLiteral("#DCE6F2");
    p.disabled = QStringLiteral("#E5E9EF");
    p.black = QStringLiteral("#000000");
    p.success = QStringLiteral("#15803D");
    p.warning = QStringLiteral("#B45309");
    p.meterLow = QStringLiteral("#16A34A");
    p.meterMid = QStringLiteral("#22C55E");
    p.meterHigh = QStringLiteral("#D97706");
    p.meterPeak = QStringLiteral("#DC2626");
    p.onAccent = QStringLiteral("#FFFFFF");
    p.onDanger = QStringLiteral("#FFFFFF");
    p.onInfo = QStringLiteral("#FFFFFF");
    p.onPreview = QStringLiteral("#FFFFFF");
    p.onProgram = QStringLiteral("#FFFFFF");
    p.checkIcon = QStringLiteral("url(:/icons/check-light.svg)");
    p.chevronDown = QStringLiteral("url(:/icons/chevron-down-light.svg)");
    p.chevronUp = QStringLiteral("url(:/icons/chevron-up-light.svg)");
    return p;
}

const Palette kCharcoal = charcoal();
const Palette kYami = yami();
const Palette kDaylight = daylight();

ThemeId g_current = ThemeId::Charcoal;

} // namespace

ThemeId themeFromKey(const QString& key)
{
    const QString value = key.trimmed().toLower();
    if (value == "yami" || value == "obs") {
        return ThemeId::Yami;
    }
    if (value == "daylight" || value == "light") {
        return ThemeId::Daylight;
    }
    return ThemeId::Charcoal;
}

QString themeKey(ThemeId id)
{
    switch (id) {
    case ThemeId::Yami:
        return QStringLiteral("yami");
    case ThemeId::Daylight:
        return QStringLiteral("daylight");
    case ThemeId::Charcoal:
    default:
        return QStringLiteral("charcoal");
    }
}

QString themeName(ThemeId id)
{
    switch (id) {
    case ThemeId::Yami:
        return QCoreApplication::translate("Theme", "Yami");
    case ThemeId::Daylight:
        return QCoreApplication::translate("Theme", "Daylight");
    case ThemeId::Charcoal:
    default:
        return QCoreApplication::translate("Theme", "Charcoal");
    }
}

QString themeDescription(ThemeId id)
{
    switch (id) {
    case ThemeId::Yami:
        return QCoreApplication::translate("Theme",
            "OBS-style blue-grey surfaces and a blue control accent.");
    case ThemeId::Daylight:
        return QCoreApplication::translate("Theme",
            "Light surfaces and dark text for bright control rooms.");
    case ThemeId::Charcoal:
    default:
        return QCoreApplication::translate("Theme",
            "vMix-style charcoal surfaces and an orange control accent.");
    }
}

const Palette& palette(ThemeId id)
{
    switch (id) {
    case ThemeId::Yami:
        return kYami;
    case ThemeId::Daylight:
        return kDaylight;
    case ThemeId::Charcoal:
    default:
        return kCharcoal;
    }
}

const Palette& palette()
{
    return palette(g_current);
}

void setTheme(ThemeId id)
{
    g_current = id;
}

ThemeId currentTheme()
{
    return g_current;
}

QString styleSheet(ThemeId id)
{
    const Palette& p = palette(id);
    QString qss = QString::fromUtf8(kStyleSheetTemplate);
    const std::pair<const char*, QString> tokens[] = {
        {"{{window}}", p.window},
        {"{{surface}}", p.surface},
        {"{{surfaceAlt}}", p.surfaceAlt},
        {"{{raised}}", p.raised},
        {"{{hover}}", p.hover},
        {"{{field}}", p.field},
        {"{{fieldDeep}}", p.fieldDeep},
        {"{{monitor}}", p.monitor},
        {"{{border}}", p.border},
        {"{{borderStrong}}", p.borderStrong},
        {"{{borderHover}}", p.borderHover},
        {"{{text}}", p.text},
        {"{{textStrong}}", p.textStrong},
        {"{{textSecondary}}", p.textSecondary},
        {"{{textSoft}}", p.textSoft},
        {"{{textMuted}}", p.textMuted},
        {"{{textDim}}", p.textDim},
        {"{{accent}}", p.accent},
        {"{{accentHover}}", p.accentHover},
        {"{{accentSoft}}", p.accentSoft},
        {"{{preview}}", p.preview},
        {"{{program}}", p.program},
        {"{{danger}}", p.danger},
        {"{{dangerDark}}", p.dangerDark},
        {"{{dangerSoft}}", p.dangerSoft},
        {"{{info}}", p.info},
        {"{{infoHover}}", p.infoHover},
        {"{{infoSoft}}", p.infoSoft},
        {"{{infoDark}}", p.infoDark},
        {"{{infoTextDim}}", p.infoTextDim},
        {"{{selection}}", p.selection},
        {"{{disabled}}", p.disabled},
        {"{{black}}", p.black},
        {"{{success}}", p.success},
        {"{{warning}}", p.warning},
        {"{{onAccent}}", p.onAccent},
        {"{{onDanger}}", p.onDanger},
        {"{{onInfo}}", p.onInfo},
        {"{{onPreview}}", p.onPreview},
        {"{{onProgram}}", p.onProgram},
        {"{{checkIcon}}", p.checkIcon},
        {"{{chevronDown}}", p.chevronDown},
        {"{{chevronUp}}", p.chevronUp},
    };
    for (const auto& [token, value] : tokens) {
        qss.replace(QLatin1String(token), value);
    }
    return qss;
}

QString styleSheet()
{
    return styleSheet(g_current);
}

void apply(QWidget* widget)
{
    if (widget) {
        widget->setStyleSheet(styleSheet());
        widget->update();
    }
}

} // namespace mixvera::ui::theme
