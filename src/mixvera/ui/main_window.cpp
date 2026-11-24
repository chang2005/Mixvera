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

#include <QApplication>
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
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

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

    QLabel* projectLabel = nullptr;
    QLabel* recPill = nullptr;
    QLabel* streamPill = nullptr;
    QPushButton* recordButton = nullptr;
    QPushButton* streamButton = nullptr;

    QTimer* tickTimer = nullptr;
    QTimer* secondTimer = nullptr;

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
    setWindowTitle(tr("Mixvera 0.2 — Live Production Switcher"));
    resize(1600, 960);
    setMinimumSize(1180, 700);
    setStyleSheet(theme::applicationStyleSheet());

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

    if (impl_->meter->start()) {
        impl_->mixer->setAudioDeviceStatus(
            tr("MIC · %1").arg(impl_->meter->deviceName()), true);
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

void MainWindow::buildLayout()
{
    auto* root = new QWidget(this);
    root->setObjectName("root");
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* top = new QFrame(root);
    top->setObjectName("topBar");
    top->setFixedHeight(54);
    auto* topLayout = new QHBoxLayout(top);
    topLayout->setContentsMargins(12, 6, 12, 6);
    topLayout->setSpacing(10);

    auto* mark = makeLabel("M", "brandMark", top);
    mark->setFixedSize(32, 32);
    mark->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(mark);

    auto* titles = new QVBoxLayout;
    titles->setSpacing(0);
    titles->addWidget(makeLabel("MIXVERA", "brandTitle", top));
    titles->addWidget(makeLabel("LIVE PRODUCTION SWITCHER", "brandSubtitle", top));
    topLayout->addLayout(titles);

    impl_->projectLabel = makeLabel("", "projectName", top);
    topLayout->addSpacing(12);
    topLayout->addWidget(impl_->projectLabel);
    topLayout->addStretch();

    auto* addInput = new QPushButton(tr("Add Input"), top);
    addInput->setToolTip(tr("Add a capture, media or generated input"));
    auto* settings = new QPushButton(tr("Settings"), top);
    auto* fullscreen = new QPushButton(tr("Fullscreen"), top);
    topLayout->addWidget(addInput);
    topLayout->addWidget(settings);
    topLayout->addWidget(fullscreen);
    topLayout->addSpacing(10);

    impl_->recPill = makeLabel(tr("REC 00:00"), "pillRec", top);
    impl_->streamPill = makeLabel(tr("STREAM 00:00"), "pillStream", top);
    topLayout->addWidget(impl_->recPill);
    topLayout->addWidget(impl_->streamPill);

    impl_->recordButton = new QPushButton(tr("RECORD"), top);
    impl_->recordButton->setObjectName("recordButton");
    impl_->recordButton->setCheckable(true);
    impl_->recordButton->setToolTip(tr("Start / stop recording to disk (Ctrl+R)"));

    impl_->streamButton = new QPushButton(tr("STREAM"), top);
    impl_->streamButton->setObjectName("streamButton");
    impl_->streamButton->setCheckable(true);
    impl_->streamButton->setToolTip(tr("Start / stop the RTMP push"));

    topLayout->addWidget(impl_->recordButton);
    topLayout->addWidget(impl_->streamButton);
    layout->addWidget(top);

    auto* splitter = new QSplitter(Qt::Horizontal, root);
    splitter->setChildrenCollapsible(false);

    impl_->scenes = new ScenePanel(splitter);
    impl_->scenes->setMinimumWidth(210);
    impl_->scenes->setMaximumWidth(340);

    auto* center = new QWidget(splitter);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(6, 6, 6, 6);
    centerLayout->setSpacing(6);

    auto* monitors = new QHBoxLayout;
    monitors->setSpacing(6);

    auto* programFrame = new QFrame(center);
    programFrame->setObjectName("monitorFrame");
    programFrame->setProperty("role", "program");
    auto* programLayout = new QVBoxLayout(programFrame);
    programLayout->setContentsMargins(3, 3, 3, 3);
    impl_->program = new StageView(StageView::Role::Program, programFrame);
    programLayout->addWidget(impl_->program);

    auto* previewFrame = new QFrame(center);
    previewFrame->setObjectName("monitorFrame");
    previewFrame->setProperty("role", "preview");
    auto* previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(3, 3, 3, 3);
    impl_->preview = new StageView(StageView::Role::Preview, previewFrame);
    previewLayout->addWidget(impl_->preview);

    monitors->addWidget(programFrame, 2);
    monitors->addWidget(previewFrame, 1);
    centerLayout->addLayout(monitors, 1);

    impl_->transitionBar = new TransitionBar(center);
    centerLayout->addWidget(impl_->transitionBar);

    impl_->mixer = new MixerPanel(center);
    impl_->mixer->setFixedHeight(148);
    centerLayout->addWidget(impl_->mixer);

    impl_->inputsPanel = new InputPanel(splitter);
    impl_->inputsPanel->setMinimumWidth(250);
    impl_->inputsPanel->setMaximumWidth(360);

    splitter->addWidget(impl_->scenes);
    splitter->addWidget(center);
    splitter->addWidget(impl_->inputsPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({245, 1040, 310});
    layout->addWidget(splitter, 1);

    setCentralWidget(root);

    impl_->status = new StatusBar(this);
    setStatusBar(impl_->status);

    connect(addInput, &QPushButton::clicked, this, &MainWindow::onAddInput);
    connect(settings, &QPushButton::clicked, this, &MainWindow::onSettings);
    connect(fullscreen, &QPushButton::clicked, this, [this] {
        isFullScreen() ? showNormal() : showFullScreen();
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

    auto* help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About Mixvera"), this, [this] {
        QMessageBox::about(this, tr("About Mixvera"),
            tr("<h3>Mixvera 0.2</h3>"
               "<p>Live production switcher: scene based compositing, real capture "
               "inputs, CUT / AUTO / T-Bar transitions and ffmpeg recording.</p>"
               "<p>Interface language follows OBS Studio (dark docks, scenes and sources) "
               "and vMix (PGM/PVW monitors, input thumbnails, transition cluster).</p>"));
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
    connect(impl_->inputsPanel, &InputPanel::removeRequested, this, &MainWindow::onInputRemove);
    connect(impl_->inputsPanel, &InputPanel::propertiesRequested, this,
        &MainWindow::onInputProperties);

    connect(impl_->mixer, &MixerPanel::gainChanged, this,
        [this](const QString& id, float gain) {
            impl_->audio->setGain(id.toStdString(), gain);
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
            persistProject();
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

    impl_->compositor->setCanvasSize(QSize(impl_->project.canvasWidth,
        impl_->project.canvasHeight));
    impl_->compositor->setFrameRate(impl_->project.fps);

    impl_->inputService->clear();
    impl_->inputService->load(impl_->project.inputs);

    impl_->audio->clear();
    for (const auto& input : impl_->inputService->inputs()) {
        if (input.hasAudio) {
            impl_->audio->addChannel(input.id);
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

void MainWindow::persistProject()
{
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
    const QImage previewFrame = impl_->compositor->renderInput(all, state.previewInputId);

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

    if (impl_->ftb) {
        programFrame = impl_->compositor->fadeToBlack(programFrame, 1.0F);
    }

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
    copy.name = current->name + " copy";
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
        descriptor.name = core::inputKindLabel(descriptor.kind);
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
    SettingsDialog dialog(this);
    dialog.setValues(impl_->project.canvasWidth, impl_->project.canvasHeight,
        impl_->project.fps, QString::fromStdString(impl_->project.outputs.recordingDirectory),
        QString::fromStdString(impl_->project.outputs.streamUrl),
        QString::fromStdString(impl_->project.outputs.streamKey));
    if (dialog.exec() != QDialog::Accepted) {
        return;
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