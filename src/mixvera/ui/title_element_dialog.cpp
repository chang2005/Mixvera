#include "mixvera/ui/title_element_dialog.hpp"

#include <QColorDialog>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace mixvera::ui {

namespace {

QString elementTypeLabel(const QString& type)
{
    if (type == QLatin1String("text")) {
        return QCoreApplication::translate("TitleElementDialog", "Text");
    }
    if (type == QLatin1String("colour")) {
        return QCoreApplication::translate("TitleElementDialog", "Colour");
    }
    if (type == QLatin1String("image")) {
        return QCoreApplication::translate("TitleElementDialog", "Image");
    }
    if (type == QLatin1String("ticker")) {
        return QCoreApplication::translate("TitleElementDialog", "Ticker");
    }
    if (type == QLatin1String("qr")) {
        return QCoreApplication::translate("TitleElementDialog", "QR Code");
    }
    return type;
}

} // namespace

TitleElementDialog::TitleElementDialog(const TitlePackage& package,
    const QHash<QString, QString>& textValues,
    const QHash<QString, QColor>& colourValues,
    const QHash<QString, QString>& imageValues, QSize canvas, QWidget* parent)
    : QDialog(parent)
    , package_(package)
    , texts_(textValues)
    , colours_(colourValues)
    , images_(imageValues)
    , canvas_(canvas)
{
    setWindowTitle(tr("Title Elements"));
    setModal(true);
    resize(940, 620);
    elements_ = package_.elements();
    previewClock_.start();
    buildUi();
    refreshList();
    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
    previewTimer_ = new QTimer(this);
    connect(previewTimer_, &QTimer::timeout, this, [this] { updatePreview(); });
    previewTimer_->start(120);
    updatePreview();
}

void TitleElementDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    const QSize design = canvas_.isValid() ? canvas_ : package_.designSize();
    auto* header = new QLabel(tr("Title package: %1  ·  %2x%3")
                                  .arg(package_.name())
                                  .arg(design.width())
                                  .arg(design.height()),
        this);
    header->setObjectName("cardTitle");
    root->addWidget(header);

    auto* body = new QHBoxLayout;
    body->setSpacing(12);

    list_ = new QListWidget(this);
    list_->setObjectName("titleElementList");
    list_->setMinimumWidth(230);
    list_->setMaximumWidth(310);
    body->addWidget(list_);

    auto* right = new QVBoxLayout;
    right->setSpacing(8);

    preview_ = new QLabel(this);
    preview_->setObjectName("titleElementPreview");
    preview_->setMinimumSize(480, 270);
    preview_->setAlignment(Qt::AlignCenter);
    preview_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    preview_->setStyleSheet(
        QStringLiteral("background-color:#0b0d12; border:1px solid #2b3240;"));
    right->addWidget(preview_, 2);

    auto* properties = new QWidget(this);
    auto* propertyLayout = new QVBoxLayout(properties);
    propertyLayout->setContentsMargins(0, 0, 0, 0);
    propertyLayout->setSpacing(6);

    nameLabel_ = new QLabel(tr("Select an element"), properties);
    nameLabel_->setStyleSheet(QStringLiteral("font-weight:600; font-size:15px;"));
    propertyLayout->addWidget(nameLabel_);
    typeLabel_ = new QLabel(properties);
    typeLabel_->setStyleSheet(QStringLiteral("color:#9aa6b5;"));
    propertyLayout->addWidget(typeLabel_);

    pages_ = new QStackedWidget(properties);

    auto* textPage = new QWidget(pages_);
    auto* textLayout = new QVBoxLayout(textPage);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);
    textEdit_ = new QPlainTextEdit(textPage);
    textEdit_->setObjectName("titleElementText");
    textEdit_->setFixedHeight(92);
    textLayout->addWidget(textEdit_);
    propertyHint_ = new QLabel(textPage);
    propertyHint_->setWordWrap(true);
    propertyHint_->setStyleSheet(QStringLiteral("color:#9aa6b5;"));
    textLayout->addWidget(propertyHint_);
    pages_->addWidget(textPage);

    auto* colourPage = new QWidget(pages_);
    auto* colourLayout = new QHBoxLayout(colourPage);
    colourLayout->setContentsMargins(0, 0, 0, 0);
    colourLayout->setSpacing(8);
    colourButton_ = new QPushButton(tr("Choose Colour…"), colourPage);
    colourButton_->setObjectName("titleElementColour");
    colourValue_ = new QLabel(colourPage);
    colourValue_->setMinimumWidth(80);
    colourReset_ = new QPushButton(tr("Use Package Colour"), colourPage);
    colourLayout->addWidget(colourButton_);
    colourLayout->addWidget(colourValue_);
    colourLayout->addWidget(colourReset_);
    colourLayout->addStretch(1);
    pages_->addWidget(colourPage);

    auto* imagePage = new QWidget(pages_);
    auto* imageLayout = new QHBoxLayout(imagePage);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->setSpacing(6);
    imagePath_ = new QLineEdit(imagePage);
    imagePath_->setObjectName("titleElementImage");
    imagePath_->setPlaceholderText(tr("Use the image stored in the package"));
    imageBrowse_ = new QPushButton(tr("Browse…"), imagePage);
    imageReset_ = new QPushButton(tr("Use Package Image"), imagePage);
    imageLayout->addWidget(imagePath_, 1);
    imageLayout->addWidget(imageBrowse_);
    imageLayout->addWidget(imageReset_);
    pages_->addWidget(imagePage);

    propertyLayout->addWidget(pages_);
    right->addWidget(properties, 1);
    body->addLayout(right, 1);
    root->addLayout(body, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    connect(list_, &QListWidget::currentRowChanged, this, &TitleElementDialog::selectElement);
    connect(textEdit_, &QPlainTextEdit::textChanged, this, &TitleElementDialog::editText);
    connect(colourButton_, &QPushButton::clicked, this, &TitleElementDialog::chooseColour);
    connect(colourReset_, &QPushButton::clicked, this, &TitleElementDialog::resetColour);
    connect(imagePath_, &QLineEdit::textChanged, this, &TitleElementDialog::editImagePath);
    connect(imageBrowse_, &QPushButton::clicked, this, &TitleElementDialog::browseImage);
    connect(imageReset_, &QPushButton::clicked, this, &TitleElementDialog::resetImage);
}

void TitleElementDialog::refreshList()
{
    const QSignalBlocker blocker(list_);
    list_->clear();
    for (int index = 0; index < elements_.size(); ++index) {
        const auto& element = elements_.at(index);
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  ·  %2").arg(element.name, elementTypeLabel(element.type)),
            list_);
        item->setData(Qt::UserRole, index);
        item->setToolTip(element.value);
    }
}

void TitleElementDialog::selectElement(int row)
{
    currentRow_ = row;
    if (row < 0 || row >= elements_.size()) {
        updating_ = true;
        nameLabel_->setText(tr("No editable elements"));
        typeLabel_->clear();
        pages_->setEnabled(false);
        updating_ = false;
        return;
    }
    const auto& element = elements_.at(row);
    updating_ = true;
    pages_->setEnabled(true);
    nameLabel_->setText(element.name);
    typeLabel_->setText(tr("Type: %1").arg(elementTypeLabel(element.type)));
    if (element.type == QStringLiteral("text") || element.type == QStringLiteral("ticker")
        || element.type == QStringLiteral("qr")) {
        pages_->setCurrentIndex(0);
        textEdit_->setPlainText(texts_.value(element.name, element.value));
        textEdit_->setPlaceholderText(tr("Use the package text"));
        if (element.type == QStringLiteral("ticker")) {
            propertyHint_->setText(tr("Ticker text scrolls at the speed stored in the package."));
        } else if (element.value.contains(QLatin1String(title_params::kTimerMarker))) {
            propertyHint_->setText(
                tr("Timer supports {0:HH:mm:ss}; Start / Pause / Reset Countdown controls it live."));
        } else if (element.type == QStringLiteral("qr")) {
            propertyHint_->setText(tr("Text encoded by the QR element."));
        } else {
            propertyHint_->setText(tr("Leave empty to use the package default."));
        }
    } else if (element.type == QStringLiteral("colour")) {
        pages_->setCurrentIndex(1);
        const QColor fallback(element.value);
        showColour(colours_.value(element.name, fallback.isValid() ? fallback : Qt::white));
    } else if (element.type == QStringLiteral("image")) {
        pages_->setCurrentIndex(2);
        imagePath_->setText(images_.value(element.name));
        imagePath_->setPlaceholderText(tr("Use the image stored in the package"));
    } else {
        pages_->setCurrentIndex(0);
        textEdit_->setPlainText(texts_.value(element.name, element.value));
        propertyHint_->setText(tr("Leave empty to use the package default."));
    }
    updating_ = false;
}

const TitleElementInfo* TitleElementDialog::currentInfo() const
{
    if (currentRow_ < 0 || currentRow_ >= elements_.size()) {
        return nullptr;
    }
    return &elements_.at(currentRow_);
}

void TitleElementDialog::editText()
{
    if (updating_) {
        return;
    }
    const auto* info = currentInfo();
    if (!info) {
        return;
    }
    const QString value = textEdit_->toPlainText();
    if (value.isEmpty() || value == info->value) {
        texts_.remove(info->name);
    } else {
        texts_.insert(info->name, value);
    }
    updatePreview();
}

void TitleElementDialog::showColour(const QColor& colour)
{
    const QColor shown = colour.isValid() ? colour : Qt::white;
    colourButton_->setText(shown.name(QColor::HexRgb).toUpper());
    colourButton_->setStyleSheet(QStringLiteral("background:%1; color:%2;")
                                     .arg(shown.name(),
                                         shown.lightness() > 140 ? QStringLiteral("#10141a")
                                                                 : QStringLiteral("#ffffff")));
    colourValue_->setText(shown.name(QColor::HexRgb).toUpper());
}

void TitleElementDialog::chooseColour()
{
    const auto* info = currentInfo();
    if (!info) {
        return;
    }
    QColor initial = colours_.value(info->name);
    if (!initial.isValid()) {
        initial = QColor(info->value);
    }
    const QColor chosen = QColorDialog::getColor(initial.isValid() ? initial : Qt::white,
        this, tr("Element Colour"));
    if (!chosen.isValid()) {
        return;
    }
    colours_.insert(info->name, chosen);
    showColour(chosen);
    updatePreview();
}

void TitleElementDialog::resetColour()
{
    const auto* info = currentInfo();
    if (!info) {
        return;
    }
    colours_.remove(info->name);
    const QColor fallback(info->value);
    showColour(fallback.isValid() ? fallback : Qt::white);
    updatePreview();
}

void TitleElementDialog::editImagePath()
{
    if (updating_) {
        return;
    }
    const auto* info = currentInfo();
    if (!info) {
        return;
    }
    const QString path = imagePath_->text().trimmed();
    if (path.isEmpty()) {
        images_.remove(info->name);
    } else {
        images_.insert(info->name, path);
    }
    updatePreview();
}

void TitleElementDialog::browseImage()
{
    const auto* info = currentInfo();
    if (!info) {
        return;
    }
    const QString start = imagePath_->text().isEmpty()
        ? QString()
        : QFileInfo(imagePath_->text()).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, tr("Select Element Image"),
        start, tr("Images (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"));
    if (!path.isEmpty()) {
        imagePath_->setText(path);
    }
}

void TitleElementDialog::resetImage()
{
    updating_ = true;
    imagePath_->clear();
    updating_ = false;
    const auto* info = currentInfo();
    if (info) {
        images_.remove(info->name);
    }
    updatePreview();
}

void TitleElementDialog::updatePreview()
{
    if (!preview_ || !package_.isValid()) {
        return;
    }
    const QSize design = canvas_.isValid() ? canvas_ : package_.designSize();
    if (design.isEmpty()) {
        return;
    }
    QSize area = preview_->size();
    if (area.width() < 40 || area.height() < 40) {
        area = QSize(560, 315);
    }
    const qreal aspect = design.height() / qreal(design.width());
    int width = area.width();
    int height = qRound(width * aspect);
    if (height > area.height()) {
        height = area.height();
        width = qRound(height / aspect);
    }
    width = std::max(1, width);
    height = std::max(1, height);
    const QImage image = package_.render(QSize(width, height), texts_, colours_, images_,
        10.0, previewClock_.elapsed() / 1000.0);
    preview_->setPixmap(QPixmap::fromImage(image));
}

QHash<QString, QString> TitleElementDialog::textValues() const { return texts_; }

QHash<QString, QColor> TitleElementDialog::colourValues() const { return colours_; }

QHash<QString, QString> TitleElementDialog::imageValues() const { return images_; }

} // namespace mixvera::ui