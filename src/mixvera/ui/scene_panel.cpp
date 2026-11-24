#include "mixvera/ui/scene_panel.hpp"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

namespace mixvera::ui {

namespace {

QLabel* dockTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("dockTitle");
    return label;
}

} // namespace

ScenePanel::ScenePanel(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("dockPanel");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(5);

    auto* sceneHeader = new QHBoxLayout;
    sceneHeader->setSpacing(3);
    sceneHeader->addWidget(dockTitle(tr("SCENES"), this));
    sceneHeader->addStretch();
    auto* addScene = smallButton("+", tr("Add scene"));
    auto* duplicateScene = smallButton("⧉", tr("Duplicate scene"));
    auto* removeScene = smallButton("−", tr("Remove scene"));
    sceneHeader->addWidget(addScene);
    sceneHeader->addWidget(duplicateScene);
    sceneHeader->addWidget(removeScene);
    root->addLayout(sceneHeader);

    sceneList_ = new QListWidget(this);
    sceneList_->setObjectName("sceneList");
    sceneList_->setFixedHeight(148);
    sceneList_->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(sceneList_);

    auto* sourceHeader = new QHBoxLayout;
    sourceHeader->setSpacing(3);
    sourceHeader->addWidget(dockTitle(tr("SOURCES"), this));
    sourceHeader->addStretch();
    auto* addSource = smallButton("+", tr("Add source"));
    auto* removeSource = smallButton("−", tr("Remove source"));
    sourceHeader->addWidget(addSource);
    sourceHeader->addWidget(removeSource);
    root->addLayout(sourceHeader);

    sourceList_ = new QListWidget(this);
    sourceList_->setObjectName("sourceList");
    sourceList_->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(sourceList_, 1);

    connect(addScene, &QToolButton::clicked, this, &ScenePanel::sceneAddRequested);
    connect(duplicateScene, &QToolButton::clicked, this, &ScenePanel::sceneDuplicateRequested);
    connect(removeScene, &QToolButton::clicked, this, &ScenePanel::sceneRemoveRequested);
    connect(addSource, &QToolButton::clicked, this, &ScenePanel::sourceAddRequested);
    connect(removeSource, &QToolButton::clicked, this, &ScenePanel::sourceRemoveRequested);

    connect(sceneList_, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* item, QListWidgetItem*) {
            if (!item) {
                return;
            }
            emit sceneSelected(item->data(Qt::UserRole).toString());
        });

    connect(sceneList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            emit sceneActivated(item->data(Qt::UserRole).toString());
        }
    });

    connect(sceneList_, &QListWidget::customContextMenuRequested, this,
        [this](const QPoint& point) {
            QListWidgetItem* item = sceneList_->itemAt(point);
            if (!item) {
                return;
            }
            QMenu menu(this);
            auto* rename = menu.addAction(tr("Rename…"));
            auto* duplicate = menu.addAction(tr("Duplicate"));
            auto* remove = menu.addAction(tr("Remove"));
            QAction* chosen = menu.exec(sceneList_->mapToGlobal(point));
            if (chosen == rename) {
                bool accepted = false;
                const QString name = QInputDialog::getText(this, tr("Rename Scene"),
                    tr("Scene name"), QLineEdit::Normal, item->text(), &accepted);
                if (accepted && !name.trimmed().isEmpty()) {
                    emit sceneRenamed(item->data(Qt::UserRole).toString(), name.trimmed());
                }
            } else if (chosen == duplicate) {
                sceneList_->setCurrentItem(item);
                emit sceneDuplicateRequested();
            } else if (chosen == remove) {
                sceneList_->setCurrentItem(item);
                emit sceneRemoveRequested();
            }
        });

    connect(sourceList_, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* item, QListWidgetItem*) {
            if (item) {
                emit sourceSelected(item->data(Qt::UserRole).toString());
            }
        });

    connect(sourceList_, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (!item) {
            return;
        }
        emit sourceVisibilityChanged(item->data(Qt::UserRole).toString(),
            item->checkState() == Qt::Checked);
    });

    connect(sourceList_, &QListWidget::customContextMenuRequested, this,
        [this](const QPoint& point) {
            QListWidgetItem* item = sourceList_->itemAt(point);
            if (!item) {
                return;
            }
            const QString id = item->data(Qt::UserRole).toString();
            QMenu menu(this);
            auto* up = menu.addAction(tr("Move Up"));
            auto* down = menu.addAction(tr("Move Down"));
            menu.addSeparator();
            auto* remove = menu.addAction(tr("Remove from Scene"));
            QAction* chosen = menu.exec(sourceList_->mapToGlobal(point));
            if (chosen == up) {
                sourceList_->setCurrentItem(item);
                emit sourceMoveRequested(id, -1);
            } else if (chosen == down) {
                sourceList_->setCurrentItem(item);
                emit sourceMoveRequested(id, 1);
            } else if (chosen == remove) {
                sourceList_->setCurrentItem(item);
                emit sourceRemoveRequested();
            }
        });
}

QToolButton* ScenePanel::smallButton(const QString& text, const QString& tip)
{
    auto* button = new QToolButton(this);
    button->setObjectName("dockButton");
    button->setText(text);
    button->setToolTip(tip);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void ScenePanel::setScenes(const std::vector<core::InputDescriptor>& scenes,
    const std::string& programId, const std::string& previewId)
{
    scenes_ = scenes;
    programId_ = programId;
    previewId_ = previewId;
    rebuildScenes();
}

void ScenePanel::rebuildScenes()
{
    const QString current = sceneList_->currentItem()
        ? sceneList_->currentItem()->data(Qt::UserRole).toString()
        : QString();

    QSignalBlocker blocker(sceneList_);
    sceneList_->clear();
    for (const auto& scene : scenes_) {
        QString label = QString::fromStdString(scene.name);
        if (scene.id == programId_) {
            label.append(QStringLiteral("   [PGM]"));
        } else if (scene.id == previewId_) {
            label.append(QStringLiteral("   [PVW]"));
        }
        auto* item = new QListWidgetItem(label, sceneList_);
        item->setData(Qt::UserRole, QString::fromStdString(scene.id));
        if (!current.isEmpty() && item->data(Qt::UserRole).toString() == current) {
            sceneList_->setCurrentItem(item);
        }
    }

    if (!sceneList_->currentItem() && sceneList_->count() > 0) {
        sceneList_->setCurrentRow(0);
    }
}

void ScenePanel::setSources(const core::InputDescriptor* scene,
    const std::vector<core::InputDescriptor>& allInputs)
{
    scene_ = scene;
    nameLookup_.clear();
    for (const auto& input : allInputs) {
        nameLookup_.insert({input.id, QString::fromStdString(input.name)});
    }
    rebuildSources();
}

void ScenePanel::rebuildSources()
{
    QSignalBlocker blocker(sourceList_);
    sourceList_->clear();
    if (!scene_) {
        return;
    }
    for (const auto& item : scene_->items) {
        auto* row = new QListWidgetItem(sourceList_);
        QString name = QString::fromStdString(item.inputId);
        const auto lookup = nameLookup_.find(item.inputId);
        if (lookup != nameLookup_.end()) {
            name = lookup->second;
        }
        row->setText(name);
        row->setData(Qt::UserRole, QString::fromStdString(item.inputId));
        row->setFlags(row->flags() | Qt::ItemIsUserCheckable);
        row->setCheckState(item.visible ? Qt::Checked : Qt::Unchecked);
    }
}

std::string ScenePanel::selectedScene() const
{
    const auto* item = sceneList_ ? sceneList_->currentItem() : nullptr;
    return item ? item->data(Qt::UserRole).toString().toStdString() : std::string();
}

std::string ScenePanel::selectedSource() const
{
    const auto* item = sourceList_ ? sourceList_->currentItem() : nullptr;
    return item ? item->data(Qt::UserRole).toString().toStdString() : std::string();
}

void ScenePanel::selectScene(const std::string& id)
{
    QSignalBlocker blocker(sceneList_);
    for (int row = 0; row < sceneList_->count(); ++row) {
        auto* item = sceneList_->item(row);
        if (item->data(Qt::UserRole).toString().toStdString() == id) {
            sceneList_->setCurrentItem(item);
            return;
        }
    }
}

} // namespace mixvera::ui