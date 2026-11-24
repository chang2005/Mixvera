#include "mixvera/ui/tbar_widget.hpp"

namespace mixvera::ui {

TBarWidget::TBarWidget(QWidget* parent)
    : QSlider(Qt::Horizontal, parent)
{
    setRange(0, 1023);
    setValue(0);
    setSingleStep(8);
    setPageStep(64);
    setToolTip("Manual transition T-Bar");
    setObjectName("tbar");
    connect(this, &QSlider::valueChanged, this, &TBarWidget::positionChanged);
    connect(this, &QSlider::sliderReleased, this, &TBarWidget::released);
}

} // namespace mixvera::ui
