#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/core/json_document.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mixvera::core {

struct ProjectOutputs {
    std::string recordingDirectory;
    std::string streamUrl;
    std::string streamKey;
};

/// Everything that has to survive a restart: sources, scene layouts, the
/// on-air pair, transition settings and output destinations.
struct Project {
    std::string name = "Untitled";
    int canvasWidth = 1920;
    int canvasHeight = 1080;
    int fps = 30;

    std::vector<InputDescriptor> inputs;
    std::string programId;
    std::string previewId;

    TransitionKind transitionKind = TransitionKind::Fade;
    std::uint32_t transitionDurationMs = 500;

    ProjectOutputs outputs;
};

Project makeDefaultProject();

Json projectToJson(const Project& project);
Project projectFromJson(const Json& value);

bool saveProjectFile(const Project& project, const std::string& path);
bool loadProjectFile(Project& project, const std::string& path);

const InputDescriptor* findInput(const std::vector<InputDescriptor>& inputs,
    const std::string& id);

} // namespace mixvera::core