#pragma once

#include <string>

namespace mixvera::engine {

class MediaEngine {
public:
    virtual ~MediaEngine() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual const char* backendName() const = 0;
};

class StubMediaEngine final : public MediaEngine {
public:
    bool initialize() override;
    void shutdown() override;
    const char* backendName() const override { return "stub"; }
};

} // namespace mixvera::engine
