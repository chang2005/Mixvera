#include "mixvera/ui/program_view.hpp"

namespace mixvera::ui {

ProgramView::ProgramView(QWidget* parent)
    : QLabel("PROGRAM\n\nlibobs output surface", parent)
{
    setObjectName("programView");
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background:#171a1f;color:#e74c3c;font:bold 20px;");
}

} // namespace mixvera::ui
