#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace mixvera::core {

enum class InputKind {
    Unknown,
    TestPattern,
    ScreenCapture,
    WindowCapture,
    Camera,
    Image,
    MediaFile,
    Title,
    Color,
    Scene,
    DeckLink,
    Ndi,
};

enum class TransitionKind {
    Cut,
    Fade,
    Wipe,
    Slide,
    Zoom,
    Fly,
    Cube,
};

/// One downstream overlay channel. Overlays sit above the program bus (after
/// the transition, before FTB) and survive scene changes until they are turned
/// off, following the vMix / SinSam model.
struct OverlayLayer {
    int layer = 1; ///< 1..4, 4 is on top
    std::string inputId;
    bool enabled = false;
};

/// One layer inside a scene. Geometry is normalised to the canvas (0..1) so a
/// project stays valid when the canvas resolution changes.
struct SceneItem {
    std::string inputId;
    float x = 0.0F;
    float y = 0.0F;
    float width = 1.0F;
    float height = 1.0F;
    float opacity = 1.0F;
    bool visible = true;
};

struct InputDescriptor {
    std::string id;
    std::string name;
    InputKind kind = InputKind::Unknown;
    bool active = true;

    /// Kind specific capture settings, e.g. monitor index, window handle,
    /// camera device id, file path, title text or colour value.
    std::map<std::string, std::string> params;

    /// Back to front layers; only used when kind == InputKind::Scene.
    std::vector<SceneItem> items;

    bool hasAudio = false;
    float volumeDb = 0.0F;
    bool muted = false;

    bool isScene() const { return kind == InputKind::Scene; }
};

const char* inputKindKey(InputKind kind);
InputKind inputKindFromKey(const std::string& key);
const char* inputKindLabel(InputKind kind);
bool inputKindHasAudio(InputKind kind);

const char* transitionKindKey(TransitionKind kind);
TransitionKind transitionKindFromKey(const std::string& key);
const char* transitionKindLabel(TransitionKind kind);

struct TransitionState {
    TransitionKind kind = TransitionKind::Fade;
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
    /// Monitoring-only state: soloed channels stay audible while the rest of
    /// the mix is dimmed. Not persisted in the project file.
    bool solo = false;
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