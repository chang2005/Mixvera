#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>

class QProcess;

namespace mixvera::ui {

/// Streams composed canvas frames into ffmpeg: H.264 recording and RTMP push.
/// ffmpeg is the only external dependency and is looked up on PATH.
class Recorder final : public QObject {
    Q_OBJECT

public:
    explicit Recorder(QObject* parent = nullptr);
    ~Recorder() override;

    static QString ffmpegPath();
    static bool available();

    bool startRecording(const QString& path, QSize size, int fps, QString* error);
    void stopRecording();
    bool recording() const { return recordProcess_ != nullptr; }
    QString recordingPath() const { return recordingPath_; }

    bool startStreaming(const QString& url, QSize size, int fps, QString* error);
    void stopStreaming();
    bool streaming() const { return streamProcess_ != nullptr; }

    void submit(const QImage& canvas);

    qint64 recordingMillis() const;
    qint64 streamingMillis() const;
    QString lastError() const { return lastError_; }

private:
    bool launch(QProcess*& process, const QStringList& arguments, const QString& label,
        QString* error);
    void writeFrame(QProcess* process, const QImage& canvas);
    void shutdown(QProcess*& process);

    QProcess* recordProcess_ = nullptr;
    QProcess* streamProcess_ = nullptr;
    QSize size_ = QSize(1920, 1080);
    int fps_ = 30;
    QString recordingPath_;
    QString lastError_;
    QElapsedTimer recordClock_;
    QElapsedTimer streamClock_;
};

} // namespace mixvera::ui