#include "mixvera/ui/status_bar.hpp"
#include <QLabel>

namespace mixvera::ui {

class StatusBar::impl_t {
public:
    QLabel* engine = nullptr;
    QLabel* transition = nullptr;
};

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
    , impl_(std::make_unique<impl_t>())
{
    impl_->engine = new QLabel("Engine: Stub", this);
    impl_->transition = new QLabel("T-Bar: 0.000", this);
    addWidget(impl_->engine);
    addPermanentWidget(impl_->transition);
}

void StatusBar::setTransition(float position)
{
    impl_->transition->setText(QString("T-Bar: %1").arg(position, 0, 'f', 3));
}

} // namespace mixvera::ui
