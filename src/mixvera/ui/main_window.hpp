#pragma once

#include "mixvera/core/project.hpp"
#include "mixvera/ui/theme.hpp"

#include <QMainWindow>
#include <QPoint>

#include <memory>

class QLabel;
class QPushButton;

namespace mixvera::ui {

class StageView;
class TransitionBar;
class ScenePanel;
class InputPanel;
class MixerPanel;
class StatusBar;
class Compositor;
class Recorder;
class AudioMeter;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(bool restoreSession = true, QWidget* parent = nullptr);
    ~MainWindow() override;

    /// Loads a project file into the live session (used by --project).
    bool loadProject(const QString& path);

    /// Shows or hides the audio mixer dock (used by --show-mixer and presets).
    void setMixerVisible(bool visible);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildLayout();
    void buildMenus();
    void connectSignals();

    void applyProject(core::Project project);
    void persistProject();
    void schedulePersist();
    QString projectPath() const;

    void applyTheme(theme::ThemeId id);

    void refreshPanels();
    void refreshStage();
    void refreshThumbnails();
    void refreshStatus();
    void refreshMixer();

    void tick();
    void onCut();
    void onAuto();
    void onTbarMoved(float position);
    void onTbarReleased();
    void onFtbToggled(bool engaged);

    void onOverlayToggled(int layer);
    void showOverlayMenu(int layer, const QPoint& globalPos);
    void onOverlayAssigned(const QString& id, int layer);
    void onOverlayCleared(const QString& id);
    void setOverlay(int layer, const std::string& inputId, bool enabled);
    void refreshOverlayButtons();
    void onTitleAction(const QString& id, const QString& action);
    void onSnapshot();

    void onSceneSelected(const QString& id);
    void onSceneActivated(const QString& id);
    void onSceneAdd();
    void onSceneDuplicate();
    void onSceneRemove();
    void onSceneRenamed(const QString& id, const QString& name);
    void onSourceSelected(const QString& id);
    void onSourceAdd();
    void onSourceRemove();
    void onSourceMove(const QString& id, int delta);
    void onSourceVisibility(const QString& id, bool visible);

    void onAddInput();
    void onInputActivated(const QString& id);
    void onInputPreview(const QString& id);
    void onInputCut(const QString& id);
    void onInputAddToScene(const QString& id);
    void onInputRemove(const QString& id);
    void onInputProperties(const QString& id);

    void onRecordToggled(bool start);
    void onStreamToggled(bool start);
    void onSettings();

    void cutTo(const std::string& id);
    void previewInput(const std::string& id);
    void resetTransition();

    QString outputDirectory() const;
    QString nameOf(const std::string& id) const;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mixvera::ui