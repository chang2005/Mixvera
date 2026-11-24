#include "mixvera/ui/media/audio_meter.hpp"

#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mixvera::ui {

AudioMeter::AudioMeter(QObject* parent)
    : QObject(parent)
    , timer_(new QTimer(this))
{
    timer_->setInterval(60);
    connect(timer_, &QTimer::timeout, this, [this] { poll(); });
}

AudioMeter::~AudioMeter()
{
    stop();
}

bool AudioMeter::start()
{
    if (source_) {
        return true;
    }

    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        lastError_ = tr("No audio input device");
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!device.isFormatSupported(format)) {
        format = device.preferredFormat();
    }

    source_ = new QAudioSource(device, format, this);
    device_ = source_->start();
    if (!device_) {
        lastError_ = tr("Audio input could not start");
        source_->deleteLater();
        source_ = nullptr;
        return false;
    }

    deviceName_ = device.description();
    timer_->start();
    return true;
}

void AudioMeter::stop()
{
    timer_->stop();
    if (source_) {
        source_->stop();
        source_->deleteLater();
        source_ = nullptr;
    }
    device_ = nullptr;
    levelDb_ = -60.0F;
}

void AudioMeter::poll()
{
    if (!device_) {
        return;
    }

    const QByteArray data = device_->readAll();
    if (data.isEmpty()) {
        levelDb_ = std::max(-60.0F, levelDb_ - 1.5F);
        return;
    }

    const auto* samples = reinterpret_cast<const std::int16_t*>(data.constData());
    const int count = data.size() / static_cast<int>(sizeof(std::int16_t));
    if (count <= 0) {
        return;
    }

    double sum = 0.0;
    for (int index = 0; index < count; ++index) {
        const double value = static_cast<double>(samples[index]) / 32768.0;
        sum += value * value;
    }
    const double rms = std::sqrt(sum / count);
    const float db = rms <= 1e-6 ? -60.0F
                                : static_cast<float>(20.0 * std::log10(rms));
    // Fast attack, slow release reads like a real peak meter.
    levelDb_ = std::clamp(std::max(db, levelDb_ - 2.2F), -60.0F, 0.0F);
}

} // namespace mixvera::ui