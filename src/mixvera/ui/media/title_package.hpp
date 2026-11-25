#pragma once

#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QSize>
#include <QString>
#include <QVector>

#include <memory>

namespace mixvera::ui {

/// One editable object inside a vMix / SinSam GT title package.
struct TitleElementInfo {
    QString name;
    QString type;   ///< text, image, colour, ticker, qr
    QString value;  ///< current text, image path, or #RRGGBB
};

/// InputDescriptor parameter keys shared by the package provider, the add
/// input dialog and the title element editor.
namespace title_params {
inline constexpr char kPackage[] = "package";
inline constexpr char kTextPrefix[] = "titleText:";
inline constexpr char kColourPrefix[] = "titleColor:";
inline constexpr char kImagePrefix[] = "titleImage:";
inline constexpr char kTimerMarker[] = "{0:";

inline QString textKey(const QString& element) { return QStringLiteral("titleText:") + element; }
inline QString colourKey(const QString& element)
{
    return QStringLiteral("titleColor:") + element;
}
inline QString imageKey(const QString& element)
{
    return QStringLiteral("titleImage:") + element;
}
} // namespace title_params

/// Read-only model of a .gtzip / .sinsamtitle package, an unzipped package
/// folder, or loose GT composition XML (.xaml). Loading parses the document
/// once; frame() then rasterizes it for any canvas size.
class TitlePackage {
public:
    TitlePackage();
    ~TitlePackage();

    static TitlePackage load(const QString& path, QString* error = nullptr);

    bool isValid() const;
    QString name() const;
    QString sourcePath() const;
    QSize designSize() const;
    QVector<TitleElementInfo> elements() const;

    /// Rasterizes the package on a transparent canvas. Overrides are keyed by
    /// element name; animationSeconds is measured from the entrance start and
    /// tickerSeconds drives ticker elements. A non-negative dataChangeSeconds
    /// plays the package's DataChangeIn storyboard instead of the entrance
    /// storyboard, which is how vMix animates a title preset change.
    QImage render(QSize canvas,
        const QHash<QString, QString>& textOverrides = {},
        const QHash<QString, QColor>& colourOverrides = {},
        const QHash<QString, QString>& imageOverrides = {},
        double animationSeconds = 1e9,
        double tickerSeconds = 0.0,
        const QHash<QString, qint64>& countdownMilliseconds = {},
        double dataChangeSeconds = -1.0) const;

private:
    struct Data;
    std::shared_ptr<const Data> data_;
};

/// Convenience helper for the title editor: parses a package just far enough to
/// list its editable elements. Returns an empty list when the file is invalid.
QVector<TitleElementInfo> titlePackageElements(const QString& path);

/// Evaluates GT {0:...} clock fields in text. A negative countdown value uses
/// the wall clock; a non-negative one is a countdown in milliseconds. The
/// optional now argument makes the result deterministic for tests.
QString titlePackageClockText(const QString& text, qint64 countdownMilliseconds = -1,
    const QDateTime& now = QDateTime());

} // namespace mixvera::ui