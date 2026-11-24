#include "mixvera/ui/multiview_panel.hpp"
#include "mixvera/core/application.hpp"
#include "mixvera/services/input_service.hpp"

#include <QGridLayout>
#include <QLabel>

namespace mixvera::ui {

MultiviewPanel::MultiviewPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QGridLayout(this);
    const auto& inputs = core::Application::instance().services().get<services::InputService>().inputs();
    int row = 0;
    int col = 0;
    for (const auto& input : inputs) {
        auto* tile = new QLabel(QString::fromStdString(input.name), this);
        tile->setAlignment(Qt::AlignCenter);
        tile->setStyleSheet("background:#101317;color:#b8c0cc;border:1px solid #2c3440;");
        tile->setMinimumSize(160, 90);
        layout->addWidget(tile, row, col);
        if (++col == 2) {
            col = 0;
            ++row;
        }
    }
}

} // namespace mixvera::ui
