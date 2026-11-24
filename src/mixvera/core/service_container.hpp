#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace mixvera::core {

class ServiceContainer final {
public:
    template <typename T, typename... Args>
    T& add(Args&&... args)
    {
        const std::string key(typeid(T).name());
        if (services_.find(key) != services_.end()) {
            throw std::runtime_error("Service already registered");
        }

        auto service = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = service.get();
        services_.emplace(
            key,
            std::unique_ptr<void, void (*)(void*)>(service.release(), [](void* object) {
                delete static_cast<T*>(object);
            }));
        return *ptr;
    }

    template <typename T>
    T& get() const
    {
        const auto it = services_.find(typeid(T).name());
        if (it == services_.end()) {
            throw std::runtime_error("Service not registered");
        }
        return *static_cast<T*>(it->second.get());
    }

    template <typename T>
    bool has() const
    {
        return services_.find(typeid(T).name()) != services_.end();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<void, void (*)(void*)>> services_;
};

} // namespace mixvera::core
