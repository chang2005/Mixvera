#include "mixvera/ui/add_input_dialog.hpp"

#include "mixvera/ui/platform/win_capture.hpp"

#include <QComboBox>
#include <QColorDialog>
#include <QCameraDevice>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace mixvera::ui {

AddInputDialog::AddInputDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add Input"));
    setMinimumWidth(540);
    buildUi();
}

void AddInputDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    auto* form = new QFormLayout;
    form->setSpacing(8);

    type_ = new QComboBox(this);
    type_->addItem(tr("Screen Capture"), static_cast<int>(core::InputKind::ScreenCapture));
    type_->addItem(tr("Window Capture"), static_cast<int>(core::InputKind::WindowCapture));
    type_->addItem(tr("Camera"), static_cast<int>(core::InputKind::Camera));
    type_->addItem(tr("Image"), static_cast<int>(core::InputKind::Image));
    type_->addItem(tr("Media File"), static_cast<int>(core::InputKind::MediaFile));
    type_->addItem(tr("Title"), static_cast<int>(core::InputKind::Title));
    type_->addItem(tr("Colour"), static_cast<int>(core::InputKind::Color));
    type_->addItem(tr("Test Pattern"), static_cast<int>(core::InputKind::TestPattern));
    form->addRow(tr("Type"), type_);

    name_ = new QLineEdit(this);
    form->addRow(tr("Name"), name_);
    root->addLayout(form);

    pages_ = new QStackedWidget(this);

    auto* screenPage = new QWidget(pages_);
    auto* screenForm = new QFormLayout(screenPage);
    monitor_ = new QComboBox(screenPage);
    const auto screens = QGuiApplication::screens();
    for (int index = 0; index < screens.size(); ++index) {
        monitor_->addItem(tr("Display %1 · %2x%3")
                              .arg(index + 1)
                              .arg(screens[index]->geometry().width())
                              .arg(screens[index]->geometry().height()),
            index);
    }
    screenForm->addRow(tr("Display"), monitor_);
    pages_->addWidget(screenPage);

    auto* windowPage = new QWidget(pages_);
    auto* windowForm = new QFormLayout(windowPage);
    window_ = new QComboBox(windowPage);
    for (const auto& window : listCaptureWindows()) {
        window_->addItem(window.title, QVariant::fromValue<qulonglong>(window.handle));
    }
    if (window_->count() == 0) {
        window_->addItem(tr("No capturable window found"), QVariant::fromValue<qulonglong>(0));
    }
    windowForm->addRow(tr("Window"), window_);
    auto* refresh = new QPushButton(tr("Refresh List"), windowPage);
    windowForm->addRow(QString(), refresh);
    connect(refresh, &QPushButton::clicked, this, [this] {
        window_->clear();
        for (const auto& window : listCaptureWindows()) {
            window_->addItem(window.title, QVariant::fromValue<qulonglong>(window.handle));
        }
        if (window_->count() == 0) {
            window_->addItem(tr("No capturable window found"), QVariant::fromValue<qulonglong>(0));
        }
    });
    pages_->addWidget(windowPage);

    auto* cameraPage = new QWidget(pages_);
    auto* cameraForm = new QFormLayout(cameraPage);
    camera_ = new QComboBox(cameraPage);
    for (const auto& device : QMediaDevices::videoInputs()) {
        camera_->addItem(device.description(), QString::fromUtf8(device.id()));
    }
    if (camera_->count() == 0) {
        camera_->addItem(tr("No camera detected"), QString());
    }
    cameraForm->addRow(tr("Device"), camera_);
    pages_->addWidget(cameraPage);

    auto* filePage = new QWidget(pages_);
    auto* fileLayout = new QFormLayout(filePage);
    auto* fileRow = new QHBoxLayout;
    file_ = new QLineEdit(filePage);
    browse_ = new QPushButton(tr("Browse…"), filePage);
    fileRow->addWidget(file_, 1);
    fileRow->addWidget(browse_);
    fileLayout->addRow(tr("File"), fileRow);
    pages_->addWidget(filePage);

    auto* titlePage = new QWidget(pages_);
    auto* titleForm = new QFormLayout(titlePage);
    text_ = new QPlainTextEdit(titlePage);
    text_->setPlaceholderText(tr("Title text, one line per row"));
    text_->setFixedHeight(84);
    titleForm->addRow(tr("Text"), text_);
    textSize_ = new QSpinBox(titlePage);
    textSize_->setRange(12, 200);
    textSize_->setValue(64);
    titleForm->addRow(tr("Size"), textSize_);
    align_ = new QComboBox(titlePage);
    align_->addItem(tr("Left"), "left");
    align_->addItem(tr("Center"), "center");
    align_->addItem(tr("Right"), "right");
    align_->setCurrentIndex(1);
    titleForm->addRow(tr("Align"), align_);
    colour_ = new QPushButton(tr("Choose Colour…"), titlePage);
    titleForm->addRow(tr("Colour"), colour_);
    pages_->addWidget(titlePage);

    auto* colourPage = new QWidget(pages_);
    auto* colourForm = new QFormLayout(colourPage);
    auto* colourPick = new QPushButton(tr("Choose Colour…"), colourPage);
    colourForm->addRow(tr("Colour"), colourPick);
    pages_->addWidget(colourPage);

    auto* patternPage = new QWidget(pages_);
    auto* patternForm = new QFormLayout(patternPage);
    pattern_ = new QComboBox(patternPage);
    pattern_->addItem(tr("SMPTE Bars"), "bars");
    pattern_->addItem(tr("Colour Bars"), "colour");
    patternForm->addRow(tr("Pattern"), pattern_);
    pages_->addWidget(patternPage);

    root->addWidget(pages_, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(type_, &QComboBox::currentIndexChanged, this, [this] {
        updatePage();
        refreshName();
    });
    connect(browse_, &QPushButton::clicked, this, &AddInputDialog::browseFile);
    connect(colour_, &QPushButton::clicked, this, &AddInputDialog::pickColour);
    connect(colourPick, &QPushButton::clicked, this, &AddInputDialog::pickColour);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updatePage();
    refreshName();
}

void AddInputDialog::updatePage()
{
    const auto kind = static_cast<core::InputKind>(type_->currentData().toInt());
    switch (kind) {
    case core::InputKind::ScreenCapture:
        pages_->setCurrentIndex(0);
        break;
    case core::InputKind::WindowCapture:
        pages_->setCurrentIndex(1);
        break;
    case core::InputKind::Camera:
        pages_->setCurrentIndex(2);
        break;
    case core::InputKind::Image:
    case core::InputKind::MediaFile:
        pages_->setCurrentIndex(3);
        break;
    case core::InputKind::Title:
        pages_->setCurrentIndex(4);
        break;
    case core::InputKind::Color:
        pages_->setCurrentIndex(5);
        break;
    default:
        pages_->setCurrentIndex(6);
        break;
    }
}

void AddInputDialog::refreshName()
{
    if (!name_->text().isEmpty()) {
        return;
    }
    const auto kind = static_cast<core::InputKind>(type_->currentData().toInt());
    switch (kind) {
    case core::InputKind::ScreenCapture:
        name_->setText(tr("Screen %1").arg(monitor_->currentIndex() + 1));
        break;
    case core::InputKind::WindowCapture:
        name_->setText(window_->count() > 0 ? window_->currentText() : tr("Window Capture"));
        break;
    case core::InputKind::Camera:
        name_->setText(camera_->count() > 0 ? camera_->currentText() : tr("Camera"));
        break;
    case core::InputKind::Image:
        name_->setText(file_->text().isEmpty() ? tr("Image")
                                               : QFileInfo(file_->text()).baseName());
        break;
    case core::InputKind::MediaFile:
        name_->setText(file_->text().isEmpty() ? tr("Media")
                                               : QFileInfo(file_->text()).baseName());
        break;
    case core::InputKind::Title:
        name_->setText(tr("Title"));
        break;
    case core::InputKind::Color:
        name_->setText(tr("Colour"));
        break;
    default:
        name_->setText(tr("Test Pattern"));
        break;
    }
}

void AddInputDialog::browseFile()
{
    const auto kind = static_cast<core::InputKind>(type_->currentData().toInt());
    const QString filter = kind == core::InputKind::Image
        ? tr("Images (*.png *.jpg *.jpeg *.bmp *.webp *.gif)")
        : tr("Video (*.mp4 *.mov *.mkv *.avi *.webm *.flv);;Audio (*.mp3 *.wav *.aac *.m4a)");
    const QString path = QFileDialog::getOpenFileName(this, tr("Select File"), QString(), filter);
    if (!path.isEmpty()) {
        file_->setText(path);
        name_->setText(QFileInfo(path).baseName());
    }
}

void AddInputDialog::pickColour()
{
    const QColor chosen = QColorDialog::getColor(colourValue_, this, tr("Choose Colour"));
    if (chosen.isValid()) {
        colourValue_ = chosen;
        colour_->setText(colourValue_.name().toUpper());
    }
}

void AddInputDialog::loadFrom(const core::InputDescriptor& input)
{
    id_ = QString::fromStdString(input.id);
    setWindowTitle(tr("Input Properties"));

    const int index = type_->findData(static_cast<int>(input.kind));
    if (index >= 0) {
        type_->setCurrentIndex(index);
    }
    name_->setText(QString::fromStdString(input.name));

    const auto value = [&input](const char* key) {
        const auto it = input.params.find(key);
        return it == input.params.end() ? QString() : QString::fromStdString(it->second);
    };

    if (input.kind == core::InputKind::ScreenCapture) {
        monitor_->setCurrentIndex(value("monitor").toInt());
    } else if (input.kind == core::InputKind::WindowCapture) {
        const auto handle = value("window").toULongLong();
        for (int row = 0; row < window_->count(); ++row) {
            if (window_->itemData(row).toULongLong() == handle) {
                window_->setCurrentIndex(row);
                break;
            }
        }
    } else if (input.kind == core::InputKind::Camera) {
        const QString deviceId = value("device");
        for (int row = 0; row < camera_->count(); ++row) {
            if (camera_->itemData(row).toString() == deviceId) {
                camera_->setCurrentIndex(row);
                break;
            }
        }
    } else if (input.kind == core::InputKind::Image
        || input.kind == core::InputKind::MediaFile) {
        file_->setText(value("file"));
    } else if (input.kind == core::InputKind::Title) {
        text_->setPlainText(value("text"));
        textSize_->setValue(value("size").toInt());
        const int alignIndex = align_->findData(value("align"));
        align_->setCurrentIndex(alignIndex < 0 ? 1 : alignIndex);
        colourValue_ = QColor(value("color"));
        colour_->setText(colourValue_.name().toUpper());
    } else if (input.kind == core::InputKind::Color) {
        colourValue_ = QColor(value("color"));
        setStyleSheet(QString());
    }
    updatePage();
}

void AddInputDialog::lockKind(bool locked)
{
    type_->setEnabled(!locked);
}

core::InputDescriptor AddInputDialog::descriptor() const
{
    core::InputDescriptor input;
    input.id = id_.toStdString();
    input.name = name_->text().trimmed().toStdString();
    input.kind = static_cast<core::InputKind>(type_->currentData().toInt());

    switch (input.kind) {
    case core::InputKind::ScreenCapture:
        input.params["monitor"] = std::to_string(monitor_->currentIndex());
        break;
    case core::InputKind::WindowCapture:
        input.params["window"] = std::to_string(window_->currentData().toULongLong());
        input.params["windowTitle"] = window_->currentText().toStdString();
        break;
    case core::InputKind::Camera:
        input.params["device"] = camera_->currentData().toString().toStdString();
        break;
    case core::InputKind::Image:
    case core::InputKind::MediaFile:
        input.params["file"] = file_->text().toStdString();
        break;
    case core::InputKind::Title:
        input.params["text"] = text_->toPlainText().toStdString();
        input.params["size"] = std::to_string(textSize_->value());
        input.params["align"] = align_->currentData().toString().toStdString();
        input.params["color"] = colourValue_.name().toStdString();
        break;
    case core::InputKind::Color:
        input.params["color"] = colourValue_.name().toStdString();
        break;
    default:
        input.params["pattern"] = pattern_->currentData().toString().toStdString();
        break;
    }

    input.hasAudio = core::inputKindHasAudio(input.kind);
    return input;
}

} // namespace mixvera::ui