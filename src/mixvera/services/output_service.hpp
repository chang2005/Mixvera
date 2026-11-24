#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"
#include <string>
#include <vector>

namespace mixvera::services {

class OutputService final {
public:
    explicit OutputService(core::EventBus& bus);

    const std::vector<core::OutputState>& outputs() const { return outputs_; }
    bool start(const std::string& id);
    bool stop(const std::string& id);

private:
    core::EventBus& bus_;
    std::vector<core::OutputState> outputs_;
};

} // namespace mixvera::services
