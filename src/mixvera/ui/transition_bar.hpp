#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QFrame>

class QPushButton;
class QSlider;
class QSpinBox;
class QLabel;

namespace mixvera::ui {

/// The vMix style transition cluster: CUT / AUTO, type buttons, FTB and a
/// vertical T-Bar between the preview and program monitors. It only reports
/// intent; the main window owns the state.
class TransitionBar final : public QFrame {
    Q_OBJECT

public:
    explicit TransitionBar(QWidget* parent = nullptr);

    void setKind(core::TransitionKind kind);
    void setDurationMs(int ms);
    void setPosition(float position);
    void setAutoRunning(bool running);
    void setFtb(bool engaged);
    void setLiveTransitionName(const QString& name);

signals:
    void cutRequested();
    void autoRequested();
    void kindChanged(core::TransitionKind kind);
    void durationChanged(int ms);
    void positionChanged(float position);
    void positionReleased();
    void ftbToggled(bool engaged);

private:
    QPushButton* typeButton(core::TransitionKind kind, const QString& text);
    void syncTypeButtons();

    QPushButton* fade_ = nullptr;
    QPushButton* wipe_ = nullptr;
    QPushButton* slide_ = nullptr;
    QPushButton* zoom_ = nullptr;
    QPushButton* fly_ = nullptr;
    QPushButton* cube_ = nullptr;
    QPushButton* cut_ = nullptr;
    QPushButton* auto_ = nullptr;
    QPushButton* ftb_ = nullptr;
    QSpinBox* duration_ = nullptr;
    QSlider* tbar_ = nullptr;
    QLabel* mix_ = nullptr;
    QLabel* liveName_ = nullptr;
    core::TransitionKind kind_ = core::TransitionKind::Fade;
};

} // namespace mixvera::ui