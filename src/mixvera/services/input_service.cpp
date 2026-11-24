#include "mixvera/services/input_service.hpp"
#include <algorithm>

namespace mixvera::services {

InputService::InputService(core::EventBus& bus)
    : bus_(bus)
{
    add({"camera-1", "CAM 1", core::InputKind::TestPattern});
    add({"camera-2", "CAM 2", core::InputKind::TestPattern});
    add({"camera-3", "CAM 3", core::InputKind::TestPattern});
    add({"camera-4", "CAM 4", core::InputKind::TestPattern});
}

const core::InputDescriptor* InputService::find(const std::string& id) const
{
    const auto it = std::find_if(inputs_.begin(), inputs_.end(), [&](const auto& input) {
        return input.id == id;
    });
    return it == inputs_.end() ? nullptr : &*it;
}

void InputService::add(core::InputDescriptor descriptor)
{
    inputs_.push_back(std::move(descriptor));
    bus_.publish({"input/added", inputs_.back().id});
}

bool InputService::rename(const std::string& id, const std::string& name)
{
    auto it = std::find_if(inputs_.begin(), inputs_.end(), [&](const auto& input) {
        return input.id == id;
    });
    if (it == inputs_.end()) {
        return false;
    }
    it->name = name;
    bus_.publish({"input/renamed", it->id + ":" + name});
    return true;
}

bool InputService::remove(const std::string& id)
{
    const auto before = inputs_.size();
    inputs_.erase(std::remove_if(inputs_.begin(), inputs_.end(), [&](const auto& input) {
                      return input.id == id;
                  }),
        inputs_.end());
    if (inputs_.size() == before) {
        return false;
    }
    bus_.publish({"input/removed", id});
    return true;
}

} // namespace mixvera::services
