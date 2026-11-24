#include "mixvera/ui/main_window.hpp"

#include "mixvera/core/application.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/output_service.hpp"
#include "mixvera/services/replay_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/services/transition_service.hpp"
#include "mixvera/ui/mixer_panel.hpp"
#include "mixvera/ui/multiview_panel.hpp"
#include "mixvera/ui/program_view.hpp"
#include "mixvera/ui/preview_view.hpp"
#include "mixvera/ui/status_bar.hpp"
#include "mixvera/ui/tbar_widget.hpp"

#include <QAction>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace mixvera::ui {

struct MainWindow::Impl {
    ProgramView* program = nullptr;
    PreviewView* preview = nullptr;
    MultiviewPanel* multiview = nullptr;
    MixerPanel* mixer = nullptr;
    TBarWidget* tbar = nullptr;
    StatusBar* status = nullptr;
    QLabel* programLabel = nullptr;
    QLabel* previewLabel = nullptr;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , impl_(std::make_unique<Impl>())
{
    setWindowTitle("Mixvera");
    resize(1440, 900);
    buildMenuBar();
    buildCentralWidget();
    buildDocks();
    refreshStatus();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildMenuBar()
{
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* quitAction = fileMenu->addAction("E&xit");
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* toolsMenu = menuBar()->addMenu("&Tools");
    auto* replayAction = toolsMenu->addAction("Save Replay");
    connect(replayAction, &QAction::triggered, this, &MainWindow::onReplaySave);
}

void MainWindow::buildCentralWidget()
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    auto* stages = new QWidget(central);
    auto* stageLayout = new QHBoxLayout(stages);

    impl_->program = new ProgramView(stages);
    impl_->preview = new PreviewView(stages);
    stageLayout->addWidget(impl_->program, 3);
    stageLayout->addWidget(impl_->preview, 2);
    layout->addWidget(stages, 3);

    impl_->tbar = new TBarWidget(central);
    layout->addWidget(impl_->tbar, 1);

    auto* controls = new QWidget(central);
    auto* controlLayout = new QHBoxLayout(controls);
    auto* takeButton = new QPushButton("TAKE", controls);
    takeButton->setObjectName("takeButton");
    impl_->programLabel = new QLabel("PGM: --", controls);
    impl_->previewLabel = new QLabel("PVW: --", controls);
    controlLayout->addWidget(takeButton);
    controlLayout->addWidget(impl_->programLabel);
    controlLayout->addWidget(impl_->previewLabel);
    controlLayout->addStretch();
    layout->addWidget(controls);

    connect(takeButton, &QPushButton::clicked, this, &MainWindow::onTake);
    connect(impl_->tbar, &TBarWidget::positionChanged, this, &MainWindow::onTBarChanged);

    setCentralWidget(central);
}

void MainWindow::buildDocks()
{
    auto* multiviewDock = new QDockWidget("MultiView", this);
    impl_->multiview = new MultiviewPanel(multiviewDock);
    multiviewDock->setWidget(impl_->multiview);
    addDockWidget(Qt::RightDockWidgetArea, multiviewDock);

    auto* mixerDock = new QDockWidget("Audio Mixer", this);
    impl_->mixer = new MixerPanel(mixerDock);
    mixerDock->setWidget(impl_->mixer);
    addDockWidget(Qt::BottomDockWidgetArea, mixerDock);

    impl_->status = new StatusBar(this);
    setStatusBar(impl_->status);
}

void MainWindow::onTake()
{
    auto& app = core::Application::instance();
    app.services().get<services::SwitcherService>().cut();
    refreshStatus();
}

void MainWindow::onTBarChanged(int value)
{
    auto& app = core::Application::instance();
    auto& transition = app.services().get<services::TransitionService>();
    transition.setPosition(static_cast<float>(value) / 1023.0F);
    refreshStatus();
}

void MainWindow::onReplaySave()
{
    auto& app = core::Application::instance();
    app.services().get<services::ReplayService>().saveReplay();
}

void MainWindow::refreshStatus()
{
    auto& app = core::Application::instance();
    const auto& switcher = app.services().get<services::SwitcherService>().state();
    const auto& transition = app.services().get<services::TransitionService>().state();

    impl_->programLabel->setText(QString("PGM: %1").arg(QString::fromStdString(switcher.programInputId)));
    impl_->previewLabel->setText(QString("PVW: %1").arg(QString::fromStdString(switcher.previewInputId)));
    impl_->status->setTransition(transition.position);
}

} // namespace mixvera::ui
