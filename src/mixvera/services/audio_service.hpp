#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"
#include <string>
#include <vector>

namespace mixvera::services {

class AudioService final {
public:
    explicit AudioService(core::EventBus& bus);

    const std::vector<core::AudioChannelState>& channels() const { return channels_; }
    void addChannel(const std::string& inputId);
    void clear();
    bool setGain(const std::string& inputId, float gainDb);
    bool setMuted(const std::string& inputId, bool muted);
    bool setMixerMask(const std::string& inputId, std::uint32_t mask);

private:
    core::AudioChannelState* find(const std::string& inputId);

    core::EventBus& bus_;
    std::vector<core::AudioChannelState> channels_;
};

} // namespace mixvera::services
