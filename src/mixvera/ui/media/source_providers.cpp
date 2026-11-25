#include "mixvera/ui/media/source_provider.hpp"

#include "mixvera/ui/media/title_package.hpp"
#include "mixvera/ui/platform/win_capture.hpp"

#include <QAudioOutput>
#include <QCamera>
#include <QCameraDevice>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHash>
#include <QImage>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QTime>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>

#include <algorithm>
#include <cmath>
#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace mixvera::ui {

namespace {

constexpr int kNativeWidth = 1280;
constexpr int kNativeHeight = 720;

QString param(const core::InputDescriptor& input, const char* key,
    const QString& fallback = QString())
{
    const auto it = input.params.find(key);
    if (it == input.params.end() || it->second.empty()) {
        return fallback;
    }
    return QString::fromStdString(it->second);
}

class TestPatternProvider final : public SourceProvider {
public:
    explicit TestPatternProvider(const core::InputDescriptor& input)
        : name_(QString::fromStdString(input.name))
    {
        clock_.start();
    }

    QImage frame() override
    {
        QImage image(kNativeWidth, kNativeHeight, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);

        static const QColor bars[] = {
            QColor("#c0c0c0"), QColor("#c0c000"), QColor("#00c0c0"), QColor("#00c000"),
            QColor("#c000c0"), QColor("#c00000"), QColor("#0000c0"), QColor("#101010")};
        const int barWidth = kNativeWidth / 8;
        for (int index = 0; index < 8; ++index) {
            painter.fillRect(QRect(index * barWidth, 0, barWidth + 1, kNativeHeight * 3 / 4),
                bars[index]);
        }

        const int lowerY = kNativeHeight * 3 / 4;
        painter.fillRect(QRect(0, lowerY, kNativeWidth, kNativeHeight - lowerY),
            QColor("#0b0d12"));

        // Grey ramp strip, like a real bars generator.
        const int steps = 16;
        for (int index = 0; index < steps; ++index) {
            const int value = index * 255 / (steps - 1);
            painter.fillRect(QRect(index * kNativeWidth / steps, lowerY + 24,
                                kNativeWidth / steps + 1, 46),
                QColor(value, value, value));
        }

        const qreal phase = std::fmod(clock_.elapsed() / 1000.0, 4.0) / 4.0;
        const qreal sweep = phase * kNativeWidth;
        QLinearGradient glow(sweep - 120, 0, sweep + 120, 0);
        glow.setColorAt(0.0, QColor(255, 255, 255, 0));
        glow.setColorAt(0.5, QColor(255, 255, 255, 46));
        glow.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.fillRect(QRectF(sweep - 120, 0, 240, kNativeHeight), glow);

        painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
        painter.drawLine(kNativeWidth / 2, 0, kNativeWidth / 2, kNativeHeight);
        painter.drawLine(0, kNativeHeight / 2, kNativeWidth, kNativeHeight / 2);

        QFont titleFont(QStringLiteral("Bahnschrift"), 34, QFont::DemiBold);
        titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
        painter.setFont(titleFont);
        painter.setPen(QColor(255, 255, 255, 232));
        painter.drawText(QRect(36, kNativeHeight - 150, kNativeWidth - 72, 50),
            Qt::AlignLeft | Qt::AlignVCenter, name_.toUpper());

        QFont monoFont(QStringLiteral("Consolas"), 15);
        painter.setFont(monoFont);
        painter.setPen(QColor(220, 230, 245, 190));
        painter.drawText(QRect(36, kNativeHeight - 96, kNativeWidth - 72, 30),
            Qt::AlignLeft | Qt::AlignVCenter,
            QString("1280x720  ·  30p  ·  %1").arg(QTime::currentTime().toString("HH:mm:ss.zzz")));
        return image;
    }

    QString detail() const override { return QStringLiteral("Generated test pattern"); }

private:
    QString name_;
    QElapsedTimer clock_;
};

class ColorProvider final : public SourceProvider {
public:
    explicit ColorProvider(const core::InputDescriptor& input)
        : color_(param(input, "color", "#1e3a8a"))
    {
        if (!color_.isValid()) {
            color_ = QColor("#1e3a8a");
        }
    }

    QImage frame() override
    {
        QImage image(kNativeWidth, kNativeHeight, QImage::Format_ARGB32_Premultiplied);
        image.fill(color_);
        return image;
    }

    QString detail() const override { return color_.name().toUpper(); }

private:
    QColor color_;
};

class TitleProvider final : public SourceProvider {
public:
    explicit TitleProvider(const core::InputDescriptor& input)
    {
        applyParams(input);
        tickerClock_.start();
        packageClock_.start();
    }

    /// Title edits (text, colours, images, score, countdown) apply in place so
    /// the entrance animation and ticker do not restart on every keystroke or
    /// score click. A different style or package still needs a fresh provider.
    bool update(const core::InputDescriptor& input) override
    {
        const QString style = param(input, "style", "full");
        const QString package = param(input, "package");
        if (style != style_ || package != packagePath_) {
            return false;
        }
        applyParams(input);
        return style_ != QLatin1String("package") || (package_ && package_->isValid());
    }

    void setCanvasSize(QSize size) override
    {
        if (size.isValid() && !size.isEmpty()) {
            canvas_ = size;
        }
    }

    bool ready() const override
    {
        return style_ != QLatin1String("package") || (package_ && package_->isValid());
    }

    QImage frame() override
    {
        if (style_ == QLatin1String("package")) {
            return packageFrame();
        }
        QImage image(canvas_, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        if (style_ == "lower") {
            drawLowerThird(painter);
        } else if (style_ == "countdown") {
            drawCountdown(painter);
        } else if (style_ == "scoreboard") {
            drawScoreboard(painter);
        } else if (style_ == "caption") {
            drawCaption(painter);
        } else if (style_ == "ticker") {
            drawTicker(painter);
        } else {
            drawFullScreen(painter);
        }
        return image;
    }

    QString detail() const override
    {
        if (style_ == "package") {
            if (package_ && package_->isValid()) {
                return QCoreApplication::translate("Title", "Title package: %1")
                    .arg(package_->name());
            }
            return QCoreApplication::translate("Title", "Missing title package: %1")
                .arg(QFileInfo(packagePath_).fileName());
        }
        if (style_ == "countdown") {
            const qint64 seconds = (std::max<qint64>(remainingMs(), 0) + 999) / 1000;
            return QCoreApplication::translate("Title", "Countdown: %1:%2")
                .arg(seconds / 60, 2, 10, QLatin1Char('0'))
                .arg(seconds % 60, 2, 10, QLatin1Char('0'));
        }
        if (style_ == "scoreboard") {
            return QCoreApplication::translate("Title", "Score: %1 %2 : %3 %4")
                .arg(homeName_)
                .arg(homeScore_)
                .arg(awayScore_)
                .arg(awayName_);
        }
        if (style_ == "lower") {
            return QCoreApplication::translate("Title", "Lower third: %1").arg(text_.left(32));
        }
        if (style_ == "caption") {
            return QCoreApplication::translate("Title", "Subtitle: %1")
            .arg(captionText().left(32));
        }
        if (style_ == "ticker") {
            return QCoreApplication::translate("Title", "Ticker: %1").arg(text_.left(32));
        }
        return QCoreApplication::translate("Title", "Title: %1").arg(text_.left(32));
    }

private:
    Qt::Alignment alignment() const
    {
        if (align_ == "center") {
            return Qt::AlignHCenter;
        }
        if (align_ == "right") {
            return Qt::AlignRight;
        }
        return Qt::AlignLeft;
    }

    /// Layout unit: 1.0 at the 1280x720 native title canvas. Portrait shows
    /// scale their text and panels by width while anchoring cards to the
    /// bottom, so a 1080x1920 canvas still reads like a caption, not a crop.
    qreal unit() const { return canvas_.width() / 1280.0; }

    QImage packageFrame()
    {
        if (!package_ || !package_->isValid()) {
            QImage image(canvas_, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            return image;
        }
        QHash<QString, QString> texts = elementTexts_;
        const auto elements = package_->elements();
        QStringList textNames;
        QStringList tickerNames;
        for (const auto& element : elements) {
            if (element.type == QLatin1String("text")) {
                textNames.append(element.name);
                const QString lower = element.name.toLower();
                if (!texts.contains(element.name)) {
                    if (hasScore_ && lower.contains(QLatin1String("home"))
                        && lower.contains(QLatin1String("score"))) {
                        texts.insert(element.name, QString::number(homeScore_));
                    } else if (hasScore_ && lower.contains(QLatin1String("away"))
                        && lower.contains(QLatin1String("score"))) {
                        texts.insert(element.name, QString::number(awayScore_));
                    } else if (hasHomeName_ && lower.contains(QLatin1String("home"))
                        && (lower.contains(QLatin1String("name"))
                            || lower.contains(QLatin1String("team")))) {
                        texts.insert(element.name, homeName_);
                    } else if (hasAwayName_ && lower.contains(QLatin1String("away"))
                        && (lower.contains(QLatin1String("name"))
                            || lower.contains(QLatin1String("team")))) {
                        texts.insert(element.name, awayName_);
                    }
                }
            } else if (element.type == QLatin1String("ticker")) {
                tickerNames.append(element.name);
            }
        }
        bool textPlaced = false;
        int nextText = 0;
        if (hasTextParam_ && !text_.isEmpty() && text_ != QLatin1String("TITLE")) {
            while (nextText < textNames.size() && texts.contains(textNames[nextText])) {
                ++nextText;
            }
            if (nextText < textNames.size()) {
                texts.insert(textNames[nextText], text_);
                textPlaced = true;
                ++nextText;
            }
        }
        if (!subtitle_.isEmpty()) {
            QString target;
            for (const QString& preferred : { QStringLiteral("Description"),
                     QStringLiteral("Bottom Text"), QStringLiteral("Subtitle") }) {
                for (const QString& name : textNames) {
                    if (name.compare(preferred, Qt::CaseInsensitive) == 0
                        && !texts.contains(name)) {
                        target = name;
                        break;
                    }
                }
                if (!target.isEmpty()) {
                    break;
                }
            }
            if (target.isEmpty()) {
                for (const QString& name : textNames) {
                    if (!texts.contains(name)) {
                        target = name;
                        break;
                    }
                }
            }
            if (target.isEmpty() && textNames.size() > 1) {
                target = textNames.at(1);
            }
            if (!target.isEmpty()) {
                texts.insert(target, subtitle_);
            }
        }
        if (hasTextParam_ && !textPlaced && !text_.isEmpty()
            && text_ != QLatin1String("TITLE")) {
            for (const QString& name : tickerNames) {
                if (!texts.contains(name)) {
                    texts.insert(name, text_);
                    break;
                }
            }
        }
        QHash<QString, qint64> countdowns;
        if (endMs_ > 0 || remainingMs_ > 0 || paused_) {
            const qint64 remaining = std::max<qint64>(remainingMs(), 0);
            for (const auto& element : elements) {
                const QString text = texts.value(element.name, element.value);
                if (text.contains(QLatin1String(title_params::kTimerMarker))) {
                    countdowns.insert(element.name, remaining);
                }
            }
        }
        const double dataChangeSeconds = presetChangedMs_ > 0
            ? std::max(0.0, static_cast<double>(QDateTime::currentMSecsSinceEpoch()
                                                    - presetChangedMs_)
                    / 1000.0)
            : -1.0;
        return package_->render(canvas_, texts, elementColours_, elementImages_,
            packageClock_.elapsed() / 1000.0, tickerClock_.elapsed() / 1000.0,
            countdowns, dataChangeSeconds);
    }

    QString captionText() const
    {
        if (!text_.trimmed().isEmpty()) {
            return text_;
        }
        const QStringList lines = captions_.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        if (lines.isEmpty()) {
            return QString();
        }
        const int index = std::clamp(captionIndex_, 0, static_cast<int>(lines.size()) - 1);
        return lines[index].trimmed();
    }

    qint64 remainingMs() const
    {
        if (endMs_ > 0) {
            return endMs_ - QDateTime::currentMSecsSinceEpoch();
        }
        return remainingMs_;
    }

    void drawFullScreen(QPainter& painter)
    {
        const qreal scale = unit();
        QFont font(QStringLiteral("Bahnschrift"),
            std::max(8, qRound(size_ * scale)), QFont::Bold);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 1.5 * scale);
        painter.setFont(font);

        const int margin = qRound(60 * scale);
        const QRect box(margin, margin, canvas_.width() - margin * 2,
            canvas_.height() - margin * 2);
        painter.setPen(QColor(0, 0, 0, 170));
        painter.drawText(box.translated(qRound(3 * scale), qRound(3 * scale)),
            alignment() | Qt::AlignVCenter | Qt::TextWordWrap, text_);
        painter.setPen(color_);
        painter.drawText(box, alignment() | Qt::AlignVCenter | Qt::TextWordWrap, text_);
    }

    void drawLowerThird(QPainter& painter)
    {
        const qreal scale = unit();
        const int margin = qRound(64 * scale);
        const int panelHeight = qRound(150 * scale);
        const int textWidth = qRound(760 * scale);

        QFont mainFont(QStringLiteral("Bahnschrift"),
            std::clamp(qRound(size_ * scale), 16, 200), QFont::Bold);
        QFont subFont(QStringLiteral("Bahnschrift"), std::max(12, qRound(24 * scale)),
            QFont::Normal);
        QFontMetrics mainMetrics(mainFont);
        QFontMetrics subMetrics(subFont);
        const QString mainText = mainMetrics.elidedText(text_, Qt::ElideRight, textWidth);
        const QString subText = subMetrics.elidedText(subtitle_, Qt::ElideRight, textWidth);

        int panelWidth = std::max(qRound(440 * scale),
            std::max(mainMetrics.horizontalAdvance(mainText),
                subMetrics.horizontalAdvance(subText))
                + qRound(64 * scale));
        panelWidth = std::min(panelWidth, canvas_.width() - margin * 2);
        const QRect panel(margin, canvas_.height() - qRound(80 * scale) - panelHeight,
            panelWidth, panelHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 218));
        painter.drawRoundedRect(panel, qRound(4 * scale), qRound(4 * scale));
        painter.fillRect(QRect(panel.left(), panel.top(), qRound(10 * scale), panel.height()),
            color_);
        painter.setPen(QPen(QColor(255, 255, 255, 28), 1));
        painter.drawLine(panel.left(), panel.top(), panel.right(), panel.top());

        painter.setFont(mainFont);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(QRect(panel.left() + qRound(30 * scale),
                             panel.top() + qRound(22 * scale), panel.width() - qRound(48 * scale),
                             qRound(58 * scale)),
            Qt::AlignLeft | Qt::AlignVCenter, mainText);
        if (!subText.isEmpty()) {
            painter.setFont(subFont);
            painter.setPen(QColor("#c9cfd8"));
            painter.drawText(QRect(panel.left() + qRound(30 * scale),
                                 panel.top() + qRound(84 * scale),
                                 panel.width() - qRound(48 * scale), qRound(36 * scale)),
                Qt::AlignLeft | Qt::AlignVCenter, subText);
        }
    }

    void drawCountdown(QPainter& painter)
    {
        const qreal scale = unit();
        const qint64 total = std::max<qint64>(durationMs_, 1);
        const qint64 remaining = std::clamp<qint64>(remainingMs(), 0, total);
        const int cardWidth = qRound(560 * scale);
        const int cardHeight = qRound(260 * scale);
        const QRect card((canvas_.width() - cardWidth) / 2,
            (canvas_.height() - cardHeight) / 2, cardWidth, cardHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 220));
        painter.drawRoundedRect(card, qRound(18 * scale), qRound(18 * scale));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(color_, 2));
        painter.drawRoundedRect(card.adjusted(1, 1, -1, -1), qRound(18 * scale),
            qRound(18 * scale));

        QFont labelFont(QStringLiteral("Bahnschrift"), std::max(12, qRound(24 * scale)),
            QFont::DemiBold);
        labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0 * scale);
        painter.setFont(labelFont);
        painter.setPen(QColor("#aeb7c0"));
        const QString label = paused_
            ? QCoreApplication::translate("Title", "PAUSED")
            : (text_.isEmpty() ? QCoreApplication::translate("Title", "COUNTDOWN")
                               : text_.toUpper());
        painter.drawText(QRect(card.left(), card.top() + qRound(26 * scale), card.width(),
                             qRound(34 * scale)),
            Qt::AlignHCenter, label);

        const qint64 seconds = (remaining + 999) / 1000;
        QString timer = QStringLiteral("%1:%2")
                            .arg(seconds / 60, 2, 10, QLatin1Char('0'))
                            .arg(seconds % 60, 2, 10, QLatin1Char('0'));
        if (seconds >= 3600) {
            timer = QStringLiteral("%1:%2:%3")
                        .arg(seconds / 3600)
                        .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0'))
                        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
        }
        QFont timerFont(QStringLiteral("Consolas"), std::max(20, qRound(92 * scale)),
            QFont::Bold);
        painter.setFont(timerFont);
        painter.setPen(remaining <= 0 ? QColor("#e85e75") : QColor("#ffffff"));
        painter.drawText(QRect(card.left(), card.top() + qRound(68 * scale), card.width(),
                             qRound(124 * scale)),
            Qt::AlignCenter, timer);

        const QRect track(card.left() + qRound(40 * scale),
            card.bottom() - qRound(48 * scale), card.width() - qRound(80 * scale),
            std::max(4, qRound(8 * scale)));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 28));
        painter.drawRoundedRect(track, track.height() / 2, track.height() / 2);
        QRect fill = track;
        fill.setWidth(static_cast<int>(track.width()
            * (static_cast<double>(remaining) / static_cast<double>(total))));
        painter.setBrush(remaining <= 0 ? QColor("#e85e75") : color_);
        painter.drawRoundedRect(fill, track.height() / 2, track.height() / 2);
    }

    void drawScoreboard(QPainter& painter)
    {
        const qreal scale = unit();
        const int cardWidth = std::min(canvas_.width() - qRound(64 * scale),
            qRound(820 * scale));
        const int cardHeight = qRound(158 * scale);
        const QRect card((canvas_.width() - cardWidth) / 2, qRound(42 * scale),
            cardWidth, cardHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 220));
        painter.drawRoundedRect(card, qRound(12 * scale), qRound(12 * scale));
        painter.fillRect(QRect(card.left(), card.top(), card.width(), qRound(6 * scale)),
            color_);

        QFont nameFont(QStringLiteral("Bahnschrift"), std::max(12, qRound(28 * scale)),
            QFont::DemiBold);
        QFont scoreFont(QStringLiteral("Consolas"), std::max(20, qRound(60 * scale)),
            QFont::Bold);
        QFont titleFont(QStringLiteral("Bahnschrift"), std::max(10, qRound(20 * scale)),
            QFont::Normal);
        QFontMetrics nameMetrics(nameFont);
        const int nameWidth = qRound(210 * scale);

        if (!competition_.isEmpty()) {
            painter.setFont(titleFont);
            painter.setPen(QColor("#aeb7c0"));
            painter.drawText(QRect(card.left(), card.top() + qRound(14 * scale),
                                 card.width(), qRound(26 * scale)),
                Qt::AlignHCenter, competition_.toUpper());
        }

        painter.setFont(nameFont);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(QRect(card.left() + qRound(28 * scale),
                             card.top() + qRound(84 * scale), nameWidth,
                             qRound(44 * scale)),
            Qt::AlignLeft | Qt::AlignVCenter,
            nameMetrics.elidedText(homeName_, Qt::ElideRight, nameWidth));
        painter.drawText(QRect(card.left() + qRound(582 * scale),
                             card.top() + qRound(84 * scale), nameWidth,
                             qRound(44 * scale)),
            Qt::AlignRight | Qt::AlignVCenter,
            nameMetrics.elidedText(awayName_, Qt::ElideRight, nameWidth));

        painter.setFont(scoreFont);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(QRect(card.left() + qRound(238 * scale),
                             card.top() + qRound(46 * scale), qRound(120 * scale),
                             qRound(80 * scale)),
            Qt::AlignRight | Qt::AlignVCenter, QString::number(homeScore_));
        painter.drawText(QRect(card.left() + qRound(462 * scale),
                             card.top() + qRound(46 * scale), qRound(120 * scale),
                             qRound(80 * scale)),
            Qt::AlignLeft | Qt::AlignVCenter, QString::number(awayScore_));

        painter.setFont(nameFont);
        painter.setPen(QColor("#8e98a3"));
        painter.drawText(QRect(card.left() + qRound(398 * scale),
                             card.top() + qRound(46 * scale), qRound(24 * scale),
                             qRound(80 * scale)),
            Qt::AlignCenter, QStringLiteral(":"));
    }

    /// Broadcast-style subtitle: a full-width translucent plate near the
    /// bottom safe area, centred white text and a thin accent underline.
    void drawCaption(QPainter& painter)
    {
        const QString caption = captionText();
        if (caption.trimmed().isEmpty()) {
            return;
        }
        const qreal scale = unit();
        const int margin = qRound(80 * scale);
        const int maxWidth = canvas_.width() - margin * 2;
        QFont font(QStringLiteral("Bahnschrift"),
            std::clamp(qRound(size_ * scale), 18, 140), QFont::DemiBold);
        QFontMetrics metrics(font);
        const QRect measured = metrics.boundingRect(QRect(0, 0, maxWidth, 10000),
            Qt::AlignHCenter | Qt::TextWordWrap, caption);
        const int padX = qRound(36 * scale);
        const int padY = qRound(22 * scale);
        const int panelHeight = std::max(qRound(96 * scale), measured.height() + padY * 2);
        const int bottomMargin = std::max(qRound(36 * scale),
            qRound(canvas_.height() * 0.045));
        const QRect panel(margin, canvas_.height() - bottomMargin - panelHeight,
            maxWidth, panelHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 205));
        painter.drawRoundedRect(panel, qRound(6 * scale), qRound(6 * scale));
        painter.fillRect(QRect(panel.left(), panel.bottom() - qRound(6 * scale),
                             panel.width(), qRound(6 * scale)),
            color_);

        painter.setFont(font);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(panel.adjusted(padX, padY, -padX, -padY),
            Qt::AlignCenter | Qt::TextWordWrap, caption);
    }

    /// Rolling ticker in the bottom safe area. Text enters from the right,
    /// crosses the canvas and loops seamlessly.
    void drawTicker(QPainter& painter)
    {
        QString label = text_.trimmed();
        if (label.isEmpty()) {
            label = captionText();
        }
        if (label.isEmpty()) {
            return;
        }
        label.replace(QLatin1Char('\n'), QStringLiteral("   ·   "));
        label.replace(QLatin1Char('\r'), QString());

        const qreal scale = unit();
        const int barHeight = std::max(qRound(84 * scale), qRound(24 * scale));
        const int bottomMargin = qRound(36 * scale);
        const QRect bar(0, canvas_.height() - bottomMargin - barHeight, canvas_.width(),
            barHeight);

        painter.save();
        painter.setClipRect(bar);
        painter.fillRect(bar, QColor(8, 10, 14, 220));
        painter.fillRect(QRect(bar.left(), bar.top(), bar.width(), qRound(4 * scale)), color_);

        QFont font(QStringLiteral("Bahnschrift"), std::max(12, qRound(34 * scale)),
            QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(QColor("#ffffff"));
        QFontMetrics metrics(font);
        const int textWidth = metrics.horizontalAdvance(label);
        const int gap = qRound(120 * scale);
        const int travel = textWidth + gap + canvas_.width();
        const double seconds = tickerClock_.isValid()
            ? static_cast<double>(tickerClock_.elapsed()) / 1000.0
            : 0.0;
        const int offset = static_cast<int>(std::fmod(seconds * tickerSpeed_, travel));
        const int x = canvas_.width() - offset;
        painter.drawText(QRect(x, bar.top(), textWidth + gap, bar.height()),
            Qt::AlignLeft | Qt::AlignVCenter, label);
        if (x + textWidth < canvas_.width()) {
            painter.drawText(QRect(x + travel, bar.top(), textWidth + gap, bar.height()),
                Qt::AlignLeft | Qt::AlignVCenter, label);
        }
        painter.restore();
    }

    void applyParams(const core::InputDescriptor& input)
    {
        text_ = param(input, "text", "TITLE");
        subtitle_ = param(input, "subtitle");
        style_ = param(input, "style", "full");
        color_ = QColor(param(input, "color", "#ffffff"));
        size_ = param(input, "size", "64").toInt();
        align_ = param(input, "align", "center");
        durationMs_ = param(input, "countdownSeconds", "300").toLongLong() * 1000;
        endMs_ = param(input, "countdownEnd", "0").toLongLong();
        remainingMs_ = param(input, "countdownRemaining", "").toLongLong();
        paused_ = param(input, "countdownPaused", "0") == "1";
        competition_ = param(input, "competition");
        homeName_ = param(input, "homeName", "HOME");
        awayName_ = param(input, "awayName", "AWAY");
        homeScore_ = param(input, "scoreHome", "0").toInt();
        awayScore_ = param(input, "scoreAway", "0").toInt();
        captions_ = param(input, "captions");
        captionIndex_ = param(input, "captionIndex", "0").toInt();
        tickerSpeed_ = std::clamp(param(input, "tickerSpeed", "120").toInt(), 20, 800);
        packagePath_ = param(input, "package");
        presetChangedMs_ = param(input, "titlePresetChanged", "0").toLongLong();

        elementTexts_.clear();
        elementColours_.clear();
        elementImages_.clear();
        hasTextParam_ = false;
        hasHomeName_ = false;
        hasAwayName_ = false;
        hasScore_ = false;
        const auto textParam = input.params.find("text");
        hasTextParam_ = textParam != input.params.end() && !textParam->second.empty();
        for (const auto& entry : input.params) {
            const QString key = QString::fromStdString(entry.first);
            const QString value = QString::fromStdString(entry.second);
            if (key.startsWith(QStringLiteral("titleText:"))) {
                elementTexts_.insert(key.mid(10), value);
            } else if (key.startsWith(QStringLiteral("titleColor:"))) {
                const QColor colour(value);
                if (colour.isValid()) {
                    elementColours_.insert(key.mid(11), colour);
                }
            } else if (key.startsWith(QStringLiteral("titleImage:"))) {
                elementImages_.insert(key.mid(11), value);
            }
            if (key == QLatin1String("homeName")) {
                hasHomeName_ = true;
            } else if (key == QLatin1String("awayName")) {
                hasAwayName_ = true;
            } else if (key == QLatin1String("scoreHome")
                || key == QLatin1String("scoreAway")) {
                hasScore_ = true;
            }
        }
        if (style_ == QLatin1String("package") && !packagePath_.isEmpty()
            && (!package_ || packagePath_ != package_->sourcePath())) {
            auto package = std::make_shared<TitlePackage>(
                TitlePackage::load(packagePath_));
            if (package->isValid()) {
                package_ = package;
            }
        }
        if (text_.isEmpty()) {
            text_ = "TITLE";
        }
        if (!color_.isValid()) {
            color_ = QColor("#ffffff");
        }
        if (durationMs_ <= 0) {
            durationMs_ = 300000;
        }
        if (style_.isEmpty()) {
            style_ = "full";
        }
    }

    QSize canvas_{1280, 720};
    QString text_;
    QString subtitle_;
    QString style_;
    QColor color_;
    int size_ = 64;
    QString align_;

    qint64 durationMs_ = 300000;
    qint64 endMs_ = 0;
    qint64 remainingMs_ = 0;
    bool paused_ = false;

    QString competition_;
    QString homeName_;
    QString awayName_;
    int homeScore_ = 0;
    int awayScore_ = 0;

    QString captions_;
    int captionIndex_ = 0;
    int tickerSpeed_ = 120;
    QElapsedTimer tickerClock_;

    QString packagePath_;
    qint64 presetChangedMs_ = 0;
    std::shared_ptr<TitlePackage> package_;
    QHash<QString, QString> elementTexts_;
    QHash<QString, QColor> elementColours_;
    QHash<QString, QString> elementImages_;
    bool hasTextParam_ = false;
    bool hasHomeName_ = false;
    bool hasAwayName_ = false;
    bool hasScore_ = false;
    QElapsedTimer packageClock_;
};

class ImageProvider final : public SourceProvider {
public:
    explicit ImageProvider(const core::InputDescriptor& input)
        : path_(param(input, "file"))
    {
        load();
    }

    QImage frame() override
    {
        if (image_.isNull() && !path_.isEmpty()) {
            load();
        }
        return image_;
    }

    QString detail() const override
    {
        return image_.isNull() ? QStringLiteral("Missing image: %1").arg(QFileInfo(path_).fileName())
                               : QFileInfo(path_).fileName();
    }

    bool ready() const override { return !image_.isNull(); }

private:
    void load()
    {
        image_ = QImage(path_);
    }

    QString path_;
    QImage image_;
};

class ScreenCaptureProvider final : public SourceProvider {
public:
    explicit ScreenCaptureProvider(const core::InputDescriptor& input)
        : monitor_(param(input, "monitor", "0").toInt())
    {
    }

    QImage frame() override
    {
        const auto screens = QGuiApplication::screens();
        if (screens.isEmpty()) {
            return {};
        }
        QScreen* screen = screens[std::clamp(monitor_, 0, static_cast<int>(screens.size()) - 1)];
        if (!screen) {
            return {};
        }
        const QPixmap pixmap = screen->grabWindow(0);
        return pixmap.toImage();
    }

    QString detail() const override
    {
        const auto screens = QGuiApplication::screens();
        if (monitor_ < 0 || monitor_ >= screens.size()) {
            return QStringLiteral("Screen %1").arg(monitor_ + 1);
        }
        return QStringLiteral("Screen %1 · %2x%3")
            .arg(monitor_ + 1)
            .arg(screens[monitor_]->geometry().width())
            .arg(screens[monitor_]->geometry().height());
    }

private:
    int monitor_ = 0;
};

class WindowCaptureProvider final : public SourceProvider {
public:
    explicit WindowCaptureProvider(const core::InputDescriptor& input)
        : handle_(param(input, "window").toULongLong())
        , title_(param(input, "windowTitle"))
    {
    }

    QImage frame() override
    {
        if (handle_ == 0) {
            return {};
        }
        const auto screens = QGuiApplication::screens();
        if (screens.isEmpty()) {
            return {};
        }
        QScreen* screen = screens.first();

#ifdef Q_OS_WIN
        auto* handle = reinterpret_cast<HWND>(handle_);
        if (handle && IsWindow(handle)) {
            // A window sized capture keeps the source aligned to the client area.
            const QPixmap windowPixmap = screen->grabWindow(static_cast<WId>(handle_));
            if (!windowPixmap.isNull() && windowPixmap.width() > 8 && windowPixmap.height() > 8) {
                return windowPixmap.toImage();
            }

            RECT rect = {};
            if (GetWindowRect(handle, &rect)) {
                const int width = rect.right - rect.left;
                const int height = rect.bottom - rect.top;
                if (width > 8 && height > 8) {
                    return screen->grabWindow(0, rect.left, rect.top, width, height).toImage();
                }
            }
            return {};
        }
#endif
        return {};
    }

    QString detail() const override
    {
        return title_.isEmpty() ? QStringLiteral("Window capture") : title_;
    }

private:
    quintptr handle_ = 0;
    QString title_;
};

class CameraProvider final : public SourceProvider {
public:
    explicit CameraProvider(const core::InputDescriptor& input)
        : deviceId_(param(input, "device"))
    {
        const auto devices = QMediaDevices::videoInputs();
        if (devices.isEmpty()) {
            return;
        }

        QCameraDevice device = devices.front();
        if (!deviceId_.isEmpty()) {
            for (const auto& candidate : devices) {
                if (QString::fromUtf8(candidate.id()) == deviceId_) {
                    device = candidate;
                    break;
                }
            }
        }

        sink_ = new QVideoSink();
        camera_ = new QCamera(device);
        session_ = new QMediaCaptureSession();
        session_->setCamera(camera_);
        session_->setVideoSink(sink_);
        QObject::connect(sink_, &QVideoSink::videoFrameChanged, sink_,
            [this](const QVideoFrame& frame) {
                if (frame.isValid()) {
                    const QImage image = frame.toImage();
                    if (!image.isNull()) {
                        latest_ = image;
                    }
                }
            });
        QObject::connect(camera_, &QCamera::errorOccurred, camera_,
            [this](QCamera::Error, const QString& message) { error_ = message; });
        camera_->start();
    }

    ~CameraProvider() override
    {
        if (camera_) {
            camera_->stop();
            delete camera_;
            camera_ = nullptr;
        }
        delete session_;
        delete sink_;
    }

    QImage frame() override { return latest_; }

    QString detail() const override
    {
        if (!error_.isEmpty()) {
            return error_;
        }
        return camera_ ? QStringLiteral("Camera: %1").arg(camera_->cameraDevice().description())
                       : QStringLiteral("No camera found");
    }

    bool ready() const override { return !latest_.isNull(); }

private:
    QString deviceId_;
    QVideoSink* sink_ = nullptr;
    QCamera* camera_ = nullptr;
    QMediaCaptureSession* session_ = nullptr;
    QImage latest_;
    QString error_;
};

class MediaFileProvider final : public SourceProvider {
public:
    explicit MediaFileProvider(const core::InputDescriptor& input)
        : path_(param(input, "file"))
    {
        sink_ = new QVideoSink();
        player_ = new QMediaPlayer();
        audio_ = new QAudioOutput();
        player_->setAudioOutput(audio_);
        audio_->setVolume(0.0F);
        player_->setVideoSink(sink_);
        QObject::connect(sink_, &QVideoSink::videoFrameChanged, sink_,
            [this](const QVideoFrame& frame) {
                if (frame.isValid()) {
                    const QImage image = frame.toImage();
                    if (!image.isNull()) {
                        latest_ = image;
                    }
                }
            });
        QObject::connect(player_, &QMediaPlayer::errorOccurred, player_,
            [this](QMediaPlayer::Error, const QString& message) { error_ = message; });
        if (!path_.isEmpty()) {
            player_->setSource(QUrl::fromLocalFile(path_));
            player_->setLoops(QMediaPlayer::Infinite);
            player_->play();
        }
    }

    ~MediaFileProvider() override
    {
        if (player_) {
            player_->stop();
            delete player_;
        }
        delete audio_;
        delete sink_;
    }

    QImage frame() override { return latest_; }

    QString detail() const override
    {
        if (!error_.isEmpty()) {
            return error_;
        }
        return QFileInfo(path_).fileName();
    }

    bool ready() const override { return !latest_.isNull(); }

private:
    QString path_;
    QVideoSink* sink_ = nullptr;
    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audio_ = nullptr;
    QImage latest_;
    QString error_;
};

} // namespace

std::unique_ptr<SourceProvider> createSourceProvider(const core::InputDescriptor& input)
{
    switch (input.kind) {
    case core::InputKind::TestPattern:
        return std::make_unique<TestPatternProvider>(input);
    case core::InputKind::Color:
        return std::make_unique<ColorProvider>(input);
    case core::InputKind::Title:
        return std::make_unique<TitleProvider>(input);
    case core::InputKind::Image:
        return std::make_unique<ImageProvider>(input);
    case core::InputKind::ScreenCapture:
        return std::make_unique<ScreenCaptureProvider>(input);
    case core::InputKind::WindowCapture:
        return std::make_unique<WindowCaptureProvider>(input);
    case core::InputKind::Camera:
        return std::make_unique<CameraProvider>(input);
    case core::InputKind::MediaFile:
        return std::make_unique<MediaFileProvider>(input);
    default:
        return nullptr;
    }
}

} // namespace mixvera::ui