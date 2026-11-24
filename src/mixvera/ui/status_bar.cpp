#include "mixvera/ui/status_bar.hpp"

#include <QLabel>

namespace mixvera::ui {

class StatusBar::impl_t {
public:
    QLabel* engine = nullptr;
    QLabel* outputs = nullptr;
    QLabel* transition = nullptr;
};

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
    , impl_(std::make_unique<impl_t>())
{
    impl_->engine = new QLabel("●  SERVICES ONLINE  ·  STUB ENGINE", this);
    impl_->outputs = new QLabel("OUTPUTS 0 / 3", this);
    impl_->transition = new QLabel("T-BAR 0%", this);
    addWidget(impl_->engine);
    addWidget(impl_->outputs);
    addPermanentWidget(impl_->transition);
    setSizeGripEnabled(false);
}

StatusBar::~StatusBar() = default;

void StatusBar::setTransition(float position)
{
    impl_->transition->setText(QString("T-BAR %1%").arg(
        static_cast<int>(position * 100.0F + 0.5F)));
}

void StatusBar::setOutputsActive(int active, int total)
{
    impl_->outputs->setText(QString("OUTPUTS %1 / %2").arg(active).arg(total));
}

} // namespace mixvera::ui
