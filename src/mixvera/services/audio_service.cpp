#include "mixvera/services/audio_service.hpp"
#include <algorithm>

namespace mixvera::services {

AudioService::AudioService(core::EventBus& bus)
    : bus_(bus)
{
}

core::AudioChannelState* AudioService::find(const std::string& inputId)
{
    auto it = std::find_if(channels_.begin(), channels_.end(), [&](const auto& channel) {
        return channel.inputId == inputId;
    });
    return it == channels_.end() ? nullptr : &*it;
}

void AudioService::addChannel(const std::string& inputId)
{
    if (find(inputId) == nullptr) {
        channels_.push_back({inputId, 0.0F, 0.0F, false, 1});
        bus_.publish({"audio/channel_added", inputId});
    }
}

bool AudioService::setGain(const std::string& inputId, float gainDb)
{
    auto* channel = find(inputId);
    if (!channel) {
        return false;
    }
    channel->gainDb = std::clamp(gainDb, -60.0F, 12.0F);
    bus_.publish({"audio/gain_changed", inputId + ":" + std::to_string(channel->gainDb)});
    return true;
}

bool AudioService::setMuted(const std::string& inputId, bool muted)
{
    auto* channel = find(inputId);
    if (!channel) {
        return false;
    }
    channel->muted = muted;
    bus_.publish({"audio/mute_changed", inputId + ":" + (muted ? "1" : "0")});
    return true;
}

bool AudioService::setMixerMask(const std::string& inputId, std::uint32_t mask)
{
    auto* channel = find(inputId);
    if (!channel) {
        return false;
    }
    channel->mixerMask = mask & 0x3F;
    bus_.publish({"audio/routing_changed", inputId});
    return true;
}

} // namespace mixvera::services
