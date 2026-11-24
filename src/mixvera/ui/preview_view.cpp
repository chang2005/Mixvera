#include "mixvera/ui/preview_view.hpp"

namespace mixvera::ui {

PreviewView::PreviewView(QWidget* parent)
    : VideoSurface(parent)
{
    setObjectName("previewView");
    setRole(Role::Preview);
}

} // namespace mixvera::ui
