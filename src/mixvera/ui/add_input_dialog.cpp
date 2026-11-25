#include "mixvera/ui/add_input_dialog.hpp"

#include "mixvera/ui/media/chroma_key.hpp"
#include "mixvera/ui/media/title_package.hpp"
#include "mixvera/ui/platform/win_capture.hpp"
#include "mixvera/ui/title_element_dialog.hpp"

#include <QCheckBox>
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
    titleStyle_ = new QComboBox(titlePage);
    titleStyle_->setObjectName("titleStyle");
    titleStyle_->addItem(tr("Full Screen"), "full");
    titleStyle_->addItem(tr("Lower Third"), "lower");
    titleStyle_->addItem(tr("Countdown"), "countdown");
    titleStyle_->addItem(tr("Scoreboard"), "scoreboard");
    titleStyle_->addItem(tr("Subtitle"), "caption");
    titleStyle_->addItem(tr("Rolling Ticker"), "ticker");
    titleStyle_->addItem(tr("Title Package"), "package");
    titleForm->addRow(tr("Style"), titleStyle_);

    packageRow_ = new QWidget(titlePage);
    packageRow_->setObjectName("titlePackageRow");
    auto* packageLayout = new QVBoxLayout(packageRow_);
    packageLayout->setContentsMargins(0, 0, 0, 0);
    packageLayout->setSpacing(4);
    auto* packageLine = new QHBoxLayout;
    packageLine->setContentsMargins(0, 0, 0, 0);
    packagePath_ = new QLineEdit(packageRow_);
    packagePath_->setObjectName("titlePackagePath");
    packagePath_->setPlaceholderText(tr("Select a .gtzip, .sinsamtitle or .xaml title package"));
    packageBrowse_ = new QPushButton(tr("Browse…"), packageRow_);
    editElements_ = new QPushButton(tr("Elements…"), packageRow_);
    editElements_->setObjectName("titleElementsButton");
    packageLine->addWidget(packagePath_, 1);
    packageLine->addWidget(packageBrowse_);
    packageLine->addWidget(editElements_);
    packageLayout->addLayout(packageLine);
    packageInfo_ = new QLabel(packageRow_);
    packageInfo_->setObjectName("packageInfo");
    packageInfo_->setWordWrap(true);
    packageLayout->addWidget(packageInfo_);
    titleForm->addRow(tr("Package"), packageRow_);
    titleForm_ = titleForm;
    packageRow_->setVisible(false);
    if (auto* label = titleForm_->labelForField(packageRow_)) {
        label->setVisible(false);
    }

    text_ = new QPlainTextEdit(titlePage);
    text_->setPlaceholderText(tr("Title text, one line per row"));
    text_->setFixedHeight(84);
    titleForm->addRow(tr("Text"), text_);
    subtitle_ = new QLineEdit(titlePage);
    subtitle_->setPlaceholderText(tr("Optional second line for lower thirds"));
    titleForm->addRow(tr("Subtitle"), subtitle_);
    captions_ = new QPlainTextEdit(titlePage);
    captions_->setPlaceholderText(tr("Subtitle lines for Next / Previous, one per line"));
    captions_->setFixedHeight(64);
    titleForm->addRow(tr("Subtitle Lines"), captions_);
    tickerSpeed_ = new QSpinBox(titlePage);
    tickerSpeed_->setRange(20, 800);
    tickerSpeed_->setValue(120);
    tickerSpeed_->setSuffix(" px/s");
    titleForm->addRow(tr("Ticker Speed"), tickerSpeed_);
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
    titleForm->addRow(tr("Accent Colour"), colour_);
    countdownSeconds_ = new QSpinBox(titlePage);
    countdownSeconds_->setRange(1, 86400);
    countdownSeconds_->setValue(300);
    countdownSeconds_->setSuffix(" s");
    titleForm->addRow(tr("Countdown Length"), countdownSeconds_);
    competition_ = new QLineEdit(titlePage);
    competition_->setPlaceholderText(tr("Competition or round"));
    titleForm->addRow(tr("Competition"), competition_);
    auto* homeRow = new QWidget(titlePage);
    auto* homeLayout = new QHBoxLayout(homeRow);
    homeLayout->setContentsMargins(0, 0, 0, 0);
    homeLayout->setSpacing(6);
    homeName_ = new QLineEdit(homeRow);
    homeName_->setPlaceholderText(tr("Home team"));
    homeScore_ = new QSpinBox(homeRow);
    homeScore_->setRange(0, 999);
    homeLayout->addWidget(homeName_, 1);
    homeLayout->addWidget(homeScore_);
    titleForm->addRow(tr("Home Team"), homeRow);
    auto* awayRow = new QWidget(titlePage);
    auto* awayLayout = new QHBoxLayout(awayRow);
    awayLayout->setContentsMargins(0, 0, 0, 0);
    awayLayout->setSpacing(6);
    awayName_ = new QLineEdit(awayRow);
    awayName_->setPlaceholderText(tr("Away team"));
    awayScore_ = new QSpinBox(awayRow);
    awayScore_->setRange(0, 999);
    awayLayout->addWidget(awayName_, 1);
    awayLayout->addWidget(awayScore_);
    titleForm->addRow(tr("Away Team"), awayRow);
    connect(titleStyle_, &QComboBox::currentIndexChanged, this,
        [this] { updateTitleStyleControls(); });
    connect(packagePath_, &QLineEdit::textChanged, this,
        [this] { updateTitleStyleControls(); });
    connect(packageBrowse_, &QPushButton::clicked, this, [this] {
        const QString start = packagePath_->text().isEmpty()
            ? QString()
            : QFileInfo(packagePath_->text()).absolutePath();
        const QString path = QFileDialog::getOpenFileName(this, tr("Select Title Package"),
            start, tr("GT title packages (*.gtzip *.sinsamtitle *.xaml);;All files (*)"));
        if (!path.isEmpty()) {
            if (path != packagePath_->text()) {
                elementTexts_.clear();
                elementColours_.clear();
                elementImages_.clear();
            }
            packagePath_->setText(path);
            if (name_->text().isEmpty() || name_->text() == tr("Title")) {
                name_->setText(QFileInfo(path).completeBaseName());
            }
        }
    });
    connect(editElements_, &QPushButton::clicked, this, [this] {
        const QString path = packagePath_->text().trimmed();
        const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
        if (!package.isValid()) {
            return;
        }
        mixvera::ui::TitleElementDialog dialog(package, elementTexts_, elementColours_,
            elementImages_, QSize(), this);
        if (dialog.exec() == QDialog::Accepted) {
            elementTexts_ = dialog.textValues();
            elementColours_ = dialog.colourValues();
            elementImages_ = dialog.imageValues();
        }
    });
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

    chromaPanel_ = new QWidget(this);
    auto* chromaLayout = new QVBoxLayout(chromaPanel_);
    chromaLayout->setContentsMargins(0, 2, 0, 0);
    chromaLayout->setSpacing(6);
    auto* chromaTitle = new QLabel(tr("CHROMA KEY"), chromaPanel_);
    chromaTitle->setObjectName("cardTitle");
    chromaLayout->addWidget(chromaTitle);

    auto* chromaForm = new QFormLayout;
    chromaForm->setSpacing(6);
    chromaEnabled_ = new QCheckBox(tr("Remove a green or blue screen"), chromaPanel_);
    chromaEnabled_->setObjectName("chromaEnabled");
    chromaForm->addRow(QString(), chromaEnabled_);

    chromaPreset_ = new QComboBox(chromaPanel_);
    chromaPreset_->setObjectName("chromaPreset");
    chromaPreset_->addItem(tr("Green Screen"), "green");
    chromaPreset_->addItem(tr("Blue Screen"), "blue");
    chromaPreset_->addItem(tr("Custom Colour"), "custom");
    chromaForm->addRow(tr("Key Colour"), chromaPreset_);

    chromaColour_ = new QPushButton(chromaValue_.name().toUpper(), chromaPanel_);
    chromaColour_->setObjectName("chromaColourButton");
    chromaForm->addRow(tr("Custom Colour"), chromaColour_);

    chromaSimilarity_ = new QSpinBox(chromaPanel_);
    chromaSimilarity_->setRange(0, 100);
    chromaSimilarity_->setValue(35);
    chromaSimilarity_->setSuffix(" %");
    chromaForm->addRow(tr("Similarity"), chromaSimilarity_);

    chromaSmoothness_ = new QSpinBox(chromaPanel_);
    chromaSmoothness_->setRange(0, 100);
    chromaSmoothness_->setValue(18);
    chromaSmoothness_->setSuffix(" %");
    chromaForm->addRow(tr("Smoothness"), chromaSmoothness_);

    chromaSpill_ = new QSpinBox(chromaPanel_);
    chromaSpill_->setRange(0, 100);
    chromaSpill_->setValue(60);
    chromaSpill_->setSuffix(" %");
    chromaForm->addRow(tr("Spill Suppression"), chromaSpill_);

    chromaLayout->addLayout(chromaForm);
    root->addWidget(chromaPanel_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(type_, &QComboBox::currentIndexChanged, this, [this] {
        updatePage();
        refreshName();
    });
    connect(browse_, &QPushButton::clicked, this, &AddInputDialog::browseFile);
    connect(colour_, &QPushButton::clicked, this, &AddInputDialog::pickColour);
    connect(colourPick, &QPushButton::clicked, this, &AddInputDialog::pickColour);
    connect(chromaEnabled_, &QCheckBox::toggled, this,
        [this] { updateChromaControls(); });
    connect(chromaPreset_, &QComboBox::currentIndexChanged, this, [this] {
        const QString preset = chromaPreset_->currentData().toString();
        if (preset == "green") {
            chromaValue_ = QColor("#00ff00");
        } else if (preset == "blue") {
            chromaValue_ = QColor("#0000ff");
        }
        updateChromaControls();
    });
    connect(chromaColour_, &QPushButton::clicked, this, &AddInputDialog::pickChromaColour);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updatePage();
    refreshName();
    updateChromaControls();
}

void AddInputDialog::pickChromaColour()
{
    const QColor chosen = QColorDialog::getColor(chromaValue_, this, tr("Choose Key Colour"));
    if (!chosen.isValid()) {
        return;
    }
    chromaValue_ = chosen;
    const int custom = chromaPreset_->findData("custom");
    if (custom >= 0) {
        chromaPreset_->setCurrentIndex(custom);
    }
    updateChromaControls();
}

void AddInputDialog::updateChromaControls()
{
    if (!chromaEnabled_) {
        return;
    }
    const bool enabled = chromaEnabled_->isChecked();
    const bool custom = chromaPreset_->currentData().toString() == "custom";
    chromaPreset_->setEnabled(enabled);
    chromaColour_->setEnabled(enabled && custom);
    chromaSimilarity_->setEnabled(enabled);
    chromaSmoothness_->setEnabled(enabled);
    chromaSpill_->setEnabled(enabled);
    chromaColour_->setText(chromaValue_.name().toUpper());
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
    if (chromaPanel_) {
        chromaPanel_->setVisible(inputSupportsChromaKey(kind));
    }
}

void AddInputDialog::updateTitleStyleControls()
{
    if (!titleStyle_ || !packageRow_ || !titleForm_) {
        return;
    }
    const bool package = titleStyle_->currentData().toString() == "package";
    packageRow_->setVisible(package);
    if (auto* label = titleForm_->labelForField(packageRow_)) {
        label->setVisible(package);
    }
    if (auto* label = qobject_cast<QLabel*>(titleForm_->labelForField(captions_))) {
        label->setText(package ? tr("Caption Texts") : tr("Subtitle Lines"));
    }
    captions_->setPlaceholderText(package
            ? tr("Caption texts, one per line (Next Caption cycles them)")
            : tr("Subtitle lines for Next / Previous, one per line"));
    editElements_->setEnabled(false);
    if (!package) {
        return;
    }
    const QString path = packagePath_->text().trimmed();
    if (path.isEmpty()) {
        packageInfo_->setText(tr("Choose a vMix or SinSam .gtzip, .sinsamtitle or .xaml package."));
        return;
    }
    const auto elements = titlePackageElements(path);
    if (elements.isEmpty()) {
        packageInfo_->setText(tr("Could not read this title package."));
        return;
    }
    editElements_->setEnabled(true);
    QStringList names;
    for (int index = 0; index < elements.size() && index < 8; ++index) {
        names.append(elements.at(index).name);
    }
    if (elements.size() > names.size()) {
        names.append(QStringLiteral("…"));
    }
    packageInfo_->setText(tr("%1 editable elements: %2")
            .arg(elements.size())
            .arg(names.join(QStringLiteral(", "))));
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
        if (titleStyle_->currentData().toString() == "package"
            && !packagePath_->text().isEmpty()) {
            name_->setText(QFileInfo(packagePath_->text()).completeBaseName());
        } else {
            name_->setText(tr("Title"));
        }
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
    baseParams_ = input.params;
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
        const int styleIndex = titleStyle_->findData(value("style"));
        titleStyle_->setCurrentIndex(styleIndex < 0 ? 0 : styleIndex);
        packagePath_->setText(value("package"));
        elementTexts_.clear();
        elementColours_.clear();
        elementImages_.clear();
        for (const auto& entry : input.params) {
            const QString key = QString::fromStdString(entry.first);
            const QString entryValue = QString::fromStdString(entry.second);
            if (key.startsWith(QLatin1String(title_params::kTextPrefix))) {
                elementTexts_.insert(key.mid(10), entryValue);
            } else if (key.startsWith(QLatin1String(title_params::kColourPrefix))) {
                const QColor colour(entryValue);
                if (colour.isValid()) {
                    elementColours_.insert(key.mid(11), colour);
                }
            } else if (key.startsWith(QLatin1String(title_params::kImagePrefix))) {
                elementImages_.insert(key.mid(11), entryValue);
            }
        }
        text_->setPlainText(value("text"));
        subtitle_->setText(value("subtitle"));
        captions_->setPlainText(value("captions"));
        const int tickerSpeed = value("tickerSpeed").toInt();
        tickerSpeed_->setValue(tickerSpeed > 0 ? tickerSpeed : 120);
        textSize_->setValue(value("size").toInt());
        const int alignIndex = align_->findData(value("align"));
        align_->setCurrentIndex(alignIndex < 0 ? 1 : alignIndex);
        colourValue_ = QColor(value("color"));
        colour_->setText(colourValue_.name().toUpper());
        const int countdown = value("countdownSeconds").toInt();
        countdownSeconds_->setValue(countdown > 0 ? countdown : 300);
        competition_->setText(value("competition"));
        homeName_->setText(value("homeName"));
        awayName_->setText(value("awayName"));
        homeScore_->setValue(value("scoreHome").toInt());
        awayScore_->setValue(value("scoreAway").toInt());
    } else if (input.kind == core::InputKind::Color) {
        colourValue_ = QColor(value("color"));
        setStyleSheet(QString());
    }

    chromaEnabled_->setChecked(value("chromaEnabled") == "1");
    const QColor storedChroma(value("chromaColor"));
    if (storedChroma.isValid()) {
        chromaValue_ = storedChroma;
    }
    QString preset = value("chromaPreset");
    if (preset.isEmpty()) {
        if (chromaValue_.green() > chromaValue_.red() + 60
            && chromaValue_.green() > chromaValue_.blue() + 60) {
            preset = "green";
        } else if (chromaValue_.blue() > chromaValue_.red() + 60
            && chromaValue_.blue() > chromaValue_.green() + 60) {
            preset = "blue";
        } else {
            preset = "custom";
        }
    }
    const int presetIndex = chromaPreset_->findData(preset);
    chromaPreset_->setCurrentIndex(presetIndex < 0 ? 0 : presetIndex);
    chromaSimilarity_->setValue(value("chromaSimilarity").isEmpty()
            ? 35
            : value("chromaSimilarity").toInt());
    chromaSmoothness_->setValue(value("chromaSmoothness").isEmpty()
            ? 18
            : value("chromaSmoothness").toInt());
    chromaSpill_->setValue(value("chromaSpill").isEmpty() ? 60 : value("chromaSpill").toInt());

    updatePage();
    updateChromaControls();
    updateTitleStyleControls();
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
    // Keep runtime parameters the dialog does not edit (countdown end time,
    // remaining time and pause state) so opening properties never restarts a
    // live countdown.
    input.params = baseParams_;

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
        input.params["style"] = titleStyle_->currentData().toString().toStdString();
        input.params["subtitle"] = subtitle_->text().trimmed().toStdString();
        input.params["captions"] = captions_->toPlainText().toStdString();
        input.params["tickerSpeed"] = std::to_string(tickerSpeed_->value());
        input.params["size"] = std::to_string(textSize_->value());
        input.params["align"] = align_->currentData().toString().toStdString();
        input.params["color"] = colourValue_.name().toStdString();
        input.params["countdownSeconds"] = std::to_string(countdownSeconds_->value());
        input.params["competition"] = competition_->text().trimmed().toStdString();
        input.params["homeName"] = homeName_->text().trimmed().toStdString();
        input.params["awayName"] = awayName_->text().trimmed().toStdString();
        input.params["scoreHome"] = std::to_string(homeScore_->value());
        input.params["scoreAway"] = std::to_string(awayScore_->value());
        input.params["package"] = packagePath_->text().trimmed().toStdString();
        for (auto it = input.params.begin(); it != input.params.end();) {
            const QString key = QString::fromStdString(it->first);
            if (key.startsWith(QLatin1String(title_params::kTextPrefix))
                || key.startsWith(QLatin1String(title_params::kColourPrefix))
                || key.startsWith(QLatin1String(title_params::kImagePrefix))) {
                it = input.params.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = elementTexts_.constBegin(); it != elementTexts_.constEnd(); ++it) {
            input.params[title_params::textKey(it.key()).toStdString()]
                = it.value().toStdString();
        }
        for (auto it = elementColours_.constBegin(); it != elementColours_.constEnd(); ++it) {
            input.params[title_params::colourKey(it.key()).toStdString()]
                = it.value().name(QColor::HexRgb).toStdString();
        }
        for (auto it = elementImages_.constBegin(); it != elementImages_.constEnd(); ++it) {
            input.params[title_params::imageKey(it.key()).toStdString()]
                = it.value().toStdString();
        }
        break;
    case core::InputKind::Color:
        input.params["color"] = colourValue_.name().toStdString();
        break;
    default:
        input.params["pattern"] = pattern_->currentData().toString().toStdString();
        break;
    }

    if (inputSupportsChromaKey(input.kind)) {
        input.params["chromaEnabled"] = chromaEnabled_->isChecked() ? "1" : "0";
        input.params["chromaPreset"] = chromaPreset_->currentData().toString().toStdString();
        input.params["chromaColor"] = chromaValue_.name().toStdString();
        input.params["chromaSimilarity"] = std::to_string(chromaSimilarity_->value());
        input.params["chromaSmoothness"] = std::to_string(chromaSmoothness_->value());
        input.params["chromaSpill"] = std::to_string(chromaSpill_->value());
    } else {
        for (const char* key : {"chromaEnabled", "chromaPreset", "chromaColor",
                 "chromaSimilarity", "chromaSmoothness", "chromaSpill"}) {
            input.params.erase(key);
        }
    }

    input.hasAudio = core::inputKindHasAudio(input.kind);
    return input;
}

} // namespace mixvera::ui