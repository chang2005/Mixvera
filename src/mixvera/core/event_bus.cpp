#include "mixvera/core/event_bus.hpp"

namespace mixvera::core {

void EventBus::subscribe(const std::string& topic, Handler handler)
{
    handlers_[topic].push_back(std::move(handler));
}

void EventBus::publish(const Event& event) const
{
    const auto it = handlers_.find(event.topic);
    if (it == handlers_.end()) {
        return;
    }
    for (const auto& handler : it->second) {
        handler(event);
    }
}

std::size_t EventBus::handlerCount(const std::string& topic) const
{
    const auto it = handlers_.find(topic);
    return it == handlers_.end() ? 0 : it->second.size();
}

} // namespace mixvera::core
