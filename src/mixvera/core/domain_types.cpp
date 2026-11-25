#include "mixvera/core/domain_types.hpp"

#include <algorithm>

namespace mixvera::core {

namespace {

struct KindName {
    InputKind kind;
    const char* key;
    const char* label;
    bool audio;
};

const KindName kKinds[] = {
    {InputKind::Unknown, "unknown", "Unknown", false},
    {InputKind::TestPattern, "test-pattern", "Test Pattern", false},
    {InputKind::ScreenCapture, "screen", "Screen Capture", true},
    {InputKind::WindowCapture, "window", "Window Capture", true},
    {InputKind::Camera, "camera", "Camera", true},
    {InputKind::Image, "image", "Image", false},
    {InputKind::MediaFile, "media", "Media File", true},
    {InputKind::Title, "title", "Title", false},
    {InputKind::Color, "color", "Colour", false},
    {InputKind::Scene, "scene", "Scene", false},
    {InputKind::DeckLink, "decklink", "DeckLink", true},
    {InputKind::Ndi, "ndi", "NDI", true},
};

const KindName* findKind(InputKind kind)
{
    for (const auto& entry : kKinds) {
        if (entry.kind == kind) {
            return &entry;
        }
    }
    return nullptr;
}

const KindName* findKey(const std::string& key)
{
    for (const auto& entry : kKinds) {
        if (key == entry.key) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

const char* inputKindKey(InputKind kind)
{
    const auto* entry = findKind(kind);
    return entry ? entry->key : "unknown";
}

InputKind inputKindFromKey(const std::string& key)
{
    const auto* entry = findKey(key);
    return entry ? entry->kind : InputKind::Unknown;
}

const char* inputKindLabel(InputKind kind)
{
    const auto* entry = findKind(kind);
    return entry ? entry->label : "Unknown";
}

bool inputKindHasAudio(InputKind kind)
{
    const auto* entry = findKind(kind);
    return entry ? entry->audio : false;
}

const char* transitionKindKey(TransitionKind kind)
{
    switch (kind) {
    case TransitionKind::Cut:
        return "cut";
    case TransitionKind::Fade:
        return "fade";
    case TransitionKind::Wipe:
        return "wipe";
    case TransitionKind::Slide:
        return "slide";
    case TransitionKind::Zoom:
        return "zoom";
    case TransitionKind::Fly:
        return "fly";
    case TransitionKind::Cube:
        return "cube";
    }
    return "fade";
}

TransitionKind transitionKindFromKey(const std::string& key)
{
    if (key == "cut") {
        return TransitionKind::Cut;
    }
    if (key == "wipe") {
        return TransitionKind::Wipe;
    }
    if (key == "slide") {
        return TransitionKind::Slide;
    }
    if (key == "zoom") {
        return TransitionKind::Zoom;
    }
    if (key == "fly") {
        return TransitionKind::Fly;
    }
    if (key == "cube") {
        return TransitionKind::Cube;
    }
    return TransitionKind::Fade;
}

const char* transitionKindLabel(TransitionKind kind)
{
    switch (kind) {
    case TransitionKind::Cut:
        return "Cut";
    case TransitionKind::Fade:
        return "Fade";
    case TransitionKind::Wipe:
        return "Wipe";
    case TransitionKind::Slide:
        return "Slide";
    case TransitionKind::Zoom:
        return "Zoom";
    case TransitionKind::Fly:
        return "Fly";
    case TransitionKind::Cube:
        return "Cube";
    }
    return "Fade";
}

} // namespace mixvera::core