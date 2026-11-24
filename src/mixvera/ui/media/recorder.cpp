#include "mixvera/ui/media/recorder.hpp"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <algorithm>

namespace mixvera::ui {

namespace {

QString encoderArgument()
{
    static QString cached;
    if (!cached.isEmpty()) {
        return cached;
    }
    cached = "mpeg4";
    const QString ffmpeg = QStandardPaths::findExecutable("ffmpeg");
    if (ffmpeg.isEmpty()) {
        return cached;
    }
    QProcess probe;
    probe.start(ffmpeg, {"-hide_banner", "-encoders"});
    if (probe.waitForFinished(4000)) {
        const QString output = QString::fromUtf8(probe.readAllStandardOutput());
        if (output.contains("libx264")) {
            cached = "libx264";
        }
    }
    return cached;
}

} // namespace

Recorder::Recorder(QObject* parent)
    : QObject(parent)
{
}

Recorder::~Recorder()
{
    stopRecording();
    stopStreaming();
}

QString Recorder::ffmpegPath()
{
    return QStandardPaths::findExecutable("ffmpeg");
}

bool Recorder::available()
{
    return !ffmpegPath().isEmpty();
}

bool Recorder::launch(QProcess*& process, const QStringList& arguments, const QString& label,
    QString* error)
{
    const QString ffmpeg = ffmpegPath();
    if (ffmpeg.isEmpty()) {
        lastError_ = tr("ffmpeg was not found on PATH");
        if (error) {
            *error = lastError_;
        }
        return false;
    }

    process = new QProcess(this);
    process->setProgram(ffmpeg);
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->start();
    if (!process->waitForStarted(4000)) {
        lastError_ = tr("%1: ffmpeg did not start").arg(label);
        if (error) {
            *error = lastError_;
        }
        delete process;
        process = nullptr;
        return false;
    }
    return true;
}

bool Recorder::startRecording(const QString& path, QSize size, int fps, QString* error)
{
    if (recording()) {
        return true;
    }
    if (path.isEmpty()) {
        lastError_ = tr("No recording path configured");
        if (error) {
            *error = lastError_;
        }
        return false;
    }

    const QFileInfo info(path);
    if (!info.absoluteDir().exists()) {
        QDir().mkpath(info.absolutePath());
    }

    size_ = size;
    fps_ = std::clamp(fps, 1, 120);

    const QStringList arguments = {
        "-hide_banner",
        "-loglevel", "error",
        "-f", "rawvideo",
        "-pix_fmt", "bgra",
        "-s", QString("%1x%2").arg(size_.width()).arg(size_.height()),
        "-r", QString::number(fps_),
        "-i", "pipe:0",
        "-an",
        "-c:v", encoderArgument(),
        "-preset", "veryfast",
        "-pix_fmt", "yuv420p",
        "-movflags", "+faststart",
        "-y", path,
    };

    if (!launch(recordProcess_, arguments, tr("Recording"), error)) {
        return false;
    }
    recordingPath_ = path;
    recordClock_.restart();
    return true;
}

void Recorder::stopRecording()
{
    if (!recordingPath_.isEmpty()) {
        recordingPath_.clear();
    }
    shutdown(recordProcess_);
}

bool Recorder::startStreaming(const QString& url, QSize size, int fps, QString* error)
{
    if (streaming()) {
        return true;
    }
    if (url.isEmpty()) {
        lastError_ = tr("No stream URL configured");
        if (error) {
            *error = lastError_;
        }
        return false;
    }

    size_ = size;
    fps_ = std::clamp(fps, 1, 120);

    const QStringList arguments = {
        "-hide_banner",
        "-loglevel", "error",
        "-f", "rawvideo",
        "-pix_fmt", "bgra",
        "-s", QString("%1x%2").arg(size_.width()).arg(size_.height()),
        "-r", QString::number(fps_),
        "-i", "pipe:0",
        "-an",
        "-c:v", "libx264",
        "-preset", "veryfast",
        "-tune", "zerolatency",
        "-b:v", "4500k",
        "-maxrate", "4500k",
        "-bufsize", "9000k",
        "-pix_fmt", "yuv420p",
        "-f", "flv",
        url,
    };

    if (!launch(streamProcess_, arguments, tr("Streaming"), error)) {
        return false;
    }
    streamClock_.restart();
    return true;
}

void Recorder::stopStreaming()
{
    shutdown(streamProcess_);
}

void Recorder::writeFrame(QProcess* process, const QImage& canvas)
{
    if (!process || process->state() != QProcess::Running) {
        return;
    }

    QImage frame = canvas;
    if (frame.size() != size_) {
        frame = frame.scaled(size_, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    if (frame.format() != QImage::Format_ARGB32) {
        frame = frame.convertToFormat(QImage::Format_ARGB32);
    }

    const int lineBytes = size_.width() * 4;
    if (frame.bytesPerLine() == lineBytes) {
        process->write(reinterpret_cast<const char*>(frame.constBits()), lineBytes * size_.height());
        return;
    }

    QByteArray buffer;
    buffer.reserve(lineBytes * size_.height());
    for (int row = 0; row < size_.height(); ++row) {
        buffer.append(reinterpret_cast<const char*>(frame.constScanLine(row)), lineBytes);
    }
    process->write(buffer);
}

void Recorder::submit(const QImage& canvas)
{
    if (canvas.isNull()) {
        return;
    }
    writeFrame(recordProcess_, canvas);
    writeFrame(streamProcess_, canvas);
}

void Recorder::shutdown(QProcess*& process)
{
    if (!process) {
        return;
    }
    if (process->state() == QProcess::Running) {
        process->closeWriteChannel();
        if (!process->waitForFinished(4000)) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
    const QString errors = QString::fromUtf8(process->readAllStandardError()).trimmed();
    if (!errors.isEmpty()) {
        lastError_ = errors;
    }
    delete process;
    process = nullptr;
}

qint64 Recorder::recordingMillis() const
{
    return recordingPath_.isEmpty() ? 0 : recordClock_.elapsed();
}

qint64 Recorder::streamingMillis() const
{
    return streaming() ? streamClock_.elapsed() : 0;
}

} // namespace mixvera::ui