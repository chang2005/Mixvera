#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"

namespace mixvera::services {

class ReplayService final {
public:
    explicit ReplayService(core::EventBus& bus);

    const core::ReplayState& state() const { return state_; }
    void setRecording(bool enabled);
    void setSpeedPercent(std::uint32_t percent);
    void saveReplay();

private:
    core::EventBus& bus_;
    core::ReplayState state_;
};

} // namespace mixvera::services
