#pragma once

#include <cstdint>
#include <string>

namespace mixvera::core {

enum class InputKind {
    Unknown,
    TestPattern,
    DeckLink,
    Ndi,
    ScreenCapture,
};

enum class TransitionKind {
    Cut,
    Fade,
    Wipe,
    Stinger,
};

struct InputDescriptor {
    std::string id;
    std::string name;
    InputKind kind = InputKind::Unknown;
    bool active = true;
};

struct TransitionState {
    TransitionKind kind = TransitionKind::Cut;
    float position = 0.0F;
    std::uint32_t durationMs = 500;
    bool active = false;
};

struct ProgramPreviewState {
    std::string programInputId;
    std::string previewInputId;
    TransitionState transition;
};

struct AudioChannelState {
    std::string inputId;
    float gainDb = 0.0F;
    float pan = 0.0F;
    bool muted = false;
    std::uint32_t mixerMask = 1;
};

struct OutputState {
    std::string id;
    std::string name;
    bool running = false;
};

struct ReplayState {
    bool recording = false;
    std::uint32_t seconds = 30;
    std::uint32_t speedPercent = 100;
};

struct DeviceState {
    std::string id;
    std::string name;
    bool connected = false;
};

} // namespace mixvera::core
