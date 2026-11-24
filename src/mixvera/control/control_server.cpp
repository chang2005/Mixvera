#include "mixvera/control/control_server.hpp"

namespace mixvera::control {

ControlServer::ControlServer(core::EventBus& bus)
    : bus_(bus)
{
}

core::Json ControlServer::handleRequest(const std::string& requestJson)
{
    const auto request = core::parseJson(requestJson);
    core::Json response;
    response["jsonrpc"] = "2.0";

    if (!request.is_object() || !request.contains("method")) {
        response["error"] = {{"code", -32600}, {"message", "Invalid Request"}};
        return response;
    }

    const auto method = request["method"].get<std::string>();
    response["id"] = request.value("id", core::Json(nullptr));

    if (method == "GetStatus") {
        response["result"] = {{"product", "Mixvera"}, {"version", "0.1.0"}, {"state", "ready"}};
        bus_.publish({"control/request", method});
        return response;
    }

    response["error"] = {{"code", -32601}, {"message", "Method not found"}};
    return response;
}

void ControlServer::publishEvent(const core::Event& event)
{
    core::Json message;
    message["jsonrpc"] = "2.0";
    message["method"] = event.topic;
    message["params"] = {{"payload", event.payload}};
    bus_.publish({"control/event", core::dumpJson(message)});
}

} // namespace mixvera::control
