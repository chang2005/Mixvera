#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QFrame>
#include <QIcon>
#include <QMap>
#include <QString>

#include <string>
#include <vector>

class QListWidget;

namespace mixvera::ui {

/// vMix style input list: live thumbnails, numbering, PGM/PVW badges and a
/// context menu for the operations that matter during a show.
class InputPanel final : public QFrame {
    Q_OBJECT

public:
    explicit InputPanel(QWidget* parent = nullptr);

    void setInputs(const std::vector<core::InputDescriptor>& inputs);
    void setThumbnail(const std::string& id, const QImage& image);
    void setProgramPreview(const std::string& programId, const std::string& previewId);
    std::string selectedId() const;
    void selectInput(const std::string& id);

signals:
    void inputActivated(const QString& id);
    void previewRequested(const QString& id);
    void cutRequested(const QString& id);
    void addToSceneRequested(const QString& id);
    void removeRequested(const QString& id);
    void propertiesRequested(const QString& id);
    void addInputRequested();

private:
    void rebuild();

    QListWidget* list_ = nullptr;
    std::vector<core::InputDescriptor> inputs_;
    QMap<QString, QIcon> thumbnails_;
    std::string programId_;
    std::string previewId_;
};

} // namespace mixvera::ui