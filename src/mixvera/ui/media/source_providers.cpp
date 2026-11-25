#include "mixvera/ui/media/source_provider.hpp"

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
    explicit TitleProvider(core::InputDescriptor input)
        : text_(param(input, "text", "TITLE"))
        , subtitle_(param(input, "subtitle"))
        , style_(param(input, "style", "full"))
        , color_(param(input, "color", "#ffffff"))
        , size_(param(input, "size", "64").toInt())
        , align_(param(input, "align", "center"))
        , durationMs_(param(input, "countdownSeconds", "300").toLongLong() * 1000)
        , endMs_(param(input, "countdownEnd", "0").toLongLong())
        , remainingMs_(param(input, "countdownRemaining", "").toLongLong())
        , paused_(param(input, "countdownPaused", "0") == "1")
        , competition_(param(input, "competition"))
        , homeName_(param(input, "homeName", "HOME"))
        , awayName_(param(input, "awayName", "AWAY"))
        , homeScore_(param(input, "scoreHome", "0").toInt())
        , awayScore_(param(input, "scoreAway", "0").toInt())
    {
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

    QImage frame() override
    {
        QImage image(kNativeWidth, kNativeHeight, QImage::Format_ARGB32_Premultiplied);
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
        } else {
            drawFullScreen(painter);
        }
        return image;
    }

    QString detail() const override
    {
        if (style_ == "countdown") {
            const qint64 seconds = (std::max<qint64>(remainingMs(), 0) + 999) / 1000;
            return QStringLiteral("Countdown: %1:%2")
                .arg(seconds / 60, 2, 10, QLatin1Char('0'))
                .arg(seconds % 60, 2, 10, QLatin1Char('0'));
        }
        if (style_ == "scoreboard") {
            return QStringLiteral("Score: %1 %2 : %3 %4")
                .arg(homeName_)
                .arg(homeScore_)
                .arg(awayScore_)
                .arg(awayName_);
        }
        if (style_ == "lower") {
            return QStringLiteral("Lower third: %1").arg(text_.left(32));
        }
        return QStringLiteral("Title: %1").arg(text_.left(32));
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

    qint64 remainingMs() const
    {
        if (endMs_ > 0) {
            return endMs_ - QDateTime::currentMSecsSinceEpoch();
        }
        return remainingMs_;
    }

    void drawFullScreen(QPainter& painter)
    {
        QFont font(QStringLiteral("Bahnschrift"), std::max(8, size_), QFont::Bold);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        painter.setFont(font);

        const QRect box(60, 60, kNativeWidth - 120, kNativeHeight - 120);
        painter.setPen(QColor(0, 0, 0, 170));
        painter.drawText(box.translated(3, 3),
            alignment() | Qt::AlignVCenter | Qt::TextWordWrap, text_);
        painter.setPen(color_);
        painter.drawText(box, alignment() | Qt::AlignVCenter | Qt::TextWordWrap, text_);
    }

    void drawLowerThird(QPainter& painter)
    {
        const int margin = 64;
        const int panelHeight = 150;
        const int textWidth = 760;

        QFont mainFont(QStringLiteral("Bahnschrift"),
            std::clamp(size_, 32, 72), QFont::Bold);
        QFont subFont(QStringLiteral("Bahnschrift"), 24, QFont::Normal);
        QFontMetrics mainMetrics(mainFont);
        QFontMetrics subMetrics(subFont);
        const QString mainText = mainMetrics.elidedText(text_, Qt::ElideRight, textWidth);
        const QString subText = subMetrics.elidedText(subtitle_, Qt::ElideRight, textWidth);

        int panelWidth = std::max(440,
            std::max(mainMetrics.horizontalAdvance(mainText),
                subMetrics.horizontalAdvance(subText))
                + 64);
        panelWidth = std::min(panelWidth, kNativeWidth - margin * 2);
        const QRect panel(margin, kNativeHeight - 80 - panelHeight, panelWidth, panelHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 218));
        painter.drawRoundedRect(panel, 4, 4);
        painter.fillRect(QRect(panel.left(), panel.top(), 10, panel.height()), color_);
        painter.setPen(QPen(QColor(255, 255, 255, 28), 1));
        painter.drawLine(panel.left(), panel.top(), panel.right(), panel.top());

        painter.setFont(mainFont);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(QRect(panel.left() + 30, panel.top() + 22, panel.width() - 48, 58),
            Qt::AlignLeft | Qt::AlignVCenter, mainText);
        if (!subText.isEmpty()) {
            painter.setFont(subFont);
            painter.setPen(QColor("#c9cfd8"));
            painter.drawText(QRect(panel.left() + 30, panel.top() + 84, panel.width() - 48, 36),
                Qt::AlignLeft | Qt::AlignVCenter, subText);
        }
    }

    void drawCountdown(QPainter& painter)
    {
        const qint64 total = std::max<qint64>(durationMs_, 1);
        const qint64 remaining = std::clamp<qint64>(remainingMs(), 0, total);
        const int cardWidth = 560;
        const int cardHeight = 260;
        const QRect card((kNativeWidth - cardWidth) / 2, (kNativeHeight - cardHeight) / 2,
            cardWidth, cardHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 220));
        painter.drawRoundedRect(card, 18, 18);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(color_, 2));
        painter.drawRoundedRect(card.adjusted(1, 1, -1, -1), 18, 18);

        QFont labelFont(QStringLiteral("Bahnschrift"), 24, QFont::DemiBold);
        labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
        painter.setFont(labelFont);
        painter.setPen(QColor("#aeb7c0"));
        const QString label = paused_
            ? QCoreApplication::translate("Title", "PAUSED")
            : (text_.isEmpty() ? QCoreApplication::translate("Title", "COUNTDOWN")
                               : text_.toUpper());
        painter.drawText(QRect(card.left(), card.top() + 26, card.width(), 34),
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
        QFont timerFont(QStringLiteral("Consolas"), 92, QFont::Bold);
        painter.setFont(timerFont);
        painter.setPen(remaining <= 0 ? QColor("#e85e75") : QColor("#ffffff"));
        painter.drawText(QRect(card.left(), card.top() + 68, card.width(), 124),
            Qt::AlignCenter, timer);

        const QRect track(card.left() + 40, card.bottom() - 48, card.width() - 80, 8);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 28));
        painter.drawRoundedRect(track, 4, 4);
        QRect fill = track;
        fill.setWidth(static_cast<int>(track.width()
            * (static_cast<double>(remaining) / static_cast<double>(total))));
        painter.setBrush(remaining <= 0 ? QColor("#e85e75") : color_);
        painter.drawRoundedRect(fill, 4, 4);
    }

    void drawScoreboard(QPainter& painter)
    {
        const int cardWidth = 820;
        const int cardHeight = 158;
        const QRect card((kNativeWidth - cardWidth) / 2, 42, cardWidth, cardHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(8, 10, 14, 220));
        painter.drawRoundedRect(card, 12, 12);
        painter.fillRect(QRect(card.left(), card.top(), card.width(), 6), color_);

        QFont nameFont(QStringLiteral("Bahnschrift"), 28, QFont::DemiBold);
        QFont scoreFont(QStringLiteral("Consolas"), 60, QFont::Bold);
        QFont titleFont(QStringLiteral("Bahnschrift"), 20, QFont::Normal);
        QFontMetrics nameMetrics(nameFont);

        if (!competition_.isEmpty()) {
            painter.setFont(titleFont);
            painter.setPen(QColor("#aeb7c0"));
            painter.drawText(QRect(card.left(), card.top() + 14, card.width(), 26),
                Qt::AlignHCenter, competition_.toUpper());
        }

        painter.setFont(nameFont);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(QRect(card.left() + 28, card.top() + 84, 210, 44),
            Qt::AlignLeft | Qt::AlignVCenter,
            nameMetrics.elidedText(homeName_, Qt::ElideRight, 210));
        painter.drawText(QRect(card.left() + 582, card.top() + 84, 210, 44),
            Qt::AlignRight | Qt::AlignVCenter,
            nameMetrics.elidedText(awayName_, Qt::ElideRight, 210));

        painter.setFont(scoreFont);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(QRect(card.left() + 238, card.top() + 46, 120, 80),
            Qt::AlignRight | Qt::AlignVCenter, QString::number(homeScore_));
        painter.drawText(QRect(card.left() + 462, card.top() + 46, 120, 80),
            Qt::AlignLeft | Qt::AlignVCenter, QString::number(awayScore_));

        painter.setFont(nameFont);
        painter.setPen(QColor("#8e98a3"));
        painter.drawText(QRect(card.left() + 398, card.top() + 46, 24, 80),
            Qt::AlignCenter, QStringLiteral(":"));
    }

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