#include "mixvera/core/application.hpp"

#include "mixvera/services/audio_service.hpp"
#include "mixvera/services/device_service.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/output_service.hpp"
#include "mixvera/services/replay_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/services/transition_service.hpp"

#include <iostream>

namespace mixvera::core {

Application::Application()
    : events_(std::make_shared<EventBus>())
    , services_(std::make_unique<ServiceContainer>())
{
}

Application::~Application() = default;

Application& Application::instance()
{
    static Application app;
    return app;
}

int Application::initialize()
{
    auto& services = *services_;
    auto& inputs = services.add<services::InputService>(*events_);
    auto& switcher = services.add<services::SwitcherService>(*events_, &inputs);
    services.add<services::TransitionService>(*events_, &switcher);
    auto& audio = services.add<services::AudioService>(*events_);
    for (const auto& input : inputs.inputs()) {
        audio.addChannel(input.id);
    }
    services.add<services::OutputService>(*events_);
    services.add<services::ReplayService>(*events_);
    services.add<services::DeviceService>(*events_);

    events_->publish({"app/initialized", "Mixvera core services ready"});
    return 0;
}

int Application::run()
{
    setStatus("running");
    events_->publish({"app/started", status_});
    return 0;
}

void Application::shutdown()
{
    setStatus("stopped");
    events_->publish({"app/stopped", status_});
}

void Application::setStatus(std::string status)
{
    status_ = std::move(status);
    events_->publish({"app/status", status_});
}

} // namespace mixvera::core
