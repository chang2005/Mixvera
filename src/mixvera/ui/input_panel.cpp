#include "mixvera/ui/input_panel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

namespace mixvera::ui {

namespace {

constexpr int kRowHeight = 64;
constexpr int kThumbWidth = 96;
constexpr int kThumbHeight = 54;

class InputDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return QSize(160, kRowHeight);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        const QRect row = option.rect.adjusted(0, 0, 0, -1);
        const bool selected = option.state & QStyle::State_Selected;
        painter->fillRect(row, selected ? QColor("#284CB8") : QColor("#1D1F26"));

        const QRect thumb(row.left() + 6, row.top() + 5, kThumbWidth, kThumbHeight);
        painter->fillRect(thumb, QColor("#0B0D12"));
        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!icon.isNull()) {
            icon.paint(painter, thumb, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        } else {
            painter->setPen(QColor("#3C404D"));
            painter->drawRect(thumb.adjusted(0, 0, -1, -1));
            painter->setPen(QColor("#646464"));
            painter->setFont(QFont(QStringLiteral("Segoe UI"), 8));
            painter->drawText(thumb, Qt::AlignCenter, QStringLiteral("NO FRAME"));
        }

        const int textLeft = thumb.right() + 10;
        const int number = index.row() + 1;
        painter->setFont(QFont(QStringLiteral("Consolas"), 9, QFont::Bold));
        painter->setPen(selected ? QColor("#DCE4FF") : QColor("#8FA6E8"));
        painter->drawText(QRect(textLeft, row.top() + 6, 22, 16),
            Qt::AlignLeft | Qt::AlignVCenter, QString::number(number).rightJustified(2, '0'));

        const QString status = index.data(Qt::UserRole + 2).toString();
        const QString name = index.data(Qt::DisplayRole).toString();
        const QString kind = index.data(Qt::UserRole + 1).toString();

        QFont nameFont(QStringLiteral("Segoe UI"), 10, QFont::DemiBold);
        painter->setFont(nameFont);
        painter->setPen(QColor("#E9ECF2"));
        painter->drawText(QRect(textLeft + 24, row.top() + 4, row.width() - textLeft - 120, 18),
            Qt::AlignLeft | Qt::AlignVCenter,
            painter->fontMetrics().elidedText(name, Qt::ElideRight,
                row.width() - textLeft - 120));

        painter->setFont(QFont(QStringLiteral("Segoe UI"), 8));
        painter->setPen(selected ? QColor("#C9D3F5") : QColor("#969696"));
        painter->drawText(QRect(textLeft + 24, row.top() + 22, row.width() - textLeft - 120, 16),
            Qt::AlignLeft | Qt::AlignVCenter, kind);

        if (index.data(Qt::UserRole + 4).toBool()) {
            painter->setFont(QFont(QStringLiteral("Segoe UI"), 8, QFont::Bold));
            painter->setPen(QColor("#E85E75"));
            painter->drawText(QRect(textLeft + 24, row.top() + 40, 40, 14),
                Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("MUTE"));
        }

        if (!status.isEmpty()) {
            const bool program = status == "PGM";
            const QRect pill(row.right() - 74, row.top() + 8, 64, 16);
            painter->setPen(Qt::NoPen);
            painter->setBrush(program ? QColor("#C01C37") : QColor("#25A231"));
            painter->drawRoundedRect(pill, 2, 2);
            painter->setPen(QColor("#FFFFFF"));
            painter->setFont(QFont(QStringLiteral("Segoe UI"), 8, QFont::Bold));
            painter->drawText(pill, Qt::AlignCenter, status);
        }

        painter->restore();
    }
};

QLabel* dockTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("dockTitle");
    return label;
}

} // namespace

InputPanel::InputPanel(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("dockPanel");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(5);

    auto* header = new QHBoxLayout;
    header->setSpacing(4);
    header->addWidget(dockTitle(tr("INPUTS"), this));
    header->addStretch();
    auto* add = new QToolButton(this);
    add->setObjectName("dockButton");
    add->setText("+ " + tr("Add Input"));
    add->setToolTip(tr("Add a capture, media or generated input"));
    add->setCursor(Qt::PointingHandCursor);
    header->addWidget(add);
    root->addLayout(header);

    list_ = new QListWidget(this);
    list_->setObjectName("inputList");
    list_->setItemDelegate(new InputDelegate(list_));
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setUniformItemSizes(true);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(list_, 1);

    connect(add, &QToolButton::clicked, this, &InputPanel::addInputRequested);

    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            emit inputActivated(item->data(Qt::UserRole).toString());
        }
    });

    connect(list_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& point) {
        QListWidgetItem* item = list_->itemAt(point);
        QMenu menu(this);
        if (item) {
            const QString id = item->data(Qt::UserRole).toString();
            auto* preview = menu.addAction(tr("Set Preview"));
            auto* cut = menu.addAction(tr("Cut to Program"));
            menu.addSeparator();
            auto* addToScene = menu.addAction(tr("Add to Current Scene"));
            auto* properties = menu.addAction(tr("Input Properties…"));
            menu.addSeparator();
            auto* remove = menu.addAction(tr("Remove Input"));
            QAction* chosen = menu.exec(list_->mapToGlobal(point));
            if (chosen == preview) {
                emit previewRequested(id);
            } else if (chosen == cut) {
                emit cutRequested(id);
            } else if (chosen == addToScene) {
                emit addToSceneRequested(id);
            } else if (chosen == properties) {
                emit propertiesRequested(id);
            } else if (chosen == remove) {
                emit removeRequested(id);
            }
        } else {
            auto* addAction = menu.addAction(tr("Add Input…"));
            if (menu.exec(list_->mapToGlobal(point)) == addAction) {
                emit addInputRequested();
            }
        }
    });
}

void InputPanel::setInputs(const std::vector<core::InputDescriptor>& inputs)
{
    inputs_ = inputs;
    rebuild();
}

void InputPanel::rebuild()
{
    const QString current = list_->currentItem()
        ? list_->currentItem()->data(Qt::UserRole).toString()
        : QString();

    QSignalBlocker blocker(list_);
    list_->clear();
    for (const auto& input : inputs_) {
        const QString id = QString::fromStdString(input.id);
        QString status;
        if (input.id == programId_) {
            status = "PGM";
        } else if (input.id == previewId_) {
            status = "PVW";
        }

        auto* item = new QListWidgetItem(QString::fromStdString(input.name), list_);
        item->setData(Qt::UserRole, id);
        item->setData(Qt::UserRole + 1, QString::fromLatin1(core::inputKindLabel(input.kind)));
        item->setData(Qt::UserRole + 2, status);
        item->setData(Qt::UserRole + 3, input.hasAudio);
        item->setData(Qt::UserRole + 4, input.muted);
        item->setSizeHint(QSize(0, kRowHeight));
        if (thumbnails_.contains(id)) {
            item->setIcon(thumbnails_.value(id));
        }
        if (!current.isEmpty() && id == current) {
            list_->setCurrentItem(item);
        }
    }
    if (!list_->currentItem() && list_->count() > 0) {
        list_->setCurrentRow(0);
    }
}

void InputPanel::setThumbnail(const std::string& id, const QImage& image)
{
    const QString key = QString::fromStdString(id);
    if (image.isNull()) {
        return;
    }
    const QIcon icon(QPixmap::fromImage(image));
    if (thumbnails_.value(key).cacheKey() == icon.cacheKey()) {
        return;
    }
    thumbnails_.insert(key, icon);
    for (int row = 0; row < list_->count(); ++row) {
        auto* item = list_->item(row);
        if (item->data(Qt::UserRole).toString() == key) {
            item->setIcon(icon);
            break;
        }
    }
}

void InputPanel::setProgramPreview(const std::string& programId, const std::string& previewId)
{
    if (programId_ == programId && previewId_ == previewId) {
        return;
    }
    programId_ = programId;
    previewId_ = previewId;
    rebuild();
}

std::string InputPanel::selectedId() const
{
    const auto* item = list_->currentItem();
    return item ? item->data(Qt::UserRole).toString().toStdString() : std::string();
}

void InputPanel::selectInput(const std::string& id)
{
    for (int row = 0; row < list_->count(); ++row) {
        auto* item = list_->item(row);
        if (item->data(Qt::UserRole).toString().toStdString() == id) {
            list_->setCurrentItem(item);
            return;
        }
    }
}

} // namespace mixvera::ui