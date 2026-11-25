#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QFrame>
#include <QString>

#include <string>
#include <vector>

class QLabel;
class QScrollArea;
class QWidget;

namespace mixvera::ui {

/// vMix / OBS style audio mixer dock: vertical channel strips with a fader,
/// mute and solo per channel, plus a master level meter. The master meter is
/// driven by a real capture device; per-channel metering arrives with the
/// audio mixing engine.
class MixerPanel final : public QFrame {
    Q_OBJECT

public:
    explicit MixerPanel(QWidget* parent = nullptr);

    void setChannels(const std::vector<core::InputDescriptor>& inputs,
        const std::vector<core::AudioChannelState>& channels);
    void setMasterLevel(float levelDb);
    void setAudioDeviceStatus(const QString& text, bool active);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void gainChanged(const QString& inputId, float gainDb);
    void muteChanged(const QString& inputId, bool muted);
    void soloChanged(const QString& inputId, bool solo);

private:
    void rebuildChannels();
    void updateSoloState();

    QScrollArea* scroll_ = nullptr;
    QWidget* stripArea_ = nullptr;
    QLabel* deviceLabel_ = nullptr;
    QLabel* soloChip_ = nullptr;
    std::vector<core::InputDescriptor> inputs_;
    std::vector<core::AudioChannelState> channels_;
    QWidget* masterMeter_ = nullptr;
    QLabel* masterDb_ = nullptr;
};

} // namespace mixvera::ui
