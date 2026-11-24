#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"

namespace mixvera::services {

class InputService;

class SwitcherService final {
public:
    SwitcherService(core::EventBus& bus, InputService* inputs = nullptr);

    const core::ProgramPreviewState& state() const { return state_; }

    void setInputService(InputService& inputs) { inputs_ = &inputs; }
    bool setPreview(const std::string& inputId);
    bool take();
    bool cut();

private:
    void validateState() const;

    core::EventBus& bus_;
    InputService* inputs_ = nullptr;
    core::ProgramPreviewState state_;
};

} // namespace mixvera::services
