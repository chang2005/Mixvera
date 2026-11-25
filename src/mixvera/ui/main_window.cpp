#include "mixvera/ui/main_window.hpp"

#include "mixvera/core/application.hpp"
#include "mixvera/services/audio_service.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/services/transition_service.hpp"
#include "mixvera/ui/add_input_dialog.hpp"
#include "mixvera/ui/input_panel.hpp"
#include "mixvera/ui/media/audio_meter.hpp"
#include "mixvera/ui/media/compositor.hpp"
#include "mixvera/ui/media/recorder.hpp"
#include "mixvera/ui/mixer_panel.hpp"
#include "mixvera/ui/scene_panel.hpp"
#include "mixvera/ui/settings_dialog.hpp"
#include "mixvera/ui/stage_view.hpp"
#include "mixvera/ui/status_bar.hpp"
#include "mixvera/ui/theme.hpp"
#include "mixvera/ui/transition_bar.hpp"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <utility>

namespace mixvera::ui {

namespace {

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QString::fromLatin1(objectName));
    return label;
}

QString kindPrefix(core::InputKind kind)
{
    switch (kind) {
    case core::InputKind::ScreenCapture:
        return QStringLiteral("screen");
    case core::InputKind::WindowCapture:
        return QStringLiteral("window");
    case core::InputKind::Camera:
        return QStringLiteral("camera");
    case core::InputKind::Image:
        return QStringLiteral("image");
    case core::InputKind::MediaFile:
        return QStringLiteral("media");
    case core::InputKind::Title:
        return QStringLiteral("title");
    case core::InputKind::Color:
        return QStringLiteral("colour");
    case core::InputKind::TestPattern:
        return QStringLiteral("pattern");
    default:
        return QStringLiteral("input");
    }
}

void setLivePill(QLabel* pill, bool live)
{
    if (pill->property("live").toBool() == live) {
        return;
    }
    pill->setProperty("live", live);
    pill->style()->unpolish(pill);
    pill->style()->polish(pill);
}

} // namespace

struct MainWindow::Impl {
    services::InputService* inputService = nullptr;
    services::SwitcherService* switcher = nullptr;
    services::TransitionService* transition = nullptr;
    services::AudioService* audio = nullptr;

    core::Project project;
    std::unique_ptr<Compositor> compositor;
    Recorder* recorder = nullptr;
    AudioMeter* meter = nullptr;

    StageView* program = nullptr;
    StageView* preview = nullptr;
    TransitionBar* transitionBar = nullptr;
    ScenePanel* scenes = nullptr;
    InputPanel* inputsPanel = nullptr;
    MixerPanel* mixer = nullptr;
    StatusBar* status = nullptr;
    QSplitter* workspace = nullptr;
    QToolButton* scenesToggle = nullptr;
    QToolButton* mixerToggle = nullptr;

    QLabel* projectLabel = nullptr;
    QLabel* recPill = nullptr;
    QLabel* streamPill = nullptr;
    QPushButton* recordButton = nullptr;
    QPushButton* streamButton = nullptr;
    QPushButton* overlayButtons[4] = {nullptr, nullptr, nullptr, nullptr};
    QCheckBox* previewOverlaysToggle = nullptr;
    bool previewOverlays = false;
    QImage lastProgramFrame;

    QTimer* tickTimer = nullptr;
    QTimer* secondTimer = nullptr;
    QTimer* persistTimer = nullptr;

    QElapsedTimer autoClock;
    QElapsedTimer recordClock;
    QElapsedTimer streamClock;

    bool autoRunning = false;
    float position = 0.0F;
    bool ftb = false;
    int tickCount = 0;

    std::string selectedSceneId;
    std::string selectedSourceId;
    QString projectPath;
    QString recordingFile;
};

MainWindow::MainWindow(bool restoreSession, QWidget* parent)
    : QMainWindow(parent)
    , impl_(std::make_unique<Impl>())
{
    setWindowTitle(tr("Mixvera 0.5 — Live Production Switcher"));
    resize(1600, 960);
    setMinimumSize(1280, 760);

    // --theme wins for this run; otherwise remember the saved choice.
    QString themeKey = qApp->property("mixvera/theme").toString();
    if (themeKey.isEmpty()) {
        themeKey = QSettings().value("ui/theme").toString();
    }
    applyTheme(theme::themeFromKey(themeKey));

    auto& app = core::Application::instance();
    impl_->inputService = &app.services().get<services::InputService>();
    impl_->switcher = &app.services().get<services::SwitcherService>();
    impl_->transition = &app.services().get<services::TransitionService>();
    impl_->audio = &app.services().get<services::AudioService>();

    impl_->compositor = std::make_unique<Compositor>();
    impl_->recorder = new Recorder(this);
    impl_->meter = new AudioMeter(this);

    buildLayout();
    buildMenus();
    connectSignals();

    core::Project project = core::makeDefaultProject();
    if (restoreSession) {
        core::Project saved;
        if (core::loadProjectFile(saved, projectPath().toStdString())) {
            project = saved;
        }
    }
    applyProject(project);

    impl_->tickTimer = new QTimer(this);
    impl_->tickTimer->setTimerType(Qt::PreciseTimer);
    impl_->tickTimer->setInterval(1000 / std::max(1, impl_->project.fps));
    connect(impl_->tickTimer, &QTimer::timeout, this, &MainWindow::tick);
    impl_->tickTimer->start();

    impl_->secondTimer = new QTimer(this);
    impl_->secondTimer->setInterval(500);
    connect(impl_->secondTimer, &QTimer::timeout, this, &MainWindow::refreshStatus);
    impl_->secondTimer->start();

    // Fader drags emit many values; writing the project JSON on every step
    // would stall the UI thread, so coalesce the writes.
    impl_->persistTimer = new QTimer(this);
    impl_->persistTimer->setSingleShot(true);
    impl_->persistTimer->setInterval(300);
    connect(impl_->persistTimer, &QTimer::timeout, this, &MainWindow::persistProject);

    if (impl_->meter->start()) {
        impl_->mixer->setAudioDeviceStatus(
            tr("LIVE · %1").arg(impl_->meter->deviceName()), true);
    } else {
        impl_->mixer->setAudioDeviceStatus(tr("NO AUDIO INPUT"), false);
    }
    refreshStatus();
}

MainWindow::~MainWindow() = default;

bool MainWindow::loadProject(const QString& path)
{
    core::Project project;
    if (!core::loadProjectFile(project, path.toStdString())) {
        return false;
    }
    impl_->projectPath = path;
    applyProject(project);
    return true;
}

void MainWindow::setMixerVisible(bool visible)
{
    impl_->mixerToggle->setChecked(visible);
}

void MainWindow::buildLayout()
{
    auto* root = new QWidget(this);
    root->setObjectName("root");
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* top = new QFrame(root);
    top->setObjectName("topBar");
    top->setFixedHeight(52);
    auto* topLayout = new QHBoxLayout(top);
    topLayout->setContentsMargins(10, 6, 10, 6);
    topLayout->setSpacing(8);

    auto* mark = makeLabel("M", "brandMark", top);
    mark->setFixedSize(32, 32);
    mark->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(mark);

    auto* titles = new QVBoxLayout;
    titles->setSpacing(0);
    titles->addWidget(makeLabel("MIXVERA", "brandTitle", top));
    titles->addWidget(makeLabel(tr("LIVE PRODUCTION SWITCHER"), "brandSubtitle", top));
    topLayout->addLayout(titles);

    impl_->projectLabel = makeLabel("", "projectName", top);
    topLayout->addSpacing(10);
    topLayout->addWidget(impl_->projectLabel);
    topLayout->addStretch();

    impl_->scenesToggle = new QToolButton(top);
    impl_->scenesToggle->setObjectName("topToggle");
    impl_->scenesToggle->setText(tr("Scenes"));
    impl_->scenesToggle->setCheckable(true);
    impl_->scenesToggle->setToolTip(tr("Show or hide the scenes and sources panel"));
    impl_->scenesToggle->setCursor(Qt::PointingHandCursor);
    topLayout->addWidget(impl_->scenesToggle);

    impl_->mixerToggle = new QToolButton(top);
    impl_->mixerToggle->setObjectName("topToggle");
    impl_->mixerToggle->setText(tr("Audio Mixer"));
    impl_->mixerToggle->setCheckable(true);
    impl_->mixerToggle->setToolTip(tr("Show or hide the audio mixer"));
    impl_->mixerToggle->setCursor(Qt::PointingHandCursor);
    topLayout->addWidget(impl_->mixerToggle);

    auto* settings = new QPushButton(tr("Settings"), top);
    settings->setObjectName("topButton");
    auto* fullscreen = new QPushButton(tr("Fullscreen"), top);
    fullscreen->setObjectName("topButton");
    topLayout->addWidget(settings);
    topLayout->addWidget(fullscreen);
    topLayout->addSpacing(8);

    impl_->recPill = makeLabel(tr("REC 00:00"), "pillRec", top);
    impl_->streamPill = makeLabel(tr("STREAM 00:00"), "pillStream", top);
    topLayout->addWidget(impl_->recPill);
    topLayout->addWidget(impl_->streamPill);
    layout->addWidget(top);

    impl_->workspace = new QSplitter(Qt::Horizontal, root);
    impl_->workspace->setChildrenCollapsible(false);

    impl_->scenes = new ScenePanel(impl_->workspace);
    impl_->scenes->setMinimumWidth(220);
    impl_->scenes->setMaximumWidth(320);

    auto* center = new QWidget(impl_->workspace);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(6, 6, 6, 6);
    centerLayout->setSpacing(6);

    auto* monitors = new QHBoxLayout;
    monitors->setSpacing(6);

    // vMix places preview on the left (orange) and program on the right
    // (green), with the transition cluster between them.
    auto* previewFrame = new QFrame(center);
    previewFrame->setObjectName("monitorFrame");
    previewFrame->setProperty("role", "preview");
    auto* previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(1, 1, 1, 1);
    impl_->preview = new StageView(StageView::Role::Preview, previewFrame);
    previewLayout->addWidget(impl_->preview);

    impl_->transitionBar = new TransitionBar(center);

    auto* programFrame = new QFrame(center);
    programFrame->setObjectName("monitorFrame");
    programFrame->setProperty("role", "program");
    auto* programLayout = new QVBoxLayout(programFrame);
    programLayout->setContentsMargins(1, 1, 1, 1);
    impl_->program = new StageView(StageView::Role::Program, programFrame);
    programLayout->addWidget(impl_->program);

    monitors->addWidget(previewFrame, 1);
    monitors->addWidget(impl_->transitionBar, 0, Qt::AlignVCenter);
    monitors->addWidget(programFrame, 1);
    centerLayout->addLayout(monitors, 1);

    impl_->mixer = new MixerPanel(impl_->workspace);
    impl_->mixer->setMinimumWidth(280);
    impl_->mixer->setMaximumWidth(520);

    impl_->workspace->addWidget(impl_->scenes);
    impl_->workspace->addWidget(center);
    impl_->workspace->addWidget(impl_->mixer);
    impl_->workspace->setStretchFactor(0, 0);
    impl_->workspace->setStretchFactor(1, 1);
    impl_->workspace->setStretchFactor(2, 0);
    impl_->workspace->setSizes({0, 1500, 0});
    impl_->scenes->hide();
    impl_->mixer->hide();
    layout->addWidget(impl_->workspace, 1);

    impl_->inputsPanel = new InputPanel(root);
    impl_->inputsPanel->setFixedHeight(212);
    layout->addWidget(impl_->inputsPanel);

    auto* footer = new QFrame(root);
    footer->setObjectName("footerBar");
    footer->setFixedHeight(54);
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(10, 7, 10, 7);
    footerLayout->setSpacing(8);

    auto* addInput = new QPushButton(QStringLiteral("+ ") + tr("Add Input"), footer);
    addInput->setObjectName("footerButton");
    addInput->setToolTip(tr("Add a capture, media or generated input"));
    addInput->setCursor(Qt::PointingHandCursor);
    footerLayout->addWidget(addInput);

    impl_->recordButton = new QPushButton(tr("Record"), footer);
    impl_->recordButton->setObjectName("recordButton");
    impl_->recordButton->setCheckable(true);
    impl_->recordButton->setToolTip(tr("Start / stop recording to disk (Ctrl+R)"));
    footerLayout->addWidget(impl_->recordButton);

    impl_->streamButton = new QPushButton(tr("Stream"), footer);
    impl_->streamButton->setObjectName("streamButton");
    impl_->streamButton->setCheckable(true);
    impl_->streamButton->setToolTip(tr("Start / stop the RTMP push"));
    footerLayout->addWidget(impl_->streamButton);

    auto* snapshot = new QPushButton(tr("Snapshot"), footer);
    snapshot->setObjectName("snapshotButton");
    snapshot->setToolTip(tr("Save the current program frame as a PNG (Ctrl+Shift+P)"));
    snapshot->setCursor(Qt::PointingHandCursor);
    footerLayout->addWidget(snapshot);

    footerLayout->addSpacing(8);
    footerLayout->addWidget(makeLabel(tr("OVERLAY"), "footerHint", footer));
    // Four downstream overlay channels, laid out like the vMix overlay row:
    // 1 is the lowest layer, 4 sits on top. A right-click picks the input.
    for (int layer = 1; layer <= 4; ++layer) {
        auto* button = new QPushButton(QString::number(layer), footer);
        button->setObjectName("overlayButton");
        button->setCheckable(true);
        button->setFixedWidth(40);
        button->setProperty("overlayLayer", layer);
        button->setCursor(Qt::PointingHandCursor);
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        footerLayout->addWidget(button);
        impl_->overlayButtons[layer - 1] = button;
        connect(button, &QPushButton::clicked, this, [this, layer] { onOverlayToggled(layer); });
        connect(button, &QWidget::customContextMenuRequested, this,
            [this, button, layer](const QPoint& position) {
                showOverlayMenu(layer, button->mapToGlobal(position));
            });
    }

    impl_->previewOverlaysToggle = new QCheckBox(tr("PVW OVL"), footer);
    impl_->previewOverlaysToggle->setObjectName("footerCheck");
    impl_->previewOverlaysToggle->setToolTip(
        tr("Show the enabled overlays on the preview monitor too"));
    footerLayout->addWidget(impl_->previewOverlaysToggle);

    auto* hint = makeLabel(tr("SPACE cut · ENTER transition · F11 fullscreen"),
        "footerHint", footer);
    footerLayout->addStretch();
    footerLayout->addWidget(hint);
    layout->addWidget(footer);

    setCentralWidget(root);

    impl_->status = new StatusBar(this);
    setStatusBar(impl_->status);

    connect(addInput, &QPushButton::clicked, this, &MainWindow::onAddInput);
    connect(snapshot, &QPushButton::clicked, this, &MainWindow::onSnapshot);
    connect(settings, &QPushButton::clicked, this, &MainWindow::onSettings);
    connect(impl_->previewOverlaysToggle, &QCheckBox::toggled, this,
        [this](bool enabled) { impl_->previewOverlays = enabled; });
    connect(fullscreen, &QPushButton::clicked, this, [this] {
        isFullScreen() ? showNormal() : showFullScreen();
    });

    connect(impl_->scenesToggle, &QToolButton::toggled, this, [this](bool visible) {
        impl_->scenes->setVisible(visible);
        if (!visible || impl_->workspace->width() <= 0) {
            return;
        }
        auto sizes = impl_->workspace->sizes();
        if (sizes.size() != 3) {
            sizes = {240, 1000, 0};
        }
        sizes[0] = std::clamp(impl_->workspace->width() / 6, 220, 300);
        if (!impl_->mixer->isVisible()) {
            sizes[2] = 0;
        }
        sizes[1] = std::max(520, impl_->workspace->width() - sizes[0] - sizes[2]);
        impl_->workspace->setSizes(sizes);
    });

    connect(impl_->mixerToggle, &QToolButton::toggled, this, [this](bool visible) {
        impl_->mixer->setVisible(visible);
        if (!visible || impl_->workspace->width() <= 0) {
            return;
        }
        auto sizes = impl_->workspace->sizes();
        if (sizes.size() != 3) {
            sizes = {0, 1200, 280};
        }
        sizes[2] = std::clamp(impl_->workspace->width() / 4, 320, 440);
        if (!impl_->scenes->isVisible()) {
            sizes[0] = 0;
        }
        sizes[1] = std::max(520, impl_->workspace->width() - sizes[0] - sizes[2]);
        impl_->workspace->setSizes(sizes);
    });
}

void MainWindow::buildMenus()
{
    auto* file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&New Project"), QKeySequence::New, this, [this] {
        applyProject(core::makeDefaultProject());
        persistProject();
    });
    file->addAction(tr("&Open Project…"), QKeySequence::Open, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Open Project"),
            QFileInfo(projectPath()).absolutePath(), tr("Mixvera Project (*.json)"));
        if (path.isEmpty()) {
            return;
        }
        core::Project project;
        if (!core::loadProjectFile(project, path.toStdString())) {
            QMessageBox::warning(this, tr("Open Project"), tr("The project file could not be read."));
            return;
        }
        impl_->projectPath = path;
        applyProject(project);
    });
    file->addAction(tr("&Save Project"), QKeySequence::Save, this, [this] { persistProject(); });
    file->addAction(tr("Save Project &As…"), QKeySequence::SaveAs, this, [this] {
        const QString path = QFileDialog::getSaveFileName(this, tr("Save Project"),
            projectPath(), tr("Mixvera Project (*.json)"));
        if (path.isEmpty()) {
            return;
        }
        impl_->projectPath = path;
        persistProject();
    });
    file->addSeparator();
    file->addAction(tr("&Settings…"), this, &MainWindow::onSettings);
    file->addSeparator();
    file->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);

    auto* view = menuBar()->addMenu(tr("&View"));
    auto* scenesAction = view->addAction(tr("Scenes && Sources"));
    scenesAction->setCheckable(true);
    connect(scenesAction, &QAction::toggled, impl_->scenesToggle, &QToolButton::setChecked);
    connect(impl_->scenesToggle, &QToolButton::toggled, scenesAction, &QAction::setChecked);
    auto* mixerAction = view->addAction(tr("Audio Mixer"));
    mixerAction->setCheckable(true);
    connect(mixerAction, &QAction::toggled, impl_->mixerToggle, &QToolButton::setChecked);
    connect(impl_->mixerToggle, &QToolButton::toggled, mixerAction, &QAction::setChecked);
    view->addSeparator();
    view->addAction(tr("Toggle Fullscreen"), this, [this] {
        isFullScreen() ? showNormal() : showFullScreen();
    });

    auto* scene = menuBar()->addMenu(tr("&Scene"));
    scene->addAction(tr("Add Scene"), this, &MainWindow::onSceneAdd);
    scene->addAction(tr("Duplicate Scene"), this, &MainWindow::onSceneDuplicate);
    scene->addAction(tr("Remove Scene"), this, &MainWindow::onSceneRemove);

    auto* input = menuBar()->addMenu(tr("&Input"));
    input->addAction(tr("Add Input…"), QKeySequence(tr("Ctrl+I")), this, &MainWindow::onAddInput);
    input->addAction(tr("Remove Selected Input"), this, [this] {
        const std::string id = impl_->inputsPanel->selectedId();
        if (!id.empty()) {
            onInputRemove(QString::fromStdString(id));
        }
    });

    auto* output = menuBar()->addMenu(tr("&Output"));
    output->addAction(tr("Start / Stop Recording"), QKeySequence(tr("Ctrl+R")), this, [this] {
        impl_->recordButton->setChecked(!impl_->recordButton->isChecked());
    });
    output->addAction(tr("Start / Stop Streaming"), this, [this] {
        impl_->streamButton->setChecked(!impl_->streamButton->isChecked());
    });
    output->addSeparator();
    output->addAction(tr("Save Program &Snapshot"), QKeySequence(tr("Ctrl+Shift+P")), this,
        &MainWindow::onSnapshot);

    auto* help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About Mixvera"), this, [this] {
        QMessageBox::about(this, tr("About Mixvera"),
            tr("<h3>Mixvera 0.5</h3>"
               "<p>Live production switcher: scene based compositing, real capture "
               "inputs, CUT / AUTO / T-Bar transitions with Fly and Cube, four "
               "downstream overlay layers, lower third / countdown / scoreboard "
               "titles, a channel-strip audio mixer, PNG snapshots and ffmpeg "
               "recording.</p>"
               "<p>The workspace follows vMix: preview on the left, program on the "
               "right and a vertical transition cluster between them. The input "
               "bar, overlay row and footer keep the show controls within one "
               "click.</p>"
               "<p>Three themes (Charcoal, Yami, Daylight), an OBS style settings "
               "window and a Simplified Chinese / English interface.</p>"));
    });
}

void MainWindow::connectSignals()
{
    connect(impl_->transitionBar, &TransitionBar::cutRequested, this, &MainWindow::onCut);
    connect(impl_->transitionBar, &TransitionBar::autoRequested, this, &MainWindow::onAuto);
    connect(impl_->transitionBar, &TransitionBar::positionChanged, this,
        &MainWindow::onTbarMoved);
    connect(impl_->transitionBar, &TransitionBar::positionReleased, this,
        &MainWindow::onTbarReleased);
    connect(impl_->transitionBar, &TransitionBar::ftbToggled, this, &MainWindow::onFtbToggled);

    connect(impl_->transitionBar, &TransitionBar::kindChanged, this,
        [this](core::TransitionKind kind) {
            impl_->transition->setKind(kind);
            impl_->project.transitionKind = kind;
            persistProject();
        });
    connect(impl_->transitionBar, &TransitionBar::durationChanged, this, [this](int ms) {
        impl_->transition->setDuration(static_cast<std::uint32_t>(ms));
        impl_->project.transitionDurationMs = static_cast<std::uint32_t>(ms);
        persistProject();
    });

    connect(impl_->scenes, &ScenePanel::sceneSelected, this, &MainWindow::onSceneSelected);
    connect(impl_->scenes, &ScenePanel::sceneActivated, this, &MainWindow::onSceneActivated);
    connect(impl_->scenes, &ScenePanel::sceneAddRequested, this, &MainWindow::onSceneAdd);
    connect(impl_->scenes, &ScenePanel::sceneDuplicateRequested, this,
        &MainWindow::onSceneDuplicate);
    connect(impl_->scenes, &ScenePanel::sceneRemoveRequested, this, &MainWindow::onSceneRemove);
    connect(impl_->scenes, &ScenePanel::sceneRenamed, this, &MainWindow::onSceneRenamed);
    connect(impl_->scenes, &ScenePanel::sourceSelected, this, &MainWindow::onSourceSelected);
    connect(impl_->scenes, &ScenePanel::sourceAddRequested, this, &MainWindow::onSourceAdd);
    connect(impl_->scenes, &ScenePanel::sourceRemoveRequested, this, &MainWindow::onSourceRemove);
    connect(impl_->scenes, &ScenePanel::sourceMoveRequested, this, &MainWindow::onSourceMove);
    connect(impl_->scenes, &ScenePanel::sourceVisibilityChanged, this,
        &MainWindow::onSourceVisibility);

    connect(impl_->inputsPanel, &InputPanel::addInputRequested, this, &MainWindow::onAddInput);
    connect(impl_->inputsPanel, &InputPanel::inputActivated, this,
        &MainWindow::onInputActivated);
    connect(impl_->inputsPanel, &InputPanel::previewRequested, this,
        &MainWindow::onInputPreview);
    connect(impl_->inputsPanel, &InputPanel::cutRequested, this, &MainWindow::onInputCut);
    connect(impl_->inputsPanel, &InputPanel::addToSceneRequested, this,
        &MainWindow::onInputAddToScene);
    connect(impl_->inputsPanel, &InputPanel::overlayAssignRequested, this,
        &MainWindow::onOverlayAssigned);
    connect(impl_->inputsPanel, &InputPanel::overlayClearRequested, this,
        &MainWindow::onOverlayCleared);
    connect(impl_->inputsPanel, &InputPanel::titleActionRequested, this,
        &MainWindow::onTitleAction);
    connect(impl_->inputsPanel, &InputPanel::removeRequested, this, &MainWindow::onInputRemove);
    connect(impl_->inputsPanel, &InputPanel::propertiesRequested, this,
        &MainWindow::onInputProperties);

    connect(impl_->mixer, &MixerPanel::gainChanged, this,
        [this](const QString& id, float gain) {
            impl_->audio->setGain(id.toStdString(), gain);
            const auto* input = impl_->inputService->find(id.toStdString());
            if (input) {
                auto copy = *input;
                copy.volumeDb = gain;
                impl_->inputService->update(copy);
            }
            schedulePersist();
        });
    connect(impl_->mixer, &MixerPanel::muteChanged, this,
        [this](const QString& id, bool muted) {
            impl_->audio->setMuted(id.toStdString(), muted);
            const auto* input = impl_->inputService->find(id.toStdString());
            if (input) {
                auto copy = *input;
                copy.muted = muted;
                impl_->inputService->update(copy);
            }
            impl_->inputsPanel->setInputs(impl_->inputService->inputs());
            schedulePersist();
        });
    connect(impl_->mixer, &MixerPanel::soloChanged, this,
        [this](const QString& id, bool solo) {
            impl_->audio->setSolo(id.toStdString(), solo);
        });

    connect(impl_->recordButton, &QPushButton::toggled, this, &MainWindow::onRecordToggled);
    connect(impl_->streamButton, &QPushButton::toggled, this, &MainWindow::onStreamToggled);

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
}

QString MainWindow::projectPath() const
{
    if (!impl_->projectPath.isEmpty()) {
        return impl_->projectPath;
    }
    const QString directory = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    return QDir(directory).filePath("project.json");
}

void MainWindow::applyProject(core::Project project)
{
    impl_->project = std::move(project);

    // The bundled welcome project keeps stable ids but its display names follow
    // the interface language. Match both the English source and the translated
    // form so switching zh_CN → en also restores the default names. User-renamed
    // inputs are left untouched because their names no longer match either form.
    const auto localizedName = [this](const std::string& name) {
        const QString value = QString::fromStdString(name);
        const QString translations[] = {
            tr("Welcome"), tr("Camera 1"), tr("Camera 2"),
            tr("Mixvera Title"), tr("Scene 1"), tr("Scene 2"),
        };
        const char* sources[] = {
            "Welcome", "Camera 1", "Camera 2", "Mixvera Title", "Scene 1", "Scene 2",
        };
        for (int index = 0; index < 6; ++index) {
            if (value == QLatin1String(sources[index]) || value == translations[index]) {
                return translations[index].toStdString();
            }
        }
        return name;
    };
    impl_->project.name = localizedName(impl_->project.name);
    for (auto& input : impl_->project.inputs) {
        input.name = localizedName(input.name);
    }

    impl_->compositor->setCanvasSize(QSize(impl_->project.canvasWidth,
        impl_->project.canvasHeight));
    impl_->compositor->setFrameRate(impl_->project.fps);

    impl_->inputService->clear();
    impl_->inputService->load(impl_->project.inputs);

    impl_->audio->clear();
    for (const auto& input : impl_->inputService->inputs()) {
        if (input.hasAudio) {
            impl_->audio->addChannel(input.id);
            impl_->audio->setGain(input.id, input.volumeDb);
            impl_->audio->setMuted(input.id, input.muted);
        }
    }

    if (!impl_->project.programId.empty()
        && impl_->inputService->find(impl_->project.programId)) {
        impl_->switcher->setPreview(impl_->project.programId);
        impl_->switcher->take();
    }
    if (!impl_->project.previewId.empty()
        && impl_->inputService->find(impl_->project.previewId)) {
        impl_->switcher->setPreview(impl_->project.previewId);
    }

    impl_->transition->setKind(impl_->project.transitionKind);
    impl_->transition->setDuration(impl_->project.transitionDurationMs);
    impl_->transitionBar->setKind(impl_->project.transitionKind);
    impl_->transitionBar->setDurationMs(
        static_cast<int>(impl_->project.transitionDurationMs));

    impl_->position = 0.0F;
    impl_->autoRunning = false;
    impl_->ftb = false;
    impl_->transitionBar->setFtb(false);
    impl_->transitionBar->setAutoRunning(false);
    impl_->transitionBar->setPosition(0.0F);

    impl_->previewOverlays = false;
    if (impl_->previewOverlaysToggle) {
        QSignalBlocker blocker(impl_->previewOverlaysToggle);
        impl_->previewOverlaysToggle->setChecked(false);
    }
    refreshOverlayButtons();

    impl_->compositor->sync(impl_->inputService->inputs());

    const auto& state = impl_->switcher->state();
    impl_->selectedSceneId = state.programInputId;
    if (!impl_->inputService->find(impl_->selectedSceneId)) {
        impl_->selectedSceneId.clear();
        for (const auto& input : impl_->inputService->inputs()) {
            if (input.isScene()) {
                impl_->selectedSceneId = input.id;
                break;
            }
        }
    }

    if (impl_->tickTimer) {
        impl_->tickTimer->setInterval(1000 / std::max(1, impl_->project.fps));
    }

    refreshPanels();
    refreshStatus();
}

void MainWindow::applyTheme(theme::ThemeId id)
{
    theme::setTheme(id);
    setStyleSheet(theme::styleSheet(id));
    // StageView and the mixer meters paint with theme::palette() directly, so
    // a repaint is all they need.
    update();
}

void MainWindow::schedulePersist()
{
    if (impl_->persistTimer) {
        impl_->persistTimer->start();
    }
}

void MainWindow::persistProject()
{
    if (impl_->persistTimer) {
        impl_->persistTimer->stop();
    }
    impl_->project.inputs = impl_->inputService->inputs();
    const auto& state = impl_->switcher->state();
    impl_->project.programId = state.programInputId;
    impl_->project.previewId = state.previewInputId;
    impl_->project.transitionKind = impl_->transition->state().kind;
    impl_->project.transitionDurationMs = impl_->transition->state().durationMs;

    const QString path = projectPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    core::saveProjectFile(impl_->project, path.toStdString());
}

void MainWindow::refreshPanels()
{
    const auto all = impl_->inputService->inputs();
    const auto& state = impl_->switcher->state();

    std::vector<core::InputDescriptor> scenes;
    for (const auto& input : all) {
        if (input.isScene()) {
            scenes.push_back(input);
        }
    }

    impl_->scenes->setScenes(scenes, state.programInputId, state.previewInputId);
    if (!impl_->selectedSceneId.empty()) {
        impl_->scenes->selectScene(impl_->selectedSceneId);
    }
    impl_->scenes->setSources(impl_->inputService->find(impl_->selectedSceneId), all);

    impl_->inputsPanel->setInputs(all);
    impl_->inputsPanel->setProgramPreview(state.programInputId, state.previewInputId);

    impl_->transitionBar->setLiveTransitionName(tr("%1 → %2")
            .arg(nameOf(state.previewInputId), nameOf(state.programInputId)));

    refreshMixer();
    refreshStage();
}

void MainWindow::refreshStage()
{
    const auto& state = impl_->switcher->state();
    impl_->program->setSourceName(nameOf(state.programInputId));
    impl_->preview->setSourceName(nameOf(state.previewInputId));
    impl_->program->setLive(true);
    impl_->preview->setLive(false);
}

void MainWindow::refreshMixer()
{
    impl_->mixer->setChannels(impl_->inputService->inputs(), impl_->audio->channels());
}

void MainWindow::refreshThumbnails()
{
    const auto all = impl_->inputService->inputs();
    const QSize thumb(96, 54);
    for (const auto& input : all) {
        QImage image;
        if (input.isScene()) {
            image = impl_->compositor->renderInput(all, input.id)
                        .scaled(thumb, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            image = impl_->compositor->thumbnail(input.id, thumb);
        }
        impl_->inputsPanel->setThumbnail(input.id, image);
    }
}

void MainWindow::refreshStatus()
{
    impl_->status->setEngineText(tr("ENGINE · Qt Multimedia + ffmpeg"));
    impl_->status->setCanvasText(tr("CANVAS %1×%2 · %3p")
            .arg(impl_->project.canvasWidth)
            .arg(impl_->project.canvasHeight)
            .arg(impl_->project.fps));
    impl_->status->setProjectText(tr("PROJECT · %1").arg(QString::fromStdString(
        impl_->project.name)));

    const bool recording = impl_->recorder->recording();
    const bool streaming = impl_->recorder->streaming();
    impl_->status->setOutputText(tr("OUTPUTS · %1%2")
            .arg(recording ? tr("REC ") : QString())
            .arg(streaming ? tr("STREAM") : (recording ? QString::fromUtf8("—") : tr("—"))));
    impl_->status->setOnAir(recording || streaming);
    impl_->status->setClock(QDateTime::currentDateTime().toString("HH:mm:ss"));

    const auto elapsed = [](qint64 ms) {
        const qint64 seconds = ms / 1000;
        return QString("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10,
            QLatin1Char('0'));
    };
    impl_->recPill->setText(QString("REC %1").arg(elapsed(recording
                ? impl_->recordClock.elapsed()
                : 0)));
    impl_->streamPill->setText(QString("STREAM %1").arg(elapsed(streaming
                ? impl_->streamClock.elapsed()
                : 0)));
    setLivePill(impl_->recPill, recording);
    setLivePill(impl_->streamPill, streaming);

    impl_->projectLabel->setText(tr("%1  ·  %2×%3  %4p")
            .arg(QString::fromStdString(impl_->project.name))
            .arg(impl_->project.canvasWidth)
            .arg(impl_->project.canvasHeight)
            .arg(impl_->project.fps));
}

void MainWindow::tick()
{
    const auto all = impl_->inputService->inputs();
    impl_->compositor->refresh(all);

    const auto& state = impl_->switcher->state();
    QImage programFrame = impl_->compositor->renderInput(all, state.programInputId);
    QImage previewFrame = impl_->compositor->renderInput(all, state.previewInputId);

    const bool hasOverlay = std::any_of(impl_->project.overlays.begin(),
        impl_->project.overlays.end(), [](const auto& overlay) {
            return overlay.enabled && !overlay.inputId.empty();
        });

    if (impl_->autoRunning) {
        const float progress = static_cast<float>(impl_->autoClock.elapsed())
            / static_cast<float>(std::max<std::uint32_t>(impl_->transition->state().durationMs, 1));
        impl_->position = std::clamp(progress, 0.0F, 1.0F);
        impl_->transition->setPosition(impl_->position);
        impl_->transitionBar->setPosition(impl_->position);

        if (impl_->position >= 1.0F) {
            impl_->autoRunning = false;
            impl_->position = 0.0F;
            impl_->switcher->take();
            impl_->transitionBar->setAutoRunning(false);
            impl_->transitionBar->setPosition(0.0F);
            refreshPanels();
            persistProject();
        }
    }

    if (impl_->position > 0.001F) {
        programFrame = impl_->compositor->mix(programFrame, previewFrame,
            impl_->transition->state().kind, impl_->position);
    }
    impl_->program->setTransition(impl_->position);

    // Downstream overlays sit above the transition but below fade-to-black,
    // matching the vMix / SinSam bus order.
    if (hasOverlay) {
        programFrame = impl_->compositor->applyOverlays(programFrame, all,
            impl_->project.overlays);
        if (impl_->previewOverlays) {
            previewFrame = impl_->compositor->applyOverlays(previewFrame, all,
                impl_->project.overlays);
        }
    }

    if (impl_->ftb) {
        programFrame = impl_->compositor->fadeToBlack(programFrame, 1.0F);
    }

    impl_->lastProgramFrame = programFrame;
    impl_->program->setFrame(programFrame);
    impl_->preview->setFrame(previewFrame);
    impl_->recorder->submit(programFrame);

    impl_->mixer->setMasterLevel(impl_->meter->levelDb());

    if (++impl_->tickCount % 6 == 0) {
        refreshThumbnails();
    }
}

void MainWindow::onCut()
{
    impl_->autoRunning = false;
    impl_->transitionBar->setAutoRunning(false);
    impl_->switcher->cut();
    resetTransition();
    refreshPanels();
    persistProject();
}

void MainWindow::onAuto()
{
    if (impl_->autoRunning) {
        return;
    }
    impl_->autoRunning = true;
    impl_->autoClock.restart();
    impl_->transitionBar->setAutoRunning(true);
}

void MainWindow::onTbarMoved(float position)
{
    if (impl_->autoRunning) {
        return;
    }
    impl_->position = std::clamp(position, 0.0F, 1.0F);
    impl_->transition->setPosition(impl_->position);
    impl_->transitionBar->setPosition(impl_->position);
}

void MainWindow::onTbarReleased()
{
    if (impl_->autoRunning) {
        return;
    }
    if (impl_->position >= 0.5F) {
        impl_->switcher->take();
    }
    resetTransition();
    refreshPanels();
    persistProject();
}

void MainWindow::onFtbToggled(bool engaged)
{
    impl_->ftb = engaged;
}

void MainWindow::onOverlayToggled(int layer)
{
    core::OverlayLayer* overlay = nullptr;
    for (auto& candidate : impl_->project.overlays) {
        if (candidate.layer == layer) {
            overlay = &candidate;
            break;
        }
    }
    if (overlay && overlay->enabled) {
        setOverlay(layer, overlay->inputId, false);
        return;
    }
    if (overlay && !overlay->inputId.empty()) {
        setOverlay(layer, overlay->inputId, true);
        return;
    }

    const std::string selected = impl_->inputsPanel->selectedId();
    if (!selected.empty()) {
        setOverlay(layer, selected, true);
        return;
    }
    if (auto* button = impl_->overlayButtons[layer - 1]) {
        showOverlayMenu(layer, button->mapToGlobal(button->rect().center()));
    }
}

void MainWindow::showOverlayMenu(int layer, const QPoint& globalPos)
{
    QMenu menu(this);
    QAction* none = menu.addAction(tr("None (clear layer)"));
    menu.addSeparator();

    std::string assigned;
    for (const auto& overlay : impl_->project.overlays) {
        if (overlay.layer == layer) {
            assigned = overlay.inputId;
        }
    }

    std::vector<std::pair<QAction*, std::string>> entries;
    for (const auto& input : impl_->inputService->inputs()) {
        QAction* action = menu.addAction(QString::fromStdString(input.name));
        action->setCheckable(true);
        action->setChecked(input.id == assigned);
        entries.emplace_back(action, input.id);
    }

    QAction* chosen = menu.exec(globalPos);
    if (!chosen) {
        return;
    }
    if (chosen == none) {
        setOverlay(layer, {}, false);
        return;
    }
    for (const auto& [action, inputId] : entries) {
        if (action == chosen) {
            setOverlay(layer, inputId, true);
            return;
        }
    }
}

void MainWindow::setOverlay(int layer, const std::string& inputId, bool enabled)
{
    if (layer < 1 || layer > 4) {
        return;
    }
    for (int index = 1; index <= 4; ++index) {
        const bool exists = std::any_of(impl_->project.overlays.begin(),
            impl_->project.overlays.end(),
            [index](const auto& overlay) { return overlay.layer == index; });
        if (!exists) {
            core::OverlayLayer overlay;
            overlay.layer = index;
            impl_->project.overlays.push_back(overlay);
        }
    }
    for (auto& overlay : impl_->project.overlays) {
        if (!inputId.empty() && overlay.layer != layer && overlay.inputId == inputId) {
            overlay.inputId.clear();
            overlay.enabled = false;
        }
        if (overlay.layer == layer) {
            overlay.inputId = inputId;
            overlay.enabled = enabled && !inputId.empty();
        }
    }
    refreshOverlayButtons();
    schedulePersist();
}

void MainWindow::refreshOverlayButtons()
{
    for (int layer = 1; layer <= 4; ++layer) {
        QPushButton* button = impl_->overlayButtons[layer - 1];
        if (!button) {
            continue;
        }
        const core::OverlayLayer* overlay = nullptr;
        for (const auto& candidate : impl_->project.overlays) {
            if (candidate.layer == layer) {
                overlay = &candidate;
                break;
            }
        }
        const bool enabled = overlay && overlay->enabled && !overlay->inputId.empty();
        QSignalBlocker blocker(button);
        button->setChecked(enabled);
        if (overlay && !overlay->inputId.empty()) {
            button->setToolTip(tr("Overlay %1 — %2 (%3)\nClick to toggle, right-click to choose an input")
                    .arg(layer)
                    .arg(nameOf(overlay->inputId))
                    .arg(enabled ? tr("on") : tr("off")));
        } else {
            button->setToolTip(tr("Overlay %1 — unassigned\nClick to choose an input")
                    .arg(layer));
        }
    }
}

void MainWindow::onOverlayAssigned(const QString& id, int layer)
{
    setOverlay(layer, id.toStdString(), true);
    statusBar()->showMessage(tr("%1 assigned to overlay %2")
            .arg(nameOf(id.toStdString()))
            .arg(layer),
        4000);
}

void MainWindow::onOverlayCleared(const QString& id)
{
    const std::string inputId = id.toStdString();
    bool changed = false;
    for (auto& overlay : impl_->project.overlays) {
        if (overlay.inputId == inputId) {
            overlay.inputId.clear();
            overlay.enabled = false;
            changed = true;
        }
    }
    if (!changed) {
        return;
    }
    refreshOverlayButtons();
    schedulePersist();
    statusBar()->showMessage(tr("%1 removed from overlays").arg(nameOf(inputId)), 4000);
}

void MainWindow::onTitleAction(const QString& id, const QString& action)
{
    const auto* input = impl_->inputService->find(id.toStdString());
    if (!input || input->kind != core::InputKind::Title) {
        return;
    }
    auto copy = *input;
    const auto read = [&copy](const char* key) {
        const auto it = copy.params.find(key);
        return it == copy.params.end() ? QString() : QString::fromStdString(it->second);
    };
    const auto write = [&copy](const char* key, qint64 value) {
        copy.params[key] = std::to_string(value);
    };
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool changed = true;

    if (action == "countdownStart") {
        qint64 duration = read("countdownSeconds").toLongLong();
        if (duration <= 0) {
            duration = 300;
        }
        qint64 remaining = read("countdownRemaining").toLongLong();
        if (read("countdownEnd").toLongLong() > now) {
            changed = false;
        } else {
            if (remaining <= 0 || remaining > duration * 1000) {
                remaining = duration * 1000;
            }
            write("countdownEnd", now + remaining);
            write("countdownRemaining", remaining);
            write("countdownPaused", 0);
        }
    } else if (action == "countdownPause") {
        const qint64 end = read("countdownEnd").toLongLong();
        if (end <= 0) {
            changed = false;
        } else {
            const qint64 remaining = std::max<qint64>(0, end - now);
            write("countdownEnd", 0);
            write("countdownRemaining", remaining);
            write("countdownPaused", 1);
        }
    } else if (action == "countdownReset") {
        qint64 duration = read("countdownSeconds").toLongLong();
        if (duration <= 0) {
            duration = 300;
        }
        write("countdownEnd", 0);
        write("countdownRemaining", duration * 1000);
        write("countdownPaused", 0);
    } else if (action == "homePlus" || action == "homeMinus" || action == "awayPlus"
        || action == "awayMinus") {
        const bool home = action.startsWith("home");
        const bool plus = action.endsWith("Plus");
        const char* key = home ? "scoreHome" : "scoreAway";
        const int score = std::max(0, read(key).toInt() + (plus ? 1 : -1));
        write(key, score);
    } else {
        changed = false;
    }

    if (!changed) {
        return;
    }
    impl_->inputService->update(copy);
    refreshPanels();
    schedulePersist();
}

void MainWindow::onSnapshot()
{
    QImage frame = impl_->lastProgramFrame;
    if (frame.isNull()) {
        const auto all = impl_->inputService->inputs();
        const auto& state = impl_->switcher->state();
        frame = impl_->compositor->renderInput(all, state.programInputId);
        frame = impl_->compositor->applyOverlays(frame, all, impl_->project.overlays);
        if (impl_->ftb) {
            frame = impl_->compositor->fadeToBlack(frame, 1.0F);
        }
    }
    if (frame.isNull()) {
        return;
    }

    const QString directory = outputDirectory();
    QDir().mkpath(directory);
    const QString file = QDir(directory).filePath(
        QString("Mixvera_snapshot_%1.png")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
    if (!frame.save(file, "PNG")) {
        QMessageBox::warning(this, tr("Snapshot"),
            tr("The snapshot could not be saved to %1.").arg(file));
        return;
    }
    statusBar()->showMessage(tr("Snapshot saved: %1").arg(file), 6000);
}

void MainWindow::resetTransition()
{
    impl_->position = 0.0F;
    impl_->transition->setPosition(0.0F);
    impl_->transitionBar->setPosition(0.0F);
}

void MainWindow::onSceneSelected(const QString& id)
{
    impl_->selectedSceneId = id.toStdString();
    impl_->selectedSourceId.clear();
    impl_->scenes->setSources(impl_->inputService->find(impl_->selectedSceneId),
        impl_->inputService->inputs());
    previewInput(impl_->selectedSceneId);
}

void MainWindow::onSceneActivated(const QString& id)
{
    cutTo(id.toStdString());
}

void MainWindow::onSceneAdd()
{
    core::InputDescriptor scene;
    scene.id = impl_->inputService->uniqueId("scene");
    scene.kind = core::InputKind::Scene;
    const QString number = QString::fromStdString(scene.id).section('-', 1);
    scene.name = tr("Scene %1").arg(number).toStdString();
    impl_->inputService->add(scene);
    impl_->selectedSceneId = scene.id;
    previewInput(scene.id);
    refreshPanels();
    persistProject();
}

void MainWindow::onSceneDuplicate()
{
    const auto* current = impl_->inputService->find(impl_->selectedSceneId);
    if (!current) {
        return;
    }
    core::InputDescriptor copy = *current;
    copy.id = impl_->inputService->uniqueId("scene");
    copy.name = tr("%1 copy").arg(QString::fromStdString(current->name)).toStdString();
    impl_->inputService->add(copy);
    impl_->selectedSceneId = copy.id;
    refreshPanels();
    persistProject();
}

void MainWindow::onSceneRemove()
{
    std::size_t sceneCount = 0;
    for (const auto& input : impl_->inputService->inputs()) {
        if (input.isScene()) {
            ++sceneCount;
        }
    }
    if (sceneCount <= 1) {
        QMessageBox::information(this, tr("Remove Scene"),
            tr("A project needs at least one scene."));
        return;
    }
    const std::string id = impl_->selectedSceneId;
    if (id.empty()) {
        return;
    }
    impl_->inputService->remove(id);
    impl_->selectedSceneId.clear();
    for (const auto& input : impl_->inputService->inputs()) {
        if (input.isScene()) {
            impl_->selectedSceneId = input.id;
            break;
        }
    }
    previewInput(impl_->selectedSceneId);
    impl_->switcher->cut();
    resetTransition();
    refreshPanels();
    persistProject();
}

void MainWindow::onSceneRenamed(const QString& id, const QString& name)
{
    impl_->inputService->rename(id.toStdString(), name.toStdString());
    refreshPanels();
    persistProject();
}

void MainWindow::onSourceSelected(const QString& id)
{
    impl_->selectedSourceId = id.toStdString();
}

void MainWindow::onSourceAdd()
{
    std::vector<core::InputDescriptor> candidates;
    QStringList names;
    for (const auto& input : impl_->inputService->inputs()) {
        if (input.isScene()) {
            continue;
        }
        const auto* current = impl_->inputService->find(impl_->selectedSceneId);
        bool alreadyInside = false;
        if (current) {
            for (const auto& item : current->items) {
                if (item.inputId == input.id) {
                    alreadyInside = true;
                    break;
                }
            }
        }
        if (alreadyInside) {
            continue;
        }
        candidates.push_back(input);
        names << QString::fromStdString(input.name);
    }
    if (candidates.empty()) {
        QMessageBox::information(this, tr("Add Source"),
            tr("Every input is already used by this scene."));
        return;
    }

    bool accepted = false;
    const QString chosen = QInputDialog::getItem(this, tr("Add Source"),
        tr("Input to add to the current scene:"), names, 0, false, &accepted);
    if (!accepted) {
        return;
    }
    const int index = names.indexOf(chosen);
    if (index < 0) {
        return;
    }

    auto* scene = impl_->inputService->find(impl_->selectedSceneId);
    if (!scene) {
        return;
    }
    auto copy = *scene;
    copy.items.push_back({candidates[index].id, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, true});
    impl_->inputService->update(copy);
    refreshPanels();
    persistProject();
}

void MainWindow::onSourceRemove()
{
    auto* scene = impl_->inputService->find(impl_->selectedSceneId);
    if (!scene || impl_->selectedSourceId.empty()) {
        return;
    }
    auto copy = *scene;
    copy.items.erase(std::remove_if(copy.items.begin(), copy.items.end(),
                         [this](const auto& item) {
                             return item.inputId == impl_->selectedSourceId;
                         }),
        copy.items.end());
    impl_->inputService->update(copy);
    impl_->selectedSourceId.clear();
    refreshPanels();
    persistProject();
}

void MainWindow::onSourceMove(const QString& id, int delta)
{
    auto* scene = impl_->inputService->find(impl_->selectedSceneId);
    if (!scene) {
        return;
    }
    auto copy = *scene;
    for (std::size_t index = 0; index < copy.items.size(); ++index) {
        if (copy.items[index].inputId != id.toStdString()) {
            continue;
        }
        const int target = static_cast<int>(index) + delta;
        if (target < 0 || target >= static_cast<int>(copy.items.size())) {
            return;
        }
        std::swap(copy.items[index], copy.items[static_cast<std::size_t>(target)]);
        break;
    }
    impl_->inputService->update(copy);
    refreshPanels();
    persistProject();
}

void MainWindow::onSourceVisibility(const QString& id, bool visible)
{
    auto* scene = impl_->inputService->find(impl_->selectedSceneId);
    if (!scene) {
        return;
    }
    auto copy = *scene;
    for (auto& item : copy.items) {
        if (item.inputId == id.toStdString()) {
            item.visible = visible;
        }
    }
    impl_->inputService->update(copy);
    refreshPanels();
    persistProject();
}

void MainWindow::onAddInput()
{
    AddInputDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    core::InputDescriptor descriptor = dialog.descriptor();
    const QString prefix = kindPrefix(descriptor.kind);
    descriptor.id = impl_->inputService->uniqueId(prefix.toStdString());
    if (descriptor.name.empty()) {
        descriptor.name = InputPanel::kindLabel(descriptor.kind).toStdString();
    }

    impl_->inputService->add(descriptor);
    if (descriptor.hasAudio) {
        impl_->audio->addChannel(descriptor.id);
    }

    auto* scene = impl_->inputService->find(impl_->selectedSceneId);
    if (scene) {
        auto copy = *scene;
        copy.items.push_back({descriptor.id, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, true});
        impl_->inputService->update(copy);
    }

    impl_->inputsPanel->selectInput(descriptor.id);
    refreshPanels();
    persistProject();
}

void MainWindow::onInputActivated(const QString& id)
{
    previewInput(id.toStdString());
}

void MainWindow::onInputPreview(const QString& id)
{
    previewInput(id.toStdString());
}

void MainWindow::onInputCut(const QString& id)
{
    cutTo(id.toStdString());
}

void MainWindow::onInputAddToScene(const QString& id)
{
    auto* scene = impl_->inputService->find(impl_->selectedSceneId);
    const auto* input = impl_->inputService->find(id.toStdString());
    if (!scene || !input || input->isScene()) {
        return;
    }
    auto copy = *scene;
    for (const auto& item : copy.items) {
        if (item.inputId == input->id) {
            return;
        }
    }
    copy.items.push_back({input->id, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, true});
    impl_->inputService->update(copy);
    refreshPanels();
    persistProject();
}

void MainWindow::onInputRemove(const QString& id)
{
    const std::string inputId = id.toStdString();
    const auto& state = impl_->switcher->state();
    if (inputId == state.programInputId || inputId == state.previewInputId) {
        QMessageBox::information(this, tr("Remove Input"),
            tr("An input that is on program or preview cannot be removed. "
               "Switch it away first."));
        return;
    }

    for (const auto& input : impl_->inputService->inputs()) {
        if (!input.isScene()) {
            continue;
        }
        auto copy = input;
        const auto before = copy.items.size();
        copy.items.erase(std::remove_if(copy.items.begin(), copy.items.end(),
                             [&](const auto& item) { return item.inputId == inputId; }),
            copy.items.end());
        if (copy.items.size() != before) {
            impl_->inputService->update(copy);
        }
    }

    bool overlayChanged = false;
    for (auto& overlay : impl_->project.overlays) {
        if (overlay.inputId == inputId) {
            overlay.inputId.clear();
            overlay.enabled = false;
            overlayChanged = true;
        }
    }
    if (overlayChanged) {
        refreshOverlayButtons();
    }

    impl_->inputService->remove(inputId);
    refreshPanels();
    persistProject();
}

void MainWindow::onInputProperties(const QString& id)
{
    const auto* input = impl_->inputService->find(id.toStdString());
    if (!input) {
        return;
    }

    if (input->isScene()) {
        bool accepted = false;
        const QString name = QInputDialog::getText(this, tr("Rename Scene"),
            tr("Scene name"), QLineEdit::Normal, QString::fromStdString(input->name),
            &accepted);
        if (accepted && !name.trimmed().isEmpty()) {
            impl_->inputService->rename(input->id, name.trimmed().toStdString());
            refreshPanels();
            persistProject();
        }
        return;
    }

    AddInputDialog dialog(this);
    dialog.loadFrom(*input);
    dialog.lockKind(true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    core::InputDescriptor updated = dialog.descriptor();
    updated.id = input->id;
    updated.items = input->items;
    if (updated.name.empty()) {
        updated.name = input->name;
    }
    impl_->inputService->update(updated);
    refreshPanels();
    persistProject();
}

void MainWindow::previewInput(const std::string& id)
{
    if (id.empty()) {
        return;
    }
    if (id == impl_->switcher->state().programInputId
        || id == impl_->switcher->state().previewInputId) {
        return;
    }
    if (impl_->switcher->setPreview(id)) {
        refreshPanels();
    }
}

void MainWindow::cutTo(const std::string& id)
{
    if (id.empty() || id == impl_->switcher->state().programInputId) {
        return;
    }
    impl_->switcher->setPreview(id);
    impl_->switcher->cut();
    resetTransition();
    refreshPanels();
    persistProject();
}

void MainWindow::onRecordToggled(bool start)
{
    if (start) {
        const QString directory = outputDirectory();
        QDir().mkpath(directory);
        const QString file = QDir(directory).filePath(
            QString("Mixvera_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
        QString error;
        if (!impl_->recorder->startRecording(file,
                QSize(impl_->project.canvasWidth, impl_->project.canvasHeight),
                impl_->project.fps, &error)) {
            QMessageBox::warning(this, tr("Record"), error);
            QSignalBlocker blocker(impl_->recordButton);
            impl_->recordButton->setChecked(false);
            return;
        }
        impl_->recordingFile = file;
        impl_->recordClock.restart();
    } else {
        impl_->recorder->stopRecording();
        impl_->recordingFile.clear();
    }
    refreshStatus();
}

void MainWindow::onStreamToggled(bool start)
{
    if (start) {
        QString url = QString::fromStdString(impl_->project.outputs.streamUrl);
        const QString key = QString::fromStdString(impl_->project.outputs.streamKey);
        if (!key.isEmpty()) {
            if (!url.endsWith('/')) {
                url += '/';
            }
            url += key;
        }
        if (url.isEmpty()) {
            QMessageBox::information(this, tr("Stream"),
                tr("Configure an RTMP URL in Settings before streaming."));
            QSignalBlocker blocker(impl_->streamButton);
            impl_->streamButton->setChecked(false);
            onSettings();
            return;
        }
        QString error;
        if (!impl_->recorder->startStreaming(url,
                QSize(impl_->project.canvasWidth, impl_->project.canvasHeight),
                impl_->project.fps, &error)) {
            QMessageBox::warning(this, tr("Stream"), error);
            QSignalBlocker blocker(impl_->streamButton);
            impl_->streamButton->setChecked(false);
            return;
        }
        impl_->streamClock.restart();
    } else {
        impl_->recorder->stopStreaming();
    }
    refreshStatus();
}

void MainWindow::onSettings()
{
    QSettings settings;
    const QString storedLanguage = settings.value("ui/language").toString();
    const theme::ThemeId originalTheme = theme::currentTheme();

    SettingsDialog dialog(this);
    dialog.setValues(impl_->project.canvasWidth, impl_->project.canvasHeight,
        impl_->project.fps, QString::fromStdString(impl_->project.outputs.recordingDirectory),
        QString::fromStdString(impl_->project.outputs.streamUrl),
        QString::fromStdString(impl_->project.outputs.streamKey));
    dialog.setLanguage(storedLanguage);
    dialog.setTheme(originalTheme);
    connect(&dialog, &SettingsDialog::themePreviewRequested, this,
        [this](theme::ThemeId id) { applyTheme(id); });

    if (dialog.exec() != QDialog::Accepted) {
        applyTheme(originalTheme);
        return;
    }

    const theme::ThemeId chosenTheme = dialog.theme();
    settings.setValue("ui/theme", theme::themeKey(chosenTheme));
    settings.setValue("ui/restoreSession", dialog.restoreSession());
    applyTheme(chosenTheme);

    const QString newLanguage = dialog.language();
    if (newLanguage != storedLanguage) {
        if (newLanguage.isEmpty()) {
            settings.remove("ui/language");
        } else {
            settings.setValue("ui/language", newLanguage);
        }
        QMessageBox::information(this, tr("Interface Language"),
            tr("The interface language will change the next time Mixvera starts."));
    }

    impl_->project.canvasWidth = dialog.canvasWidth();
    impl_->project.canvasHeight = dialog.canvasHeight();
    impl_->project.fps = dialog.fps();
    impl_->project.outputs.recordingDirectory = dialog.recordingDirectory().toStdString();
    impl_->project.outputs.streamUrl = dialog.streamUrl().toStdString();
    impl_->project.outputs.streamKey = dialog.streamKey().toStdString();

    impl_->compositor->setCanvasSize(QSize(impl_->project.canvasWidth,
        impl_->project.canvasHeight));
    impl_->compositor->setFrameRate(impl_->project.fps);
    if (impl_->tickTimer) {
        impl_->tickTimer->setInterval(1000 / std::max(1, impl_->project.fps));
    }

    refreshStatus();
    persistProject();
}

QString MainWindow::outputDirectory() const
{
    QString directory = QString::fromStdString(
        impl_->project.outputs.recordingDirectory);
    if (directory.isEmpty()) {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation))
            .filePath("Mixvera");
    }
    if (QDir::isRelativePath(directory)) {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation))
            .filePath(directory);
    }
    return directory;
}

QString MainWindow::nameOf(const std::string& id) const
{
    const auto* input = impl_->inputService->find(id);
    return input ? QString::fromStdString(input->name) : QString::fromStdString(id);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    persistProject();
    impl_->tickTimer->stop();
    impl_->secondTimer->stop();
    impl_->recorder->stopRecording();
    impl_->recorder->stopStreaming();
    impl_->meter->stop();
    QMainWindow::closeEvent(event);
}

} // namespace mixvera::ui