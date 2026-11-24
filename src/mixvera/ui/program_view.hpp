#pragma once

#include "mixvera/ui/video_surface.hpp"

namespace mixvera::ui {

class ProgramView final : public VideoSurface {
public:
    explicit ProgramView(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
