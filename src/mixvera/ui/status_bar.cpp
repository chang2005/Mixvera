#include "mixvera/ui/status_bar.hpp"

#include <QLabel>
#include <QStyle>

namespace mixvera::ui {

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
{
    engine_ = new QLabel(tr("ENGINE —"), this);
    canvas_ = new QLabel(tr("CANVAS —"), this);
    outputs_ = new QLabel(tr("OUTPUTS 0"), this);
    project_ = new QLabel(tr("PROJECT —"), this);
    onAir_ = new QLabel(tr("STANDBY"), this);
    onAir_->setObjectName("statusOnAir");
    clock_ = new QLabel(this);

    addWidget(engine_);
    addWidget(new QLabel(QStringLiteral("|"), this));
    addWidget(canvas_);
    addWidget(new QLabel(QStringLiteral("|"), this));
    addWidget(outputs_);
    addPermanentWidget(project_);
    addPermanentWidget(onAir_);
    addPermanentWidget(clock_);
}

void StatusBar::setEngineText(const QString& text)
{
    engine_->setText(text);
}

void StatusBar::setCanvasText(const QString& text)
{
    canvas_->setText(text);
}

void StatusBar::setOutputText(const QString& text)
{
    outputs_->setText(text);
}

void StatusBar::setProjectText(const QString& text)
{
    project_->setText(text);
}

void StatusBar::setOnAir(bool live)
{
    onAir_->setText(live ? tr("ON AIR") : tr("STANDBY"));
    if (onAir_->property("live").toBool() != live) {
        onAir_->setProperty("live", live);
        onAir_->style()->unpolish(onAir_);
        onAir_->style()->polish(onAir_);
    }
}

void StatusBar::setClock(const QString& text)
{
    clock_->setText(text);
}

} // namespace mixvera::ui