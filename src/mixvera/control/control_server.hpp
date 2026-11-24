#pragma once

#include "mixvera/core/event_bus.hpp"
#include "mixvera/core/json_document.hpp"
#include <string>

namespace mixvera::control {

class ControlServer final {
public:
    explicit ControlServer(core::EventBus& bus);

    core::Json handleRequest(const std::string& requestJson);
    void publishEvent(const core::Event& event);

private:
    core::EventBus& bus_;
};

} // namespace mixvera::control
