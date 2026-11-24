#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/event_bus.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mixvera::services {

class InputService final {
public:
    explicit InputService(core::EventBus& bus);

    const core::InputDescriptor* find(const std::string& id) const;
    const std::vector<core::InputDescriptor>& inputs() const { return inputs_; }
    std::size_t count() const { return inputs_.size(); }

    void add(core::InputDescriptor descriptor);
    bool rename(const std::string& id, const std::string& name);
    bool remove(const std::string& id);

private:
    core::EventBus& bus_;
    std::vector<core::InputDescriptor> inputs_;
};

} // namespace mixvera::services
