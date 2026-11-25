#include "mixvera/ui/input_panel.hpp"

#include "mixvera/ui/media/chroma_key.hpp"

#include <QButtonGroup>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace mixvera::ui {

namespace {

constexpr int kTileWidth = 190;
constexpr int kTileHeight = 164;
constexpr int kThumbWidth = 182;
constexpr int kThumbHeight = 102;

QLabel* dockTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("dockTitle");
    return label;
}

} // namespace

class InputTile final : public QFrame {
    Q_OBJECT

public:
    InputTile(const core::InputDescriptor& input, int number, const QString& kind,
        QWidget* parent)
        : QFrame(parent)
        , id_(QString::fromStdString(input.id))
        , name_(QString::fromStdString(input.name))
        , kind_(kind)
    {
        setObjectName("inputTile");
        setProperty("selected", false);
        setFixedSize(kTileWidth, kTileHeight);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("%1 · %2").arg(name_, kind_));

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(3);

        header_ = new QLabel(this);
        header_->setObjectName("inputTileHeader");
        header_->setFixedHeight(22);
        header_->setContentsMargins(5, 0, 5, 0);
        header_->setText(QStringLiteral("%1  %2").arg(number).arg(
            header_->fontMetrics().elidedText(name_, Qt::ElideRight, 132)));
        layout->addWidget(header_);

        thumb_ = new QLabel(this);
        thumb_->setObjectName("inputTileThumb");
        thumb_->setFixedSize(kThumbWidth, kThumbHeight);
        thumb_->setAlignment(Qt::AlignCenter);
        layout->addWidget(thumb_);

        auto* actions = new QHBoxLayout;
        actions->setContentsMargins(0, 0, 0, 0);
        actions->setSpacing(3);
        auto* preview = actionButton(QStringLiteral("PVW"), tr("Set Preview"), 36);
        auto* cut = actionButton(QStringLiteral("PGM"), tr("Cut to Program"), 36);
        auto* addToScene = actionButton(QStringLiteral("+"), tr("Add to Current Scene"), 22);
        auto* properties = actionButton(QStringLiteral("⚙"), tr("Input Properties…"), 22);
        auto* remove = actionButton(QStringLiteral("×"), tr("Remove Input"), 20);
        actions->addWidget(preview);
        actions->addWidget(cut);
        actions->addWidget(addToScene);
        actions->addWidget(properties);
        actions->addWidget(remove);
        actions->addStretch(1);
        layout->addLayout(actions);

        connect(preview, &QToolButton::clicked, this,
            [this] { emit previewRequested(id_); });
        connect(cut, &QToolButton::clicked, this, [this] { emit cutRequested(id_); });
        connect(addToScene, &QToolButton::clicked, this,
            [this] { emit addToSceneRequested(id_); });
        connect(properties, &QToolButton::clicked, this,
            [this] { emit propertiesRequested(id_); });
        connect(remove, &QToolButton::clicked, this, [this] { emit removeRequested(id_); });

        setStatus(QString(), QString());
    }

    const QString& id() const { return id_; }

    void setThumbnail(const QImage& image)
    {
        image_ = image;
        if (image_.isNull()) {
            thumb_->clear();
            return;
        }
        thumb_->setPixmap(QPixmap::fromImage(image_.scaled(
            QSize(kThumbWidth, kThumbHeight), Qt::KeepAspectRatio,
            Qt::SmoothTransformation)));
    }

    void setStatus(const QString& programId, const QString& previewId)
    {
        const char* role = id_ == programId ? "program"
            : (id_ == previewId ? "preview" : "idle");
        header_->setProperty("role", QString::fromLatin1(role));
        header_->style()->unpolish(header_);
        header_->style()->polish(header_);
    }

    void setSelected(bool selected)
    {
        if (property("selected").toBool() == selected) {
            return;
        }
        setProperty("selected", selected);
        style()->unpolish(this);
        style()->polish(this);
    }

signals:
    void clicked(const QString& id);
    void previewRequested(const QString& id);
    void cutRequested(const QString& id);
    void addToSceneRequested(const QString& id);
    void propertiesRequested(const QString& id);
    void removeRequested(const QString& id);
    void contextMenuRequested(const QString& id, const QPoint& globalPos);

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked(id_);
        }
        QFrame::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit propertiesRequested(id_);
        }
        QFrame::mouseDoubleClickEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        emit contextMenuRequested(id_, event->globalPos());
        event->accept();
    }

private:
    QToolButton* actionButton(const QString& text, const QString& tip, int width)
    {
        auto* button = new QToolButton(this);
        button->setObjectName("tileAction");
        button->setText(text);
        button->setToolTip(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setAutoRaise(true);
        button->setFixedSize(width, 22);
        return button;
    }

    QString id_;
    QString name_;
    QString kind_;
    QLabel* header_ = nullptr;
    QLabel* thumb_ = nullptr;
    QImage image_;
};

InputPanel::InputPanel(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("inputBar");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(4);

    auto* header = new QHBoxLayout;
    header->setSpacing(4);
    header->addWidget(dockTitle(tr("INPUTS"), this));
    header->addSpacing(6);

    auto* filters = new QButtonGroup(this);
    filters->setExclusive(true);
    struct FilterDef {
        QString text;
        Filter filter;
    };
    const FilterDef filterDefs[] = {
        {tr("All"), Filter::All},
        {tr("Scenes"), Filter::Scenes},
        {tr("Capture"), Filter::Capture},
        {tr("Media"), Filter::Media},
    };
    for (const auto& def : filterDefs) {
        auto* button = new QToolButton(this);
        button->setObjectName("filterChip");
        button->setText(def.text);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("filter", static_cast<int>(def.filter));
        filters->addButton(button);
        filterButtons_.push_back(button);
        header->addWidget(button);
        if (def.filter == Filter::All) {
            button->setChecked(true);
        }
    }
    // QButtonGroup::idClicked gives an index; map it back to the stored filter.
    connect(filters, &QButtonGroup::idClicked, this, [this, filters](int id) {
        auto* button = filters->button(id);
        if (!button) {
            return;
        }
        filter_ = static_cast<Filter>(button->property("filter").toInt());
        applyFilter();
    });

    header->addStretch();

    search_ = new QLineEdit(this);
    search_->setObjectName("inputSearch");
    search_->setPlaceholderText(tr("Search inputs…"));
    search_->setClearButtonEnabled(true);
    search_->setFixedWidth(150);
    header->addWidget(search_);

    countLabel_ = new QLabel(this);
    countLabel_->setObjectName("inputCount");
    header->addWidget(countLabel_);

    auto* add = new QToolButton(this);
    add->setObjectName("dockButton");
    add->setText(QStringLiteral("+ ") + tr("Add Input"));
    add->setToolTip(tr("Add a capture, media or generated input"));
    add->setCursor(Qt::PointingHandCursor);
    header->addWidget(add);
    root->addLayout(header);

    scroll_ = new QScrollArea(this);
    scroll_->setObjectName("inputScroll");
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_->setMinimumHeight(kTileHeight + 14);

    strip_ = new QWidget(scroll_);
    strip_->setObjectName("inputStrip");
    stripLayout_ = new QHBoxLayout(strip_);
    stripLayout_->setContentsMargins(0, 0, 0, 0);
    stripLayout_->setSpacing(6);
    stripLayout_->addStretch(1);
    scroll_->setWidget(strip_);
    root->addWidget(scroll_, 1);

    connect(add, &QToolButton::clicked, this, &InputPanel::addInputRequested);
    connect(search_, &QLineEdit::textChanged, this, [this] { applyFilter(); });
}

void InputPanel::setInputs(const std::vector<core::InputDescriptor>& inputs)
{
    inputs_ = inputs;
    rebuild();
}

void InputPanel::rebuild()
{
    while (QLayoutItem* item = stripLayout_->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    tiles_.clear();
    tileOrder_.clear();

    int number = 1;
    for (const auto& input : inputs_) {
        const QString id = QString::fromStdString(input.id);
        auto* tile = new InputTile(input, number++, kindLabel(input.kind), strip_);
        if (thumbnails_.contains(id)) {
            tile->setThumbnail(thumbnails_.value(id));
        }
        tile->setStatus(QString::fromStdString(programId_),
            QString::fromStdString(previewId_));
        stripLayout_->addWidget(tile);
        tiles_.insert(id, tile);
        tileOrder_.push_back(tile);

        connect(tile, &InputTile::clicked, this, [this](const QString& inputId) {
            selectedId_ = inputId.toStdString();
            updateSelection();
            emit previewRequested(inputId);
        });
        connect(tile, &InputTile::previewRequested, this, &InputPanel::previewRequested);
        connect(tile, &InputTile::cutRequested, this, &InputPanel::cutRequested);
        connect(tile, &InputTile::addToSceneRequested, this,
            &InputPanel::addToSceneRequested);
        connect(tile, &InputTile::propertiesRequested, this,
            &InputPanel::propertiesRequested);
        connect(tile, &InputTile::removeRequested, this, &InputPanel::removeRequested);
        connect(tile, &InputTile::contextMenuRequested, this,
            [this](const QString& inputId, const QPoint& globalPos) {
                showContextMenu(inputId, globalPos);
            });
    }

    if (inputs_.empty()) {
        auto* empty = new QLabel(tr("No inputs yet. Use Add Input to get started."), strip_);
        empty->setObjectName("inputEmpty");
        stripLayout_->addWidget(empty);
    }
    stripLayout_->addStretch(1);

    if (!selectedId_.empty() && !tiles_.contains(QString::fromStdString(selectedId_))) {
        selectedId_.clear();
    }
    updateSelection();
    applyFilter();
}

void InputPanel::applyFilter()
{
    const QString query = search_ ? search_->text().trimmed() : QString();
    int visible = 0;
    for (const auto& input : inputs_) {
        auto* tile = tiles_.value(QString::fromStdString(input.id), nullptr);
        if (!tile) {
            continue;
        }
        const bool textMatch = query.isEmpty()
            || QString::fromStdString(input.name).contains(query, Qt::CaseInsensitive);
        const bool show = textMatch && matchesFilter(input);
        tile->setVisible(show);
        if (show) {
            ++visible;
        }
    }
    if (countLabel_) {
        countLabel_->setText(tr("%1 inputs").arg(visible));
    }
}

bool InputPanel::matchesFilter(const core::InputDescriptor& input) const
{
    switch (filter_) {
    case Filter::All:
        return true;
    case Filter::Scenes:
        return input.kind == core::InputKind::Scene;
    case Filter::Capture:
        return input.kind == core::InputKind::ScreenCapture
            || input.kind == core::InputKind::WindowCapture
            || input.kind == core::InputKind::Camera;
    case Filter::Media:
        return input.kind == core::InputKind::Image
            || input.kind == core::InputKind::MediaFile
            || input.kind == core::InputKind::Title
            || input.kind == core::InputKind::Color
            || input.kind == core::InputKind::TestPattern;
    }
    return true;
}

void InputPanel::setThumbnail(const std::string& id, const QImage& image)
{
    const QString key = QString::fromStdString(id);
    if (image.isNull()) {
        return;
    }
    thumbnails_.insert(key, image);
    if (auto* tile = tiles_.value(key, nullptr)) {
        tile->setThumbnail(image);
    }
}

void InputPanel::setProgramPreview(const std::string& programId, const std::string& previewId)
{
    if (programId_ == programId && previewId_ == previewId) {
        return;
    }
    programId_ = programId;
    previewId_ = previewId;
    for (auto* tile : tileOrder_) {
        tile->setStatus(QString::fromStdString(programId_), QString::fromStdString(previewId_));
    }
}

std::string InputPanel::selectedId() const
{
    return selectedId_;
}

void InputPanel::selectInput(const std::string& id)
{
    selectedId_ = id;
    updateSelection();
}

int InputPanel::count() const
{
    return static_cast<int>(inputs_.size());
}

void InputPanel::updateSelection()
{
    for (auto* tile : tileOrder_) {
        tile->setSelected(tile->id() == QString::fromStdString(selectedId_));
    }
}

void InputPanel::showContextMenu(const QString& id, const QPoint& globalPos)
{
    QMenu menu(this);
    QAction* preview = nullptr;
    QAction* cut = nullptr;
    QAction* addToScene = nullptr;
    QAction* properties = nullptr;
    QAction* remove = nullptr;
    QAction* overlayNone = nullptr;
    QList<QAction*> overlayLayers;
    QAction* chromaOff = nullptr;
    QAction* chromaGreen = nullptr;
    QAction* chromaBlue = nullptr;
    QAction* titleStart = nullptr;
    QAction* titlePause = nullptr;
    QAction* titleReset = nullptr;
    QAction* homePlus = nullptr;
    QAction* homeMinus = nullptr;
    QAction* awayPlus = nullptr;
    QAction* awayMinus = nullptr;
    QAction* captionSet = nullptr;
    QAction* captionNext = nullptr;
    QAction* captionPrev = nullptr;
    QAction* captionClear = nullptr;
    QAction* titleElements = nullptr;
    QAction* presetSave = nullptr;
    QAction* presetNext = nullptr;
    QAction* presetPrev = nullptr;
    QAction* presetClear = nullptr;

    if (!id.isEmpty()) {
        preview = menu.addAction(tr("Set Preview"));
        cut = menu.addAction(tr("Cut to Program"));
        menu.addSeparator();
        addToScene = menu.addAction(tr("Add to Current Scene"));

        QMenu* overlay = menu.addMenu(tr("Overlay"));
        overlayNone = overlay->addAction(tr("Not in an overlay"));
        overlay->addSeparator();
        for (int layer = 1; layer <= 4; ++layer) {
            overlayLayers.append(overlay->addAction(tr("Assign to Overlay %1").arg(layer)));
        }

        const core::InputDescriptor* input = nullptr;
        for (const auto& candidate : inputs_) {
            if (candidate.id == id.toStdString()) {
                input = &candidate;
                break;
            }
        }
        if (input && inputSupportsChromaKey(input->kind)) {
            const ChromaKeySettings settings = chromaKeySettings(*input);
            QMenu* chroma = menu.addMenu(tr("Chroma Key"));
            chromaOff = chroma->addAction(tr("Off"));
            chromaGreen = chroma->addAction(tr("Green Screen"));
            chromaBlue = chroma->addAction(tr("Blue Screen"));
            for (QAction* action : {chromaOff, chromaGreen, chromaBlue}) {
                action->setCheckable(true);
            }
            if (settings.enabled) {
                if (settings.color.green() > settings.color.red() + 60
                    && settings.color.green() > settings.color.blue() + 60) {
                    chromaGreen->setChecked(true);
                } else if (settings.color.blue() > settings.color.red() + 60
                    && settings.color.blue() > settings.color.green() + 60) {
                    chromaBlue->setChecked(true);
                }
            } else {
                chromaOff->setChecked(true);
            }
        }
        if (input && input->kind == core::InputKind::Title) {
            QMenu* title = menu.addMenu(tr("Title Controls"));
            titleStart = title->addAction(tr("Start / Resume Countdown"));
            titlePause = title->addAction(tr("Pause Countdown"));
            titleReset = title->addAction(tr("Reset Countdown"));
            title->addSeparator();
            homePlus = title->addAction(tr("Home Score +1"));
            homeMinus = title->addAction(tr("Home Score −1"));
            awayPlus = title->addAction(tr("Away Score +1"));
            awayMinus = title->addAction(tr("Away Score −1"));

            const auto styleIt = input->params.find("style");
            const QString style = styleIt == input->params.end()
                ? QString()
                : QString::fromStdString(styleIt->second);
            if (style == "package") {
                title->addSeparator();
                titleElements = title->addAction(tr("Edit Title Elements…"));
                presetSave = title->addAction(tr("Save Title Preset…"));
                presetPrev = title->addAction(tr("Previous Title Preset"));
                presetNext = title->addAction(tr("Next Title Preset"));
                presetClear = title->addAction(tr("Delete Title Presets"));
            }
            if (style == "caption" || style == "ticker" || style == "package") {
                const bool package = style == "package";
                title->addSeparator();
                captionSet = title->addAction(package ? tr("Set Title Text…")
                                                      : tr("Set Subtitle Text…"));
                captionPrev = title->addAction(package ? tr("Previous Caption")
                                                       : tr("Previous Subtitle"));
                captionNext = title->addAction(package ? tr("Next Caption")
                                                       : tr("Next Subtitle"));
                captionClear = title->addAction(package ? tr("Clear Title Text")
                                                        : tr("Clear Subtitle"));
            }
        }

        properties = menu.addAction(tr("Input Properties…"));
        menu.addSeparator();
        remove = menu.addAction(tr("Remove Input"));
    } else {
        preview = menu.addAction(tr("Add Input…"));
    }
    QAction* chosen = menu.exec(globalPos);
    if (!chosen) {
        return;
    }
    if (chosen == preview) {
        if (id.isEmpty()) {
            emit addInputRequested();
        } else {
            emit previewRequested(id);
        }
    } else if (chosen == cut) {
        emit cutRequested(id);
    } else if (chosen == addToScene) {
        emit addToSceneRequested(id);
    } else if (overlayNone && chosen == overlayNone) {
        emit overlayClearRequested(id);
    } else if (overlayLayers.contains(chosen)) {
        emit overlayAssignRequested(id, overlayLayers.indexOf(chosen) + 1);
    } else if (chosen == chromaOff) {
        emit chromaKeyRequested(id, "off");
    } else if (chosen == chromaGreen) {
        emit chromaKeyRequested(id, "green");
    } else if (chosen == chromaBlue) {
        emit chromaKeyRequested(id, "blue");
    } else if (chosen == titleStart) {
        emit titleActionRequested(id, "countdownStart");
    } else if (chosen == titlePause) {
        emit titleActionRequested(id, "countdownPause");
    } else if (chosen == titleReset) {
        emit titleActionRequested(id, "countdownReset");
    } else if (chosen == homePlus) {
        emit titleActionRequested(id, "homePlus");
    } else if (chosen == homeMinus) {
        emit titleActionRequested(id, "homeMinus");
    } else if (chosen == awayPlus) {
        emit titleActionRequested(id, "awayPlus");
    } else if (chosen == awayMinus) {
        emit titleActionRequested(id, "awayMinus");
    } else if (chosen == captionSet) {
        emit titleActionRequested(id, "captionSet");
    } else if (chosen == captionNext) {
        emit titleActionRequested(id, "captionNext");
    } else if (chosen == captionPrev) {
        emit titleActionRequested(id, "captionPrev");
    } else if (chosen == captionClear) {
        emit titleActionRequested(id, "captionClear");
    } else if (chosen == titleElements) {
        emit titleActionRequested(id, "elements");
    } else if (chosen == presetSave) {
        emit titleActionRequested(id, "presetSave");
    } else if (chosen == presetNext) {
        emit titleActionRequested(id, "presetNext");
    } else if (chosen == presetPrev) {
        emit titleActionRequested(id, "presetPrev");
    } else if (chosen == presetClear) {
        emit titleActionRequested(id, "presetClear");
    } else if (chosen == properties) {
        emit propertiesRequested(id);
    } else if (chosen == remove) {
        emit removeRequested(id);
    }
}

QString InputPanel::kindLabel(core::InputKind kind)
{
    switch (kind) {
    case core::InputKind::Scene:
        return tr("Scene");
    case core::InputKind::TestPattern:
        return tr("Test Pattern");
    case core::InputKind::ScreenCapture:
        return tr("Screen Capture");
    case core::InputKind::WindowCapture:
        return tr("Window Capture");
    case core::InputKind::Camera:
        return tr("Camera");
    case core::InputKind::Image:
        return tr("Image");
    case core::InputKind::MediaFile:
        return tr("Media File");
    case core::InputKind::Title:
        return tr("Title");
    case core::InputKind::Color:
        return tr("Colour");
    default:
        return tr("Input");
    }
}

} // namespace mixvera::ui

#include "input_panel.moc"
