#include "mixvera/core/project.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace mixvera;

static void defaultProjectIsUsable()
{
    const core::Project project = core::makeDefaultProject();
    assert(!project.inputs.empty());
    assert(core::findInput(project.inputs, project.programId) != nullptr);
    assert(core::findInput(project.inputs, project.previewId) != nullptr);
    assert(project.canvasWidth >= 640);
    assert(project.fps >= 24);

    int scenes = 0;
    for (const auto& input : project.inputs) {
        if (input.isScene()) {
            ++scenes;
            assert(!input.items.empty());
        }
    }
    assert(scenes >= 2);
}

static void roundTripsThroughJson()
{
    core::Project project = core::makeDefaultProject();
    project.name = "Round Trip";
    project.canvasWidth = 1280;
    project.canvasHeight = 720;
    project.fps = 50;
    project.transitionKind = core::TransitionKind::Wipe;
    project.transitionDurationMs = 750;
    project.outputs.recordingDirectory = "D:/captures";
    project.outputs.streamUrl = "rtmp://example.test/live";
    project.outputs.streamKey = "secret";

    const std::string text = core::projectToJson(project).dump();
    const core::Project restored = core::projectFromJson(core::parseJson(text));

    assert(restored.name == project.name);
    assert(restored.canvasWidth == 1280);
    assert(restored.canvasHeight == 720);
    assert(restored.fps == 50);
    assert(restored.transitionKind == core::TransitionKind::Wipe);
    assert(restored.transitionDurationMs == 750);
    assert(restored.inputs.size() == project.inputs.size());
    assert(restored.programId == project.programId);
    assert(restored.previewId == project.previewId);
    assert(restored.outputs.streamUrl == project.outputs.streamUrl);

    for (std::size_t index = 0; index < project.inputs.size(); ++index) {
        assert(restored.inputs[index].id == project.inputs[index].id);
        assert(restored.inputs[index].kind == project.inputs[index].kind);
        assert(restored.inputs[index].params == project.inputs[index].params);
        assert(restored.inputs[index].items.size() == project.inputs[index].items.size());
    }
}

static void savesAndLoadsFromDisk()
{
    const auto path = std::filesystem::temp_directory_path() / "mixvera-project-test.json";
    core::Project project = core::makeDefaultProject();
    project.name = "Disk";
    assert(core::saveProjectFile(project, path.string()));

    core::Project loaded;
    assert(core::loadProjectFile(loaded, path.string()));
    assert(loaded.name == "Disk");
    assert(loaded.inputs.size() == project.inputs.size());

    std::filesystem::remove(path);
}

static void kindKeysRoundTrip()
{
    for (auto kind : {core::InputKind::ScreenCapture, core::InputKind::WindowCapture,
             core::InputKind::Camera, core::InputKind::Image, core::InputKind::MediaFile,
             core::InputKind::Title, core::InputKind::Color, core::InputKind::Scene,
             core::InputKind::TestPattern}) {
        const std::string key = core::inputKindKey(kind);
        assert(core::inputKindFromKey(key) == kind);
    }
    assert(core::inputKindFromKey("nonsense") == core::InputKind::Unknown);

    for (auto kind : {core::TransitionKind::Cut, core::TransitionKind::Fade,
             core::TransitionKind::Wipe, core::TransitionKind::Slide,
             core::TransitionKind::Zoom, core::TransitionKind::Fly,
             core::TransitionKind::Cube}) {
        assert(core::transitionKindFromKey(core::transitionKindKey(kind)) == kind);
    }
    assert(core::transitionKindKey(core::TransitionKind::Fly) == "fly");
    assert(core::transitionKindKey(core::TransitionKind::Cube) == "cube");
}

static void overlayLayersRoundTrip()
{
    core::Project project = core::makeDefaultProject();
    assert(project.overlays.size() == 4);
    for (int index = 0; index < 4; ++index) {
        assert(project.overlays[static_cast<std::size_t>(index)].layer == index + 1);
        assert(!project.overlays[static_cast<std::size_t>(index)].enabled);
    }

    project.overlays[0].inputId = "title-1";
    project.overlays[0].enabled = true;
    project.overlays[2].inputId = "missing-input";
    project.overlays[2].enabled = true;

    const core::Project restored = core::projectFromJson(core::parseJson(
        core::projectToJson(project).dump()));
    assert(restored.overlays.size() == 4);
    assert(restored.overlays[0].layer == 1);
    assert(restored.overlays[0].inputId == "title-1");
    assert(restored.overlays[0].enabled);
    assert(restored.overlays[2].layer == 3);
    // Unknown ids cannot be composited, so loading must clear them.
    assert(restored.overlays[2].inputId.empty());
    assert(!restored.overlays[2].enabled);
    assert(!restored.overlays[1].enabled);
}

static void sceneLayerGeometrySurvives()
{
    core::Project project = core::makeDefaultProject();
    for (auto& input : project.inputs) {
        if (!input.isScene()) {
            continue;
        }
        input.items.front().x = 0.25F;
        input.items.front().width = 0.5F;
    }
    const core::Project restored = core::projectFromJson(core::parseJson(
        core::projectToJson(project).dump()));

    for (const auto& input : restored.inputs) {
        if (!input.isScene()) {
            continue;
        }
        assert(input.items.front().x > 0.24F && input.items.front().x < 0.26F);
        assert(input.items.front().width > 0.49F && input.items.front().width < 0.51F);
    }
}

int main()
{
    defaultProjectIsUsable();
    roundTripsThroughJson();
    savesAndLoadsFromDisk();
    kindKeysRoundTrip();
    overlayLayersRoundTrip();
    sceneLayerGeometrySurvives();
    std::cout << "project tests passed\n";
    return 0;
}