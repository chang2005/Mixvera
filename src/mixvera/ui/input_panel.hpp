#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QFrame>
#include <QImage>
#include <QMap>
#include <QString>

#include <string>
#include <vector>

class QLabel;
class QLineEdit;
class QScrollArea;
class QToolButton;
class QHBoxLayout;

namespace mixvera::ui {

class InputTile;

/// vMix style input bar: a horizontal strip of live thumbnails with coloured
/// PGM/PVW headers, filters, search and per-tile actions.
class InputPanel final : public QFrame {
    Q_OBJECT

public:
    explicit InputPanel(QWidget* parent = nullptr);

    void setInputs(const std::vector<core::InputDescriptor>& inputs);
    void setThumbnail(const std::string& id, const QImage& image);
    void setProgramPreview(const std::string& programId, const std::string& previewId);
    std::string selectedId() const;
    void selectInput(const std::string& id);
    int count() const;
    static QString kindLabel(core::InputKind kind);

signals:
    void inputActivated(const QString& id);
    void previewRequested(const QString& id);
    void cutRequested(const QString& id);
    void addToSceneRequested(const QString& id);
    void removeRequested(const QString& id);
    void propertiesRequested(const QString& id);
    void addInputRequested();

private:
    enum class Filter {
        All,
        Scenes,
        Capture,
        Media,
    };

    void rebuild();
    void applyFilter();
    void updateSelection();
    void showContextMenu(const QString& id, const QPoint& globalPos);
    bool matchesFilter(const core::InputDescriptor& input) const;

    QScrollArea* scroll_ = nullptr;
    QWidget* strip_ = nullptr;
    QHBoxLayout* stripLayout_ = nullptr;
    QLineEdit* search_ = nullptr;
    QLabel* countLabel_ = nullptr;
    std::vector<QToolButton*> filterButtons_;

    std::vector<core::InputDescriptor> inputs_;
    QMap<QString, QImage> thumbnails_;
    QMap<QString, InputTile*> tiles_;
    std::vector<InputTile*> tileOrder_;

    std::string programId_;
    std::string previewId_;
    std::string selectedId_;
    Filter filter_ = Filter::All;
};

} // namespace mixvera::ui
