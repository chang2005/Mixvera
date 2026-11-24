#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"
#include <string>
#include <vector>

namespace mixvera::services {

class DeviceService final {
public:
    explicit DeviceService(core::EventBus& bus);

    const std::vector<core::DeviceState>& devices() const { return devices_; }
    void refresh();

private:
    core::EventBus& bus_;
    std::vector<core::DeviceState> devices_;
};

} // namespace mixvera::services
