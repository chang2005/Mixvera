#include "mixvera/ui/transition_bar.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cmath>

namespace mixvera::ui {

namespace {

QLabel* label(const QString& text, const char* objectName, QWidget* parent)
{
    auto* widget = new QLabel(text, parent);
    widget->setObjectName(QString::fromLatin1(objectName));
    return widget;
}

} // namespace

TransitionBar::TransitionBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("transitionBar");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 7, 10, 8);
    root->setSpacing(6);

    auto* row = new QHBoxLayout;
    row->setSpacing(6);

    fade_ = typeButton(core::TransitionKind::Fade, tr("Fade"));
    wipe_ = typeButton(core::TransitionKind::Wipe, tr("Wipe"));
    slide_ = typeButton(core::TransitionKind::Slide, tr("Slide"));
    zoom_ = typeButton(core::TransitionKind::Zoom, tr("Zoom"));
    row->addWidget(fade_);
    row->addWidget(wipe_);
    row->addWidget(slide_);
    row->addWidget(zoom_);
    row->addSpacing(4);

    cut_ = new QPushButton(tr("CUT"), this);
    cut_->setObjectName("cutButton");
    cut_->setToolTip(tr("Cut to preview (Space)"));
    cut_->setCursor(Qt::PointingHandCursor);
    row->addWidget(cut_);

    auto_ = new QPushButton(tr("AUTO"), this);
    auto_->setObjectName("autoButton");
    auto_->setToolTip(tr("Run the selected transition (Enter)"));
    auto_->setCursor(Qt::PointingHandCursor);
    row->addWidget(auto_);

    duration_ = new QSpinBox(this);
    duration_->setRange(100, 5000);
    duration_->setSingleStep(50);
    duration_->setValue(500);
    duration_->setSuffix(" ms");
    duration_->setFixedWidth(88);
    row->addWidget(duration_);

    ftb_ = new QPushButton(tr("FTB"), this);
    ftb_->setObjectName("ftbButton");
    ftb_->setCheckable(true);
    ftb_->setToolTip(tr("Fade to black (program out)"));
    row->addWidget(ftb_);

    liveName_ = label("", "controlLabel", this);
    row->addStretch();
    row->addWidget(liveName_);
    mix_ = label("MIX 000%", "mixReadout", this);
    mix_->setMinimumWidth(86);
    mix_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(mix_);
    root->addLayout(row);

    auto* tbarRow = new QHBoxLayout;
    tbarRow->setSpacing(8);
    tbarRow->addWidget(label(tr("T-BAR"), "controlLabel", this));
    tbar_ = new QSlider(Qt::Horizontal, this);
    tbar_->setObjectName("tbar");
    tbar_->setRange(0, 1023);
    tbar_->setValue(0);
    tbar_->setToolTip(tr("Drag past 50% to complete the transition"));
    tbarRow->addWidget(tbar_, 1);
    root->addLayout(tbarRow);

    connect(cut_, &QPushButton::clicked, this, &TransitionBar::cutRequested);
    connect(auto_, &QPushButton::clicked, this, &TransitionBar::autoRequested);
    connect(ftb_, &QPushButton::toggled, this, &TransitionBar::ftbToggled);
    connect(duration_, &QSpinBox::valueChanged, this, &TransitionBar::durationChanged);

    for (QPushButton* button : {fade_, wipe_, slide_, zoom_}) {
        connect(button, &QPushButton::clicked, this, [this, button] {
            kind_ = static_cast<core::TransitionKind>(button->property("transitionKind").toInt());
            syncTypeButtons();
            emit kindChanged(kind_);
        });
    }

    connect(tbar_, &QSlider::valueChanged, this, [this](int value) {
        emit positionChanged(static_cast<float>(value) / 1023.0F);
    });
    connect(tbar_, &QSlider::sliderReleased, this, &TransitionBar::positionReleased);

    syncTypeButtons();
}

QPushButton* TransitionBar::typeButton(core::TransitionKind kind, const QString& text)
{
    auto* button = new QPushButton(text, this);
    button->setObjectName("transitionType");
    button->setCheckable(true);
    button->setProperty("transitionKind", static_cast<int>(kind));
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void TransitionBar::syncTypeButtons()
{
    fade_->setChecked(kind_ == core::TransitionKind::Fade);
    wipe_->setChecked(kind_ == core::TransitionKind::Wipe);
    slide_->setChecked(kind_ == core::TransitionKind::Slide);
    zoom_->setChecked(kind_ == core::TransitionKind::Zoom);
}

void TransitionBar::setKind(core::TransitionKind kind)
{
    kind_ = kind;
    syncTypeButtons();
}

void TransitionBar::setDurationMs(int ms)
{
    duration_->setValue(ms);
}

void TransitionBar::setPosition(float position)
{
    const int value = static_cast<int>(std::lround(
        std::min(std::max(position, 0.0F), 1.0F) * 1023.0F));
    tbar_->blockSignals(true);
    tbar_->setValue(value);
    tbar_->blockSignals(false);
    mix_->setText(QString("MIX %1%").arg(static_cast<int>(std::lround(position * 100.0F)), 3, 10,
        QLatin1Char('0')));
}

void TransitionBar::setAutoRunning(bool running)
{
    auto_->setEnabled(!running);
    auto_->setText(running ? tr("RUNNING") : tr("AUTO"));
}

void TransitionBar::setFtb(bool engaged)
{
    ftb_->blockSignals(true);
    ftb_->setChecked(engaged);
    ftb_->blockSignals(false);
}

void TransitionBar::setLiveTransitionName(const QString& name)
{
    liveName_->setText(name.toUpper());
}

} // namespace mixvera::ui