#include "mixvera/core/project.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace mixvera::core {

namespace {

Json sceneItemToJson(const SceneItem& item)
{
    return Json{{"input", item.inputId}, {"x", item.x}, {"y", item.y},
        {"width", item.width}, {"height", item.height},
        {"opacity", item.opacity}, {"visible", item.visible}};
}

SceneItem sceneItemFromJson(const Json& value)
{
    SceneItem item;
    item.inputId = value.value("input", std::string());
    item.x = value.value("x", 0.0F);
    item.y = value.value("y", 0.0F);
    item.width = value.value("width", 1.0F);
    item.height = value.value("height", 1.0F);
    item.opacity = value.value("opacity", 1.0F);
    item.visible = value.value("visible", true);
    return item;
}

Json inputToJson(const InputDescriptor& input)
{
    Json params = Json::object();
    for (const auto& [key, value] : input.params) {
        params[key] = value;
    }

    Json items = Json::array();
    for (const auto& item : input.items) {
        items.push_back(sceneItemToJson(item));
    }

    return Json{{"id", input.id}, {"name", input.name},
        {"kind", inputKindKey(input.kind)}, {"active", input.active},
        {"params", params}, {"items", items},
        {"hasAudio", input.hasAudio}, {"volumeDb", input.volumeDb},
        {"muted", input.muted}};
}

InputDescriptor inputFromJson(const Json& value)
{
    InputDescriptor input;
    input.id = value.value("id", std::string());
    input.name = value.value("name", std::string());
    input.kind = inputKindFromKey(value.value("kind", std::string("unknown")));
    input.active = value.value("active", true);
    if (value.contains("params") && value["params"].is_object()) {
        for (auto it = value["params"].begin(); it != value["params"].end(); ++it) {
            input.params[it.key()] = it.value().is_string() ? it.value().get<std::string>()
                                                            : it.value().dump();
        }
    }
    if (value.contains("items") && value["items"].is_array()) {
        for (const auto& item : value["items"]) {
            input.items.push_back(sceneItemFromJson(item));
        }
    }
    input.hasAudio = value.value("hasAudio", inputKindHasAudio(input.kind));
    input.volumeDb = value.value("volumeDb", 0.0F);
    input.muted = value.value("muted", false);
    return input;
}

InputDescriptor makeScene(const std::string& id, const std::string& name)
{
    InputDescriptor scene;
    scene.id = id;
    scene.name = name;
    scene.kind = InputKind::Scene;
    return scene;
}

} // namespace

const InputDescriptor* findInput(const std::vector<InputDescriptor>& inputs,
    const std::string& id)
{
    const auto it = std::find_if(inputs.begin(), inputs.end(),
        [&](const auto& input) { return input.id == id; });
    return it == inputs.end() ? nullptr : &*it;
}

Project makeDefaultProject()
{
    Project project;
    project.name = "Welcome";
    project.outputs.recordingDirectory = "recordings";
    project.transitionKind = TransitionKind::Fade;
    project.transitionDurationMs = 500;

    InputDescriptor pattern;
    pattern.id = "camera-1";
    pattern.name = "Camera 1";
    pattern.kind = InputKind::TestPattern;
    pattern.params["pattern"] = "bars";

    InputDescriptor colour;
    colour.id = "camera-2";
    colour.name = "Camera 2";
    colour.kind = InputKind::Color;
    colour.params["color"] = "#2743a6";

    InputDescriptor title;
    title.id = "title-1";
    title.name = "Mixvera Title";
    title.kind = InputKind::Title;
    title.params["text"] = "MIXVERA\nLIVE PRODUCTION";
    title.params["color"] = "#ffffff";
    title.params["size"] = "64";
    title.params["align"] = "center";

    InputDescriptor sceneOne = makeScene("scene-1", "Scene 1");
    sceneOne.items.push_back({"camera-1", 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, true});
    sceneOne.items.push_back({"title-1", 0.06F, 0.70F, 0.70F, 0.22F, 1.0F, true});

    InputDescriptor sceneTwo = makeScene("scene-2", "Scene 2");
    sceneTwo.items.push_back({"camera-2", 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, true});
    sceneTwo.items.push_back({"title-1", 0.06F, 0.70F, 0.70F, 0.22F, 0.65F, true});

    project.inputs = {pattern, colour, title, sceneOne, sceneTwo};
    project.programId = "scene-1";
    project.previewId = "scene-2";
    return project;
}

Json projectToJson(const Project& project)
{
    Json inputs = Json::array();
    for (const auto& input : project.inputs) {
        inputs.push_back(inputToJson(input));
    }

    return Json{{"schema", "mixvera.project"}, {"version", 1},
        {"name", project.name},
        {"canvas", Json{{"width", project.canvasWidth}, {"height", project.canvasHeight},
                        {"fps", project.fps}}},
        {"inputs", inputs},
        {"program", project.programId},
        {"preview", project.previewId},
        {"transition",
            Json{{"kind", transitionKindKey(project.transitionKind)},
                {"durationMs", project.transitionDurationMs}}},
        {"outputs",
            Json{{"recordingDirectory", project.outputs.recordingDirectory},
                {"streamUrl", project.outputs.streamUrl},
                {"streamKey", project.outputs.streamKey}}}};
}

Project projectFromJson(const Json& value)
{
    Project project;
    project.name = value.value("name", std::string("Untitled"));

    if (value.contains("canvas") && value["canvas"].is_object()) {
        const auto& canvas = value["canvas"];
        project.canvasWidth = std::clamp(canvas.value("width", 1920), 320, 7680);
        project.canvasHeight = std::clamp(canvas.value("height", 1080), 180, 4320);
        project.fps = std::clamp(canvas.value("fps", 30), 1, 120);
    }

    if (value.contains("inputs") && value["inputs"].is_array()) {
        for (const auto& entry : value["inputs"]) {
            project.inputs.push_back(inputFromJson(entry));
        }
    }

    project.programId = value.value("program", std::string());
    project.previewId = value.value("preview", std::string());

    if (value.contains("transition") && value["transition"].is_object()) {
        const auto& transition = value["transition"];
        project.transitionKind = transitionKindFromKey(
            transition.value("kind", std::string("fade")));
        project.transitionDurationMs = transition.value("durationMs", 500U);
    }

    if (value.contains("outputs") && value["outputs"].is_object()) {
        const auto& outputs = value["outputs"];
        project.outputs.recordingDirectory = outputs.value(
            "recordingDirectory", std::string("recordings"));
        project.outputs.streamUrl = outputs.value("streamUrl", std::string());
        project.outputs.streamKey = outputs.value("streamKey", std::string());
    }

    if (project.inputs.empty()) {
        return makeDefaultProject();
    }

    const InputDescriptor* program = findInput(project.inputs, project.programId);
    if (!program) {
        project.programId = project.inputs.front().id;
    }
    const InputDescriptor* preview = findInput(project.inputs, project.previewId);
    if (!preview) {
        project.previewId = project.inputs.size() > 1 ? project.inputs[1].id
                                                      : project.programId;
    }
    return project;
}

bool saveProjectFile(const Project& project, const std::string& path)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return false;
    }
    stream << projectToJson(project).dump(2);
    return static_cast<bool>(stream);
}

bool loadProjectFile(Project& project, const std::string& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    try {
        project = projectFromJson(parseJson(buffer.str()));
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

} // namespace mixvera::core