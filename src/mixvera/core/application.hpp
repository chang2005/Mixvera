#pragma once

#include "mixvera/core/event_bus.hpp"
#include "mixvera/core/service_container.hpp"
#include <memory>
#include <string>

namespace mixvera::core {

class Application final {
public:
    Application();
    ~Application();

    int initialize();
    int run();
    void shutdown();

    EventBus& events() { return *events_; }
    ServiceContainer& services() { return *services_; }
    const std::string& status() const { return status_; }
    void setStatus(std::string status);

    static Application& instance();

private:
    EventBusPtr events_;
    std::unique_ptr<ServiceContainer> services_;
    std::string status_ = "initialized";
};

} // namespace mixvera::core
