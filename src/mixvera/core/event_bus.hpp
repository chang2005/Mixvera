#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mixvera::core {

struct Event {
    std::string topic;
    std::string payload;
};

class EventBus final {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(const std::string& topic, Handler handler);
    void publish(const Event& event) const;
    std::size_t handlerCount(const std::string& topic) const;

private:
    std::unordered_map<std::string, std::vector<Handler>> handlers_;
};

using EventBusPtr = std::shared_ptr<EventBus>;

} // namespace mixvera::core
