#pragma once

#include "mixvera/ui/video_surface.hpp"

namespace mixvera::ui {

class PreviewView final : public VideoSurface {
public:
    explicit PreviewView(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
