#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QFrame>
#include <QString>

#include <string>
#include <vector>

class QLabel;
class QScrollArea;

namespace mixvera::ui {

/// Audio mixer dock. Faders, mute and the master meter; the meter is driven by
/// a real capture device when one is available.
class MixerPanel final : public QFrame {
    Q_OBJECT

public:
    explicit MixerPanel(QWidget* parent = nullptr);

    void setChannels(const std::vector<core::InputDescriptor>& inputs,
        const std::vector<core::AudioChannelState>& channels);
    void setMasterLevel(float levelDb);
    void setAudioDeviceStatus(const QString& text, bool active);

signals:
    void gainChanged(const QString& inputId, float gainDb);
    void muteChanged(const QString& inputId, bool muted);
    void masterGainChanged(float gainDb);

private:
    void rebuildChannels();

    QScrollArea* scroll_ = nullptr;
    QWidget* stripArea_ = nullptr;
    QLabel* deviceLabel_ = nullptr;
    std::vector<core::InputDescriptor> inputs_;
    std::vector<core::AudioChannelState> channels_;
    QWidget* masterMeter_ = nullptr;
};

} // namespace mixvera::ui