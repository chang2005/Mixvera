#include "mixvera/ui/mixer_panel.hpp"
#include "mixvera/core/application.hpp"
#include "mixvera/services/input_service.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

namespace mixvera::ui {

MixerPanel::MixerPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    const auto& inputs = core::Application::instance().services().get<services::InputService>().inputs();
    for (const auto& input : inputs) {
        auto* strip = new QWidget(this);
        auto* stripLayout = new QVBoxLayout(strip);
        auto* label = new QLabel(QString::fromStdString(input.name), strip);
        auto* fader = new QSlider(Qt::Vertical, strip);
        label->setAlignment(Qt::AlignCenter);
        fader->setRange(-60, 12);
        fader->setValue(0);
        stripLayout->addWidget(fader);
        stripLayout->addWidget(label);
        layout->addWidget(strip);
    }
}

} // namespace mixvera::ui
