#include "mixvera/ui/output_panel.hpp"

#include "mixvera/core/application.hpp"
#include "mixvera/services/output_service.hpp"
#include "mixvera/services/replay_service.hpp"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace mixvera::ui {

namespace {

void repolish(QWidget* widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace

OutputPanel::OutputPanel(QWidget* parent)
    : QWidget(parent)
{
    auto& app = core::Application::instance();
    outputs_ = &app.services().get<services::OutputService>();
    replay_ = &app.services().get<services::ReplayService>();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(7);

    for (const auto& output : outputs_->outputs()) {
        auto* row = new QFrame(this);
        row->setObjectName("outputRow");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(9, 7, 8, 7);
        rowLayout->setSpacing(8);

        auto* dot = new QLabel(row);
        dot->setObjectName("statusDot");
        dot->setFixedSize(8, 8);

        auto* names = new QVBoxLayout;
        names->setSpacing(0);
        auto* name = new QLabel(QString::fromStdString(output.name), row);
        name->setObjectName("outputName");
        auto* id = new QLabel(QString::fromStdString(output.id).toUpper(), row);
        id->setObjectName("outputId");
        names->addWidget(name);
        names->addWidget(id);

        auto* state = new QLabel("IDLE", row);
        state->setObjectName("outputState");
        state->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* button = new QPushButton("START", row);
        button->setObjectName("outputButton");
        button->setCursor(Qt::PointingHandCursor);

        rowLayout->addWidget(dot);
        rowLayout->addLayout(names, 1);
        rowLayout->addWidget(state);
        rowLayout->addWidget(button);
        layout->addWidget(row);

        const std::string outputId = output.id;
        QObject::connect(button, &QPushButton::clicked, this, [this, outputId] {
            for (const auto& current : outputs_->outputs()) {
                if (current.id != outputId) {
                    continue;
                }
                if (current.running) {
                    outputs_->stop(outputId);
                } else {
                    outputs_->start(outputId);
                }
                break;
            }
            refresh();
        });

        rows_.push_back({output.id, dot, state, button});
    }

    auto* divider = new QFrame(this);
    divider->setObjectName("divider");
    divider->setFixedHeight(1);
    layout->addWidget(divider);

    auto* replayHeader = new QHBoxLayout;
    auto* replayTitle = new QLabel("REPLAY BUFFER", this);
    replayTitle->setObjectName("cardTitle");
    replayState_ = new QLabel("IDLE", this);
    replayState_->setObjectName("replayState");
    replayHeader->addWidget(replayTitle);
    replayHeader->addStretch();
    replayHeader->addWidget(replayState_);
    layout->addLayout(replayHeader);

    auto* controls = new QHBoxLayout;
    controls->setSpacing(6);

    recordButton_ = new QPushButton("REC", this);
    recordButton_->setObjectName("recButton");
    recordButton_->setCheckable(true);
    recordButton_->setCursor(Qt::PointingHandCursor);

    saveButton_ = new QPushButton("SAVE CLIP", this);
    saveButton_->setObjectName("saveReplayButton");
    saveButton_->setCursor(Qt::PointingHandCursor);

    speedBox_ = new QComboBox(this);
    speedBox_->addItem("100%", 100);
    speedBox_->addItem("50%", 50);
    speedBox_->addItem("25%", 25);

    controls->addWidget(recordButton_);
    controls->addWidget(saveButton_);
    controls->addWidget(speedBox_, 1);
    layout->addLayout(controls);

    QObject::connect(recordButton_, &QPushButton::toggled, this, [this](bool checked) {
        replay_->setRecording(checked);
        refresh();
    });
    QObject::connect(saveButton_, &QPushButton::clicked, this, [this] {
        replay_->saveReplay();
        replayState_->setText("CLIP SAVED");
        QTimer::singleShot(1600, this, [this] { refresh(); });
    });
    QObject::connect(speedBox_, &QComboBox::currentIndexChanged, this, [this](int index) {
        replay_->setSpeedPercent(static_cast<std::uint32_t>(speedBox_->itemData(index).toInt()));
        refresh();
    });

    refresh();
}

void OutputPanel::refresh()
{
    for (const auto& row : rows_) {
        for (const auto& output : outputs_->outputs()) {
            if (output.id != row.id) {
                continue;
            }
            row.dot->setProperty("running", output.running);
            repolish(row.dot);
            row.state->setText(output.running ? "LIVE" : "IDLE");
            row.state->setStyleSheet(output.running ? "color:#38d996;" : "color:#6e8297;");
            row.button->setText(output.running ? "STOP" : "START");
            row.button->setProperty("running", output.running);
            repolish(row.button);
            break;
        }
    }

    const auto& replay = replay_->state();
    {
        const QSignalBlocker blocker(recordButton_);
        recordButton_->setChecked(replay.recording);
    }
    recordButton_->setText(replay.recording ? "STOP" : "REC");
    replayState_->setText(replay.recording
            ? QString("RECORDING · %1S").arg(replay.seconds)
            : QString("IDLE · %1S BUFFER").arg(replay.seconds));

    int speedIndex = 0;
    for (int index = 0; index < speedBox_->count(); ++index) {
        if (speedBox_->itemData(index).toInt() == static_cast<int>(replay.speedPercent)) {
            speedIndex = index;
            break;
        }
    }
    const QSignalBlocker speedBlocker(speedBox_);
    speedBox_->setCurrentIndex(speedIndex);
}

} // namespace mixvera::ui
