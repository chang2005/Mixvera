#include "mixvera/ui/preview_view.hpp"

namespace mixvera::ui {

PreviewView::PreviewView(QWidget* parent)
    : QLabel("PREVIEW\n\nlibobs preview surface", parent)
{
    setObjectName("previewView");
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background:#171a1f;color:#2ecc71;font:bold 18px;");
}

} // namespace mixvera::ui
