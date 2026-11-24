#include "mixvera/ui/program_view.hpp"

namespace mixvera::ui {

ProgramView::ProgramView(QWidget* parent)
    : VideoSurface(parent)
{
    setObjectName("programView");
    setRole(Role::Program);
}

} // namespace mixvera::ui
