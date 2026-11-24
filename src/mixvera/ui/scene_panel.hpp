#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QFrame>
#include <QString>

#include <map>
#include <string>
#include <vector>

class QListWidget;
class QToolButton;

namespace mixvera::ui {

/// OBS style left column: the scene list with the layers of the selected
/// scene below it.
class ScenePanel final : public QFrame {
    Q_OBJECT

public:
    explicit ScenePanel(QWidget* parent = nullptr);

    void setScenes(const std::vector<core::InputDescriptor>& scenes,
        const std::string& programId, const std::string& previewId);
    void setSources(const core::InputDescriptor* scene,
        const std::vector<core::InputDescriptor>& allInputs);

    std::string selectedScene() const;
    std::string selectedSource() const;
    void selectScene(const std::string& id);

signals:
    void sceneSelected(const QString& id);
    void sceneActivated(const QString& id);
    void sceneAddRequested();
    void sceneDuplicateRequested();
    void sceneRemoveRequested();
    void sceneRenamed(const QString& id, const QString& name);
    void sourceSelected(const QString& id);
    void sourceAddRequested();
    void sourceRemoveRequested();
    void sourceMoveRequested(const QString& id, int delta);
    void sourceVisibilityChanged(const QString& id, bool visible);

private:
    QToolButton* smallButton(const QString& text, const QString& tip);
    void rebuildScenes();
    void rebuildSources();

    QListWidget* sceneList_ = nullptr;
    QListWidget* sourceList_ = nullptr;
    std::vector<core::InputDescriptor> scenes_;
    const core::InputDescriptor* scene_ = nullptr;
    std::map<std::string, QString> nameLookup_;
    std::string programId_;
    std::string previewId_;
};

} // namespace mixvera::ui