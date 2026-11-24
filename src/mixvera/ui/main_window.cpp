#include "mixvera/ui/main_window.hpp"

#include "mixvera/core/application.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/output_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/services/transition_service.hpp"
#include "mixvera/ui/mixer_panel.hpp"
#include "mixvera/ui/multiview_panel.hpp"
#include "mixvera/ui/output_panel.hpp"
#include "mixvera/ui/program_view.hpp"
#include "mixvera/ui/preview_view.hpp"
#include "mixvera/ui/status_bar.hpp"
#include "mixvera/ui/tbar_widget.hpp"
#include "mixvera/ui/theme.hpp"
#include "mixvera/ui/video_surface.hpp"

#include <QComboBox>
#include <QElapsedTimer>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyle>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace mixvera::ui {

namespace {

QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

QFrame* makeDivider(QWidget* parent, bool vertical)
{
    auto* divider = new QFrame(parent);
    divider->setObjectName("divider");
    if (vertical) {
        divider->setFixedSize(1, 22);
    } else {
        divider->setFixedHeight(1);
    }
    return divider;
}

QFrame* makeMonitorCard(VideoSurface* surface, const QString& title, const QString& accent,
    QLabel** sourceOut, QLabel** statusOut, QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName("monitorCard");
    card->setProperty("accent", accent);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 9);
    layout->setSpacing(7);

    auto* header = new QHBoxLayout;
    header->setSpacing(7);

    auto* accentBar = new QFrame(card);
    accentBar->setObjectName("accentBar");
    accentBar->setProperty("accent", accent);
    accentBar->setFixedSize(3, 14);

    auto* titleLabel = makeLabel(title, "monitorTitle", card);
    auto* source = makeLabel("--", "monitorSource", card);
    auto* status = makeLabel(accent == "program" ? "ON AIR" : "STANDBY", "monitorStatus", card);
    status->setProperty("accent", accent);

    header->addWidget(accentBar);
    header->addWidget(titleLabel);
    header->addSpacing(4);
    header->addWidget(source);
    header->addStretch();
    header->addWidget(status);
    layout->addLayout(header);

    surface->setParent(card);
    layout->addWidget(surface, 1);

    auto* footer = new QHBoxLayout;
    auto* format = makeLabel("1920 × 1080  ·  50P", "monitorMeta", card);
    auto* audio = makeLabel("EMBEDDED AUDIO 1-2", "monitorMeta", card);
    footer->addWidget(format);
    footer->addStretch();
    footer->addWidget(audio);
    layout->addLayout(footer);

    *sourceOut = source;
    *statusOut = status;
    return card;
}

QFrame* makeSideCard(const QString& title, const QString& hint, QWidget* content, QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName("sideCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 9, 10, 10);
    layout->setSpacing(8);

    auto* header = new QHBoxLayout;
    header->addWidget(makeLabel(title, "cardTitle", card));
    header->addStretch();
    header->addWidget(makeLabel(hint, "cardHint", card));
    layout->addLayout(header);

    content->setParent(card);
    layout->addWidget(content, 1);
    return card;
}

} // namespace

struct MainWindow::Impl {
    services::InputService* inputs = nullptr;
    services::SwitcherService* switcher = nullptr;
    services::TransitionService* transition = nullptr;
    services::OutputService* outputs = nullptr;

    ProgramView* program = nullptr;
    PreviewView* preview = nullptr;
    MultiviewPanel* multiview = nullptr;
    MixerPanel* mixer = nullptr;
    OutputPanel* outputPanel = nullptr;
    TBarWidget* tbar = nullptr;
    StatusBar* status = nullptr;
    QPushButton* autoButton = nullptr;
    QLabel* programSource = nullptr;
    QLabel* previewSource = nullptr;
    QLabel* programStatus = nullptr;
    QLabel* previewStatus = nullptr;
    QLabel* clock = nullptr;
    QLabel* mixReadout = nullptr;

    QTimer* autoTimer = nullptr;
    QElapsedTimer autoClock;
    int autoDurationMs = 500;
    bool autoRunning = false;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , impl_(std::make_unique<Impl>())
{
    setWindowTitle("Mixvera - Live Production Switcher");
    resize(1560, 920);
    setMinimumSize(1280, 780);
    setStyleSheet(theme::applicationStyleSheet());

    auto& app = core::Application::instance();
    impl_->inputs = &app.services().get<services::InputService>();
    impl_->switcher = &app.services().get<services::SwitcherService>();
    impl_->transition = &app.services().get<services::TransitionService>();
    impl_->outputs = &app.services().get<services::OutputService>();

    auto* root = new QWidget(this);
    root->setObjectName("root");
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(12, 12, 12, 10);
    layout->setSpacing(10);

    layout->addWidget(buildHeader());

    auto* content = new QHBoxLayout;
    content->setSpacing(10);

    auto* stageColumn = new QVBoxLayout;
    stageColumn->setSpacing(10);
    stageColumn->addWidget(buildMonitors(), 1);
    stageColumn->addWidget(buildTransitionCard());
    content->addLayout(stageColumn, 1);
    content->addWidget(buildSidebar());

    layout->addLayout(content, 1);

    impl_->mixer = new MixerPanel(root);
    auto* mixerCard = makeSideCard("AUDIO MIXER", "5 CHANNELS  ·  BUS 1", impl_->mixer, root);
    mixerCard->setFixedHeight(206);
    layout->addWidget(mixerCard);

    setCentralWidget(root);

    impl_->status = new StatusBar(this);
    setStatusBar(impl_->status);

    impl_->autoTimer = new QTimer(this);
    impl_->autoTimer->setInterval(16);
    connect(impl_->autoTimer, &QTimer::timeout, this, &MainWindow::onAutoTick);

    auto* clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, [this] {
        impl_->clock->setText(QTime::currentTime().toString("HH:mm:ss"));
    });
    clockTimer->start(1000);

    auto* cutShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    cutShortcut->setContext(Qt::ApplicationShortcut);
    connect(cutShortcut, &QShortcut::activated, this, &MainWindow::onCut);

    auto* autoShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    autoShortcut->setContext(Qt::ApplicationShortcut);
    connect(autoShortcut, &QShortcut::activated, this, &MainWindow::onAuto);

    auto* fullScreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullScreenShortcut, &QShortcut::activated, this, [this] {
        isFullScreen() ? showNormal() : showFullScreen();
    });

    auto* escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escapeShortcut, &QShortcut::activated, this, [this] {
        if (isFullScreen()) {
            showNormal();
        }
    });

    refresh();
}

MainWindow::~MainWindow() = default;

QWidget* MainWindow::buildHeader()
{
    auto* header = new QFrame(this);
    header->setObjectName("appHeader");
    header->setFixedHeight(62);

    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(14, 8, 14, 8);
    layout->setSpacing(11);

    auto* mark = makeLabel("M", "brandMark", header);
    mark->setFixedSize(38, 38);
    mark->setAlignment(Qt::AlignCenter);

    auto* titles = new QVBoxLayout;
    titles->setSpacing(0);
    titles->addWidget(makeLabel("MIXVERA", "brandTitle", header));
    titles->addWidget(makeLabel("LIVE PRODUCTION SWITCHER  ·  DEMO BUILD", "brandSubtitle", header));

    layout->addWidget(mark);
    layout->addLayout(titles);
    layout->addStretch();
    layout->addWidget(makeLabel("4 INPUTS", "pill", header));
    layout->addWidget(makeLabel("1080P50", "pill", header));
    layout->addWidget(makeLabel("STUB ENGINE", "pill", header));

    impl_->clock = makeLabel(QTime::currentTime().toString("HH:mm:ss"), "clock", header);
    impl_->clock->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    impl_->clock->setMinimumWidth(78);
    layout->addWidget(impl_->clock);

    layout->addWidget(makeLabel("●  ON AIR", "onAirPill", header));
    return header;
}

QWidget* MainWindow::buildMonitors()
{
    auto* monitors = new QWidget(this);
    auto* layout = new QHBoxLayout(monitors);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    impl_->program = new ProgramView(monitors);
    impl_->preview = new PreviewView(monitors);

    layout->addWidget(makeMonitorCard(impl_->program, "PROGRAM", "program",
        &impl_->programSource, &impl_->programStatus, monitors), 7);
    layout->addWidget(makeMonitorCard(impl_->preview, "PREVIEW", "preview",
        &impl_->previewSource, &impl_->previewStatus, monitors), 5);

    return monitors;
}

QWidget* MainWindow::buildTransitionCard()
{
    auto* card = new QFrame(this);
    card->setObjectName("transitionCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 9, 12, 10);
    layout->setSpacing(7);

    auto* row = new QHBoxLayout;
    row->setSpacing(8);

    auto* cutButton = new QPushButton("CUT", card);
    cutButton->setObjectName("cutButton");
    cutButton->setCursor(Qt::PointingHandCursor);
    cutButton->setToolTip("Cut to preview (Space)");

    impl_->autoButton = new QPushButton("AUTO", card);
    impl_->autoButton->setObjectName("autoButton");
    impl_->autoButton->setCursor(Qt::PointingHandCursor);
    impl_->autoButton->setToolTip("Run automatic transition (Enter)");

    row->addWidget(cutButton);
    row->addWidget(impl_->autoButton);
    row->addWidget(makeDivider(card, true));
    row->addWidget(makeLabel("TRANSITION", "controlLabel", card));

    auto* kind = new QComboBox(card);
    kind->addItem("FADE", static_cast<int>(core::TransitionKind::Fade));
    kind->addItem("WIPE", static_cast<int>(core::TransitionKind::Wipe));
    kind->addItem("STINGER", static_cast<int>(core::TransitionKind::Stinger));
    kind->setFixedWidth(96);
    row->addWidget(kind);

    auto* duration = new QSpinBox(card);
    duration->setRange(100, 5000);
    duration->setSingleStep(50);
    duration->setValue(impl_->autoDurationMs);
    duration->setSuffix(" ms");
    duration->setFixedWidth(92);
    row->addWidget(duration);

    row->addStretch();

    impl_->mixReadout = makeLabel("MIX 000%", "mixReadout", card);
    impl_->mixReadout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(impl_->mixReadout);
    layout->addLayout(row);

    auto* tbarRow = new QHBoxLayout;
    tbarRow->setSpacing(9);
    tbarRow->addWidget(makeLabel("T-BAR", "controlLabel", card));
    impl_->tbar = new TBarWidget(card);
    tbarRow->addWidget(impl_->tbar, 1);
    layout->addLayout(tbarRow);

    connect(cutButton, &QPushButton::clicked, this, &MainWindow::onCut);
    connect(impl_->autoButton, &QPushButton::clicked, this, &MainWindow::onAuto);
    connect(impl_->tbar, &TBarWidget::positionChanged, this, &MainWindow::onTBarChanged);
    connect(impl_->tbar, &TBarWidget::released, this, &MainWindow::onTBarReleased);
    connect(kind, &QComboBox::currentIndexChanged, this, [this, kind](int) {
        impl_->transition->setKind(
            static_cast<core::TransitionKind>(kind->currentData().toInt()));
    });
    connect(duration, &QSpinBox::valueChanged, this, [this](int value) {
        impl_->autoDurationMs = value;
        impl_->transition->setDuration(static_cast<std::uint32_t>(value));
    });
    impl_->transition->setDuration(static_cast<std::uint32_t>(impl_->autoDurationMs));

    return card;
}

QWidget* MainWindow::buildSidebar()
{
    auto* sidebar = new QWidget(this);
    sidebar->setFixedWidth(408);
    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    impl_->multiview = new MultiviewPanel(sidebar);
    impl_->multiview->setSelectCallback([this](const QString& inputId) {
        onPreviewSelected(inputId);
    });
    layout->addWidget(makeSideCard("MULTIVIEW", "4 SOURCES", impl_->multiview, sidebar), 1);

    impl_->outputPanel = new OutputPanel(sidebar);
    layout->addWidget(makeSideCard("OUTPUTS & REPLAY", "3 DESTINATIONS", impl_->outputPanel, sidebar));

    return sidebar;
}

void MainWindow::onCut()
{
    impl_->autoTimer->stop();
    impl_->autoRunning = false;
    impl_->autoButton->setText("AUTO");
    impl_->autoButton->setEnabled(true);

    impl_->switcher->cut();
    impl_->transition->setPosition(0.0F);
    refresh();
}

void MainWindow::onAuto()
{
    if (impl_->autoRunning) {
        return;
    }
    impl_->autoRunning = true;
    impl_->autoClock.restart();
    impl_->autoTimer->start();
    impl_->autoButton->setText("RUNNING");
    impl_->autoButton->setEnabled(false);
}

void MainWindow::onAutoTick()
{
    const float position = std::clamp(
        static_cast<float>(impl_->autoClock.elapsed()) / static_cast<float>(impl_->autoDurationMs),
        0.0F, 1.0F);

    if (position < 1.0F) {
        impl_->transition->setPosition(position);
        updateTransition(position);
        return;
    }

    impl_->autoTimer->stop();
    impl_->autoRunning = false;
    impl_->transition->setPosition(1.0F);
    updateTransition(1.0F);
    impl_->transition->release();
    impl_->autoButton->setText("AUTO");
    impl_->autoButton->setEnabled(true);
    refresh();
}

void MainWindow::onTBarChanged(int value)
{
    if (impl_->autoRunning) {
        return;
    }
    const float position = static_cast<float>(value) / 1023.0F;
    impl_->transition->setPosition(position);
    updateTransition(position);
}

void MainWindow::onTBarReleased()
{
    impl_->transition->release();
    refresh();
}

void MainWindow::onPreviewSelected(const QString& inputId)
{
    const std::string id = inputId.toStdString();
    if (id == impl_->switcher->state().programInputId) {
        return;
    }
    if (impl_->switcher->setPreview(id)) {
        refresh();
    }
}

void MainWindow::updateTransition(float position)
{
    impl_->program->setTransition(position);
    impl_->preview->setTransition(position);
    impl_->mixReadout->setText(QString("MIX %1%").arg(
        static_cast<int>(std::lround(position * 100.0F)), 3, 10, QLatin1Char('0')));
    impl_->status->setTransition(position);

    const QSignalBlocker blocker(impl_->tbar);
    impl_->tbar->setValue(static_cast<int>(std::lround(position * 1023.0F)));
}

void MainWindow::refresh()
{
    const auto& state = impl_->switcher->state();

    const auto nameOf = [this](const std::string& id) {
        const auto* input = impl_->inputs->find(id);
        QString name = input ? QString::fromStdString(input->name) : QString::fromStdString(id);
        return name.toUpper();
    };

    const QString programId = QString::fromStdString(state.programInputId);
    const QString previewId = QString::fromStdString(state.previewInputId);
    impl_->program->setInputId(programId);
    impl_->preview->setInputId(previewId);
    impl_->programSource->setText(nameOf(state.programInputId));
    impl_->previewSource->setText(nameOf(state.previewInputId));
    impl_->programStatus->setText("ON AIR");
    impl_->previewStatus->setText("STANDBY");

    impl_->multiview->refresh();

    int activeOutputs = 0;
    for (const auto& output : impl_->outputs->outputs()) {
        if (output.running) {
            ++activeOutputs;
        }
    }
    impl_->status->setOutputsActive(activeOutputs,
        static_cast<int>(impl_->outputs->outputs().size()));

    updateTransition(impl_->transition->state().position);
}

} // namespace mixvera::ui
