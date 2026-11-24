#include "mixvera/services/output_service.hpp"
#include <algorithm>

namespace mixvera::services {

OutputService::OutputService(core::EventBus& bus)
    : bus_(bus)
{
    outputs_.push_back({"program-rtmp", "Program RTMP", false});
    outputs_.push_back({"program-ndi", "Program NDI", false});
    outputs_.push_back({"local-record", "Local Recording", false});
}

bool OutputService::start(const std::string& id)
{
    auto it = std::find_if(outputs_.begin(), outputs_.end(), [&](const auto& output) {
        return output.id == id;
    });
    if (it == outputs_.end() || it->running) {
        return false;
    }
    it->running = true;
    bus_.publish({"output/started", id});
    return true;
}

bool OutputService::stop(const std::string& id)
{
    auto it = std::find_if(outputs_.begin(), outputs_.end(), [&](const auto& output) {
        return output.id == id;
    });
    if (it == outputs_.end() || !it->running) {
        return false;
    }
    it->running = false;
    bus_.publish({"output/stopped", id});
    return true;
}

} // namespace mixvera::services
