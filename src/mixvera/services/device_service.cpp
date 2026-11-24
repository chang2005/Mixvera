#include "mixvera/services/device_service.hpp"

namespace mixvera::services {

DeviceService::DeviceService(core::EventBus& bus)
    : bus_(bus)
{
    devices_.push_back({"decklink-0", "DeckLink 0", false});
    devices_.push_back({"decklink-1", "DeckLink 1", false});
    devices_.push_back({"tbar-hid", "T-Bar HID", false});
}

void DeviceService::refresh()
{
    for (auto& device : devices_) {
        device.connected = !device.connected;
        bus_.publish({"device/connection_changed", device.id + ":" + (device.connected ? "1" : "0")});
    }
}

} // namespace mixvera::services
