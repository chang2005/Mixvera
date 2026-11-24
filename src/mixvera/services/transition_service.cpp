#include "mixvera/services/transition_service.hpp"
#include <algorithm>

namespace mixvera::services {

TransitionService::TransitionService(core::EventBus& bus, SwitcherService* switcher)
    : bus_(bus)
    , switcher_(switcher)
{
}

void TransitionService::setKind(core::TransitionKind kind)
{
    state_.kind = kind;
    bus_.publish({"transition/kind_changed", std::to_string(static_cast<int>(kind))});
}

void TransitionService::setDuration(std::uint32_t ms)
{
    state_.durationMs = std::max<std::uint32_t>(ms, 50);
    bus_.publish({"transition/duration_changed", std::to_string(state_.durationMs)});
}

void TransitionService::setPosition(float position)
{
    state_.position = std::clamp(position, 0.0F, 1.0F);
    state_.active = state_.position > 0.0F && state_.position < 1.0F;
    bus_.publish({"transition/position_changed", std::to_string(state_.position)});
}

void TransitionService::release()
{
    const bool completed = state_.position >= 0.5F;
    state_.position = 0.0F;
    state_.active = false;
    if (completed && switcher_) {
        switcher_->take();
    }
    bus_.publish({"transition/released", completed ? "complete" : "cancel"});
}

} // namespace mixvera::services
