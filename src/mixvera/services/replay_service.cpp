#include "mixvera/services/replay_service.hpp"
#include <algorithm>

namespace mixvera::services {

ReplayService::ReplayService(core::EventBus& bus)
    : bus_(bus)
{
}

void ReplayService::setRecording(bool enabled)
{
    state_.recording = enabled;
    bus_.publish({"replay/recording_changed", enabled ? "1" : "0"});
}

void ReplayService::setSpeedPercent(std::uint32_t percent)
{
    state_.speedPercent = std::clamp(percent, 10U, 400U);
    bus_.publish({"replay/speed_changed", std::to_string(state_.speedPercent)});
}

void ReplayService::saveReplay()
{
    bus_.publish({"replay/saved", std::to_string(state_.seconds)});
}

} // namespace mixvera::services
