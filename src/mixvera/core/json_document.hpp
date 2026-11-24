#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace mixvera::core {

using Json = nlohmann::json;

Json parseJson(const std::string& text);
std::string dumpJson(const Json& value);

} // namespace mixvera::core
