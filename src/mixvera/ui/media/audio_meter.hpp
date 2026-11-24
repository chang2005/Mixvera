#pragma once

#include <QObject>
#include <QString>

class QAudioSource;
class QIODevice;
class QTimer;

namespace mixvera::ui {

/// Reads the default input device and exposes a smoothed peak level so the
/// mixer shows a real signal instead of a decorative animation.
class AudioMeter final : public QObject {
    Q_OBJECT

public:
    explicit AudioMeter(QObject* parent = nullptr);
    ~AudioMeter() override;

    bool start();
    void stop();
    bool active() const { return source_ != nullptr; }

    /// Current level in dBFS, clamped to -60..0.
    float levelDb() const { return levelDb_; }
    QString deviceName() const { return deviceName_; }
    QString lastError() const { return lastError_; }

private:
    void poll();

    QAudioSource* source_ = nullptr;
    QIODevice* device_ = nullptr;
    QTimer* timer_ = nullptr;
    QString deviceName_;
    QString lastError_;
    float levelDb_ = -60.0F;
};

} // namespace mixvera::ui