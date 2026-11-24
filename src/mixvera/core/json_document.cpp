#include "mixvera/core/json_document.hpp"

namespace mixvera::core {

Json parseJson(const std::string& text)
{
    return Json::parse(text, nullptr, false);
}

std::string dumpJson(const Json& value)
{
    return value.dump();
}

} // namespace mixvera::core
