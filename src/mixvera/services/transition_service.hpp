#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"
#include "mixvera/services/switcher_service.hpp"

namespace mixvera::services {

class TransitionService final {
public:
    explicit TransitionService(core::EventBus& bus, SwitcherService* switcher = nullptr);

    void setSwitcher(SwitcherService& switcher) { switcher_ = &switcher; }
    void setKind(core::TransitionKind kind);
    void setDuration(std::uint32_t ms);
    void setPosition(float position);
    void release();

    const core::TransitionState& state() const { return state_; }

private:
    core::EventBus& bus_;
    SwitcherService* switcher_ = nullptr;
    core::TransitionState state_;
};

} // namespace mixvera::services
