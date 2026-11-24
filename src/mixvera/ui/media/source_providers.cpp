#include "mixvera/ui/media/source_provider.hpp"

#include "mixvera/ui/platform/win_capture.hpp"

#include <QAudioOutput>
#include <QCamera>
#include <QCameraDevice>
#include <QColor>
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
        , color_(param(input, "color", "#ffffff"))
        , size_(param(input, "size", "64").toInt())
        , align_(param(input, "align", "center"))
    {
        if (text_.isEmpty()) {
            text_ = "TITLE";
        }
    }

    QImage frame() override
    {
        QImage image(kNativeWidth, kNativeHeight, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        QFont font(QStringLiteral("Bahnschrift"), std::max(8, size_), QFont::Bold);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        painter.setFont(font);

        Qt::Alignment alignment = Qt::AlignLeft;
        if (align_ == "center") {
            alignment = Qt::AlignHCenter;
        } else if (align_ == "right") {
            alignment = Qt::AlignRight;
        }

        const QRect box(60, 60, kNativeWidth - 120, kNativeHeight - 120);
        painter.setPen(QColor(0, 0, 0, 170));
        painter.drawText(box.translated(3, 3), alignment | Qt::AlignVCenter | Qt::TextWordWrap, text_);
        painter.setPen(QColor(color_));
        painter.drawText(box, alignment | Qt::AlignVCenter | Qt::TextWordWrap, text_);
        return image;
    }

    QString detail() const override { return QStringLiteral("Title: %1").arg(text_.left(32)); }

private:
    QString text_;
    QString color_;
    int size_ = 64;
    QString align_;
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