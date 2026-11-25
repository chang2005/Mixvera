#include "mixvera/ui/settings_dialog.hpp"

#include "mixvera/ui/media/recorder.hpp"

#include <QAudioDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMediaDevices>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyle>
#include <QSysInfo>
#include <QVBoxLayout>

#include <functional>
#include <numeric>

namespace mixvera::ui {

namespace {

QFrame* makeCard(const QString& title, QWidget* parent, QVBoxLayout** body)
{
    auto* card = new QFrame(parent);
    card->setObjectName("settingsCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setSpacing(10);
    auto* label = new QLabel(title, card);
    label->setObjectName("cardTitle");
    layout->addWidget(label);
    *body = layout;
    return card;
}

QFormLayout* makeForm()
{
    auto* form = new QFormLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(9);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    return form;
}

QLabel* note(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("settingsPageHint");
    label->setWordWrap(true);
    return label;
}

/// Miniature of the three-pane workspace, painted with the palette of the
/// theme it represents so the card never depends on the active theme.
class ThemePreview final : public QWidget {
public:
    ThemePreview(theme::ThemeId id, QWidget* parent)
        : QWidget(parent)
        , id_(id)
    {
        setFixedSize(168, 86);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const theme::Palette& p = theme::palette(id_);
        QPainter painter(this);
        painter.fillRect(rect(), QColor(p.window));

        painter.fillRect(QRectF(0, 0, width(), 12), QColor(p.surface));
        painter.fillRect(QRectF(6, 4, 22, 4), QColor(p.accent));
        painter.fillRect(QRectF(width() - 32, 4, 12, 4), QColor(p.surfaceAlt));
        painter.fillRect(QRectF(width() - 16, 4, 10, 4), QColor(p.surfaceAlt));

        painter.fillRect(QRectF(0, 12, 30, height() - 32), QColor(p.surface));
        painter.fillRect(QRectF(4, 19, 22, 4), QColor(p.selection));
        painter.fillRect(QRectF(4, 27, 18, 4), QColor(p.raised));
        painter.fillRect(QRectF(4, 35, 20, 4), QColor(p.raised));

        const QRectF preview(38, 18, 58, 34);
        const QRectF program(104, 18, 58, 34);
        painter.fillRect(preview, QColor(p.monitor));
        painter.fillRect(QRectF(preview.left(), preview.top(), preview.width(), 6),
            QColor(p.preview));
        painter.fillRect(program, QColor(p.monitor));
        painter.fillRect(QRectF(program.left(), program.top(), program.width(), 6),
            QColor(p.program));
        painter.fillRect(QRectF(96, 20, 5, 30), QColor(p.raised));

        painter.fillRect(QRectF(0, height() - 20, width(), 20), QColor(p.surfaceAlt));
        painter.fillRect(QRectF(6, height() - 15, 22, 10), QColor(p.field));
        painter.fillRect(QRectF(32, height() - 15, 22, 10), QColor(p.field));
        painter.fillRect(QRectF(58, height() - 15, 22, 10), QColor(p.accent));

        painter.setPen(QPen(QColor(p.border), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

private:
    theme::ThemeId id_;
};

class ThemeCard final : public QFrame {
public:
    ThemeCard(theme::ThemeId id, QWidget* parent)
        : QFrame(parent)
    {
        setObjectName("themeCard");
        setCursor(Qt::PointingHandCursor);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 9);
        layout->setSpacing(5);

        auto* preview = new ThemePreview(id, this);
        layout->addWidget(preview, 0, Qt::AlignHCenter);

        auto* name = new QLabel(theme::themeName(id), this);
        name->setObjectName("themeName");
        name->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(name);

        auto* hint = new QLabel(theme::themeDescription(id), this);
        hint->setObjectName("themeHint");
        hint->setWordWrap(true);
        hint->setMinimumHeight(30);
        hint->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(hint);
    }

    std::function<void()> onSelected;

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && onSelected) {
            onSelected();
        }
        QFrame::mousePressEvent(event);
    }
};

QString compilerName()
{
#if defined(__clang__)
    return QStringLiteral("Clang %1").arg(QStringLiteral(__clang_version__));
#elif defined(__GNUC__)
    return QStringLiteral("GCC %1.%2").arg(__GNUC__).arg(__GNUC_MINOR__);
#elif defined(_MSC_VER)
    return QStringLiteral("MSVC %1").arg(_MSC_VER);
#else
    return QStringLiteral("unknown");
#endif
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName("settingsDialog");
    setWindowTitle(tr("Mixvera Settings"));
    resize(1000, 660);
    setMinimumSize(880, 560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* heading = new QWidget(this);
    auto* headingLayout = new QVBoxLayout(heading);
    headingLayout->setContentsMargins(22, 18, 22, 12);
    headingLayout->setSpacing(2);
    auto* title = new QLabel(tr("Settings"), heading);
    title->setObjectName("settingsTitle");
    auto* subtitle = new QLabel(tr("Workspace, video, audio and outputs"), heading);
    subtitle->setObjectName("settingsSubtitle");
    headingLayout->addWidget(title);
    headingLayout->addWidget(subtitle);
    root->addWidget(heading);

    auto* body = new QHBoxLayout;
    body->setContentsMargins(18, 6, 18, 16);
    body->setSpacing(14);

    nav_ = new QListWidget(this);
    nav_->setObjectName("settingsNav");
    nav_->setFixedWidth(178);
    nav_->setFocusPolicy(Qt::NoFocus);
    nav_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pages_ = new QStackedWidget(this);
    pages_->setObjectName("settingsPages");

    const struct {
        QString label;
        QWidget* page;
    } entries[] = {
        {tr("General"), buildGeneralPage()},
        {tr("Video"), buildVideoPage()},
        {tr("Audio"), buildAudioPage()},
        {tr("Output"), buildOutputPage()},
        {tr("Shortcuts"), buildShortcutsPage()},
        {tr("About"), buildAboutPage()},
    };
    for (const auto& entry : entries) {
        nav_->addItem(entry.label);
        pages_->addWidget(entry.page);
    }
    connect(nav_, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
    nav_->setCurrentRow(0);

    body->addWidget(nav_);
    body->addWidget(pages_, 1);
    root->addLayout(body, 1);

    auto* footer = new QFrame(this);
    footer->setObjectName("settingsFooter");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 12, 20, 12);
    footerLayout->setSpacing(8);
    auto* version = new QLabel(tr("Mixvera %1").arg(QCoreApplication::applicationVersion()),
        footer);
    version->setObjectName("settingsSubtitle");
    footerLayout->addWidget(version);
    footerLayout->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, footer);
    auto* ok = buttons->button(QDialogButtonBox::Ok);
    ok->setObjectName("primaryButton");
    ok->setText(tr("OK"));
    ok->setMinimumWidth(96);
    auto* cancel = buttons->button(QDialogButtonBox::Cancel);
    cancel->setObjectName("ghostButton");
    cancel->setText(tr("Cancel"));
    cancel->setMinimumWidth(96);
    // QDialogButtonBox polishes its standard buttons before the names above
    // exist; re-polish so QPushButton#primaryButton / #ghostButton match.
    for (QPushButton* button : {ok, cancel}) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    footerLayout->addWidget(buttons);
    root->addWidget(footer);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    selectTheme(theme_, false);
}

QWidget* SettingsDialog::pageShell(const QString& title, const QString& hint, QWidget* content)
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 8, 8);
    layout->setSpacing(6);
    auto* titleLabel = new QLabel(title, page);
    titleLabel->setObjectName("settingsPageTitle");
    layout->addWidget(titleLabel);
    auto* hintLabel = new QLabel(hint, page);
    hintLabel->setObjectName("settingsPageHint");
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);
    layout->addSpacing(4);
    layout->addWidget(content);
    layout->addStretch();

    auto* scroll = new QScrollArea(pages_);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(page);
    return scroll;
}

QWidget* SettingsDialog::buildGeneralPage()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QVBoxLayout* themeBody = nullptr;
    auto* themeCard = makeCard(tr("THEME"), content, &themeBody);
    auto* cards = new QHBoxLayout;
    cards->setSpacing(10);
    const theme::ThemeId ids[] = {
        theme::ThemeId::Charcoal, theme::ThemeId::Yami, theme::ThemeId::Daylight,
    };
    for (theme::ThemeId id : ids) {
        auto* card = new ThemeCard(id, themeCard);
        card->onSelected = [this, id] { selectTheme(id, true); };
        themeCards_.push_back({id, card});
        cards->addWidget(card);
    }
    cards->addStretch();
    themeBody->addLayout(cards);
    themeBody->addWidget(note(tr("The theme applies to the running window. Cancel restores "
                                 "the previous theme."),
        themeCard));
    layout->addWidget(themeCard);

    QVBoxLayout* languageBody = nullptr;
    auto* languageCard = makeCard(tr("LANGUAGE"), content, &languageBody);
    auto* languageForm = makeForm();
    language_ = new QComboBox(languageCard);
    language_->addItem(tr("Follow system"), QString());
    language_->addItem(QStringLiteral("简体中文"), QStringLiteral("zh_CN"));
    language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
    languageForm->addRow(tr("Interface language"), language_);
    languageBody->addLayout(languageForm);
    languageBody->addWidget(note(tr("Language changes take effect the next time Mixvera "
                                    "starts."),
        languageCard));
    layout->addWidget(languageCard);

    QVBoxLayout* startupBody = nullptr;
    auto* startupCard = makeCard(tr("STARTUP"), content, &startupBody);
    restoreSession_ = new QCheckBox(tr("Restore the last project on start"), startupCard);
    restoreSession_->setChecked(QSettings().value("ui/restoreSession", true).toBool());
    startupBody->addWidget(restoreSession_);
    startupBody->addWidget(note(tr("Turn this off to start with the bundled welcome project "
                                   "every time."),
        startupCard));
    layout->addWidget(startupCard);

    return pageShell(tr("General"), tr("Appearance, language and startup behaviour."), content);
}

QWidget* SettingsDialog::buildVideoPage()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* body = nullptr;
    auto* card = makeCard(tr("CANVAS"), content, &body);
    auto* form = makeForm();

    preset_ = new QComboBox(card);
    preset_->addItem("1920 × 1080 (Full HD)", QSize(1920, 1080));
    preset_->addItem("1280 × 720 (HD)", QSize(1280, 720));
    preset_->addItem("2560 × 1440 (QHD)", QSize(2560, 1440));
    preset_->addItem("3840 × 2160 (4K UHD)", QSize(3840, 2160));
    form->addRow(tr("Preset"), preset_);

    auto* sizeRow = new QHBoxLayout;
    width_ = new QSpinBox(card);
    width_->setRange(320, 7680);
    width_->setValue(1920);
    height_ = new QSpinBox(card);
    height_->setRange(180, 4320);
    height_->setValue(1080);
    aspect_ = new QLabel(card);
    aspect_->setObjectName("settingsPageHint");
    sizeRow->addWidget(width_);
    sizeRow->addWidget(new QLabel(QStringLiteral("×"), card));
    sizeRow->addWidget(height_);
    sizeRow->addSpacing(8);
    sizeRow->addWidget(aspect_);
    sizeRow->addStretch();
    form->addRow(tr("Resolution"), sizeRow);

    fps_ = new QComboBox(card);
    for (int value : {24, 25, 30, 50, 60}) {
        fps_->addItem(QString::number(value), value);
    }
    fps_->setCurrentIndex(fps_->findData(30));
    form->addRow(tr("Frame rate"), fps_);
    body->addLayout(form);
    body->addWidget(note(tr("The canvas size drives the compositor and the recording "
                            "resolution. Changes apply when you press OK."),
        card));
    layout->addWidget(card);

    connect(preset_, &QComboBox::currentIndexChanged, this, [this] {
        const QSize size = preset_->currentData().toSize();
        width_->setValue(size.width());
        height_->setValue(size.height());
    });
    connect(width_, &QSpinBox::valueChanged, this, [this] { updateAspectLabel(); });
    connect(height_, &QSpinBox::valueChanged, this, [this] { updateAspectLabel(); });
    updateAspectLabel();

    return pageShell(tr("Video"), tr("Canvas resolution and frame rate for the whole show."),
        content);
}

QWidget* SettingsDialog::buildAudioPage()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QVBoxLayout* deviceBody = nullptr;
    auto* deviceCard = makeCard(tr("INPUT DEVICE"), content, &deviceBody);
    auto* form = makeForm();
    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    auto* name = new QLabel(device.isNull() ? tr("No audio input device")
                                            : device.description(),
        deviceCard);
    name->setWordWrap(true);
    form->addRow(tr("Default input"), name);
    auto* rate = new QLabel(device.isNull() ? QStringLiteral("—")
                                            : tr("%1 Hz · %2 ch")
                                                  .arg(device.preferredFormat().sampleRate())
                                                  .arg(device.preferredFormat().channelCount()),
        deviceCard);
    rate->setObjectName("aboutValue");
    form->addRow(tr("Preferred format"), rate);
    auto* inputs = new QLabel(tr("%1 input(s) available").arg(QMediaDevices::audioInputs().size()),
        deviceCard);
    inputs->setObjectName("aboutValue");
    form->addRow(tr("Devices"), inputs);
    deviceBody->addLayout(form);
    deviceBody->addWidget(note(tr("The master meter reads this device. Pick a different system "
                                  "default input in Windows to change it."),
        deviceCard));
    layout->addWidget(deviceCard);

    QVBoxLayout* mixBody = nullptr;
    auto* mixCard = makeCard(tr("CHANNEL BEHAVIOUR"), content, &mixBody);
    auto* mixForm = makeForm();
    mixForm->addRow(tr("Gain"), note(tr("-60 to +12 dB per input, double-click a fader to reset."),
                            mixCard));
    mixForm->addRow(tr("Mute"), note(tr("Silences the channel and is saved with the project."),
                            mixCard));
    mixForm->addRow(tr("Solo"), note(tr("Monitoring state: dims every channel that is not "
                                        "soloed. Not saved with the project."),
                            mixCard));
    mixBody->addLayout(mixForm);
    layout->addWidget(mixCard);

    QVBoxLayout* limitBody = nullptr;
    auto* limitCard = makeCard(tr("CURRENT LIMITATION"), content, &limitBody);
    auto* warning = note(tr("Mixvera 0.4 meters audio but does not mix it into the program "
                            "output yet: recordings and streams are silent video. Channel "
                            "gain, mute and solo already run through AudioService, so the "
                            "mixer becomes audible when the engine work lands."),
        limitCard);
    limitBody->addWidget(warning);
    layout->addWidget(limitCard);

    return pageShell(tr("Audio"), tr("Monitoring, per-channel gain and the audio roadmap."),
        content);
}

QWidget* SettingsDialog::buildOutputPage()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QVBoxLayout* recordBody = nullptr;
    auto* recordCard = makeCard(tr("RECORDING"), content, &recordBody);
    auto* recordForm = makeForm();
    auto* directoryRow = new QHBoxLayout;
    recordingDirectory_ = new QLineEdit(recordCard);
    recordingDirectory_->setPlaceholderText(tr("recordings"));
    auto* browse = new QPushButton(tr("Browse…"), recordCard);
    directoryRow->addWidget(recordingDirectory_, 1);
    directoryRow->addWidget(browse);
    recordForm->addRow(tr("Folder"), directoryRow);
    recordBody->addLayout(recordForm);

    const QString ffmpeg = Recorder::ffmpegPath();
    auto* ffmpegLabel = new QLabel(ffmpeg.isEmpty()
            ? tr("ffmpeg not found — recording and streaming are disabled")
            : tr("ffmpeg: %1").arg(ffmpeg),
        recordCard);
    ffmpegLabel->setObjectName(ffmpeg.isEmpty() ? "settingsPageHint" : "aboutValue");
    ffmpegLabel->setWordWrap(true);
    recordBody->addWidget(ffmpegLabel);
    recordBody->addWidget(note(tr("Recordings are H.264 MP4 at the canvas resolution and frame "
                                  "rate."),
        recordCard));
    layout->addWidget(recordCard);

    QVBoxLayout* streamBody = nullptr;
    auto* streamCard = makeCard(tr("STREAMING"), content, &streamBody);
    auto* streamForm = makeForm();
    streamUrl_ = new QLineEdit(streamCard);
    streamUrl_->setPlaceholderText("rtmp://live.example.com/app/streamkey");
    streamForm->addRow(tr("RTMP URL"), streamUrl_);
    streamKey_ = new QLineEdit(streamCard);
    streamKey_->setEchoMode(QLineEdit::Password);
    streamKey_->setPlaceholderText(tr("Merged into the URL when set"));
    streamForm->addRow(tr("Stream key"), streamKey_);
    streamBody->addLayout(streamForm);
    streamBody->addWidget(note(tr("The URL and key are stored in the project file. Use a "
                                  "dedicated key when the project is shared."),
        streamCard));
    layout->addWidget(streamCard);

    connect(browse, &QPushButton::clicked, this, [this] {
        const QString directory = QFileDialog::getExistingDirectory(this,
            tr("Recording Folder"), recordingDirectory_->text());
        if (!directory.isEmpty()) {
            recordingDirectory_->setText(directory);
        }
    });

    return pageShell(tr("Output"), tr("Where recordings land and where the live stream goes."),
        content);
}

QWidget* SettingsDialog::buildShortcutsPage()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* body = nullptr;
    auto* card = makeCard(tr("KEYBOARD"), content, &body);
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);

    int row = 0;
    const auto add = [&](const QString& key, const QString& action) {
        auto* keyLabel = new QLabel(key, card);
        keyLabel->setObjectName("shortcutKey");
        keyLabel->setAlignment(Qt::AlignCenter);
        keyLabel->setMinimumWidth(112);
        auto* actionLabel = new QLabel(action, card);
        actionLabel->setObjectName("aboutValue");
        grid->addWidget(keyLabel, row, 0, Qt::AlignLeft);
        grid->addWidget(actionLabel, row, 1);
        ++row;
    };
    add(QStringLiteral("Space"), tr("Cut preview to program"));
    add(QStringLiteral("Enter"), tr("Run the selected transition"));
    add(QStringLiteral("F11"), tr("Toggle fullscreen"));
    add(QStringLiteral("Esc"), tr("Leave fullscreen"));
    add(QStringLiteral("Ctrl+R"), tr("Start / stop recording"));
    add(QStringLiteral("Ctrl+I"), tr("Add an input"));
    add(QStringLiteral("Ctrl+S"), tr("Save the project"));
    add(QStringLiteral("Ctrl+O"), tr("Open a project"));
    add(QStringLiteral("Ctrl+Q"), tr("Quit Mixvera"));
    body->addLayout(grid);
    layout->addWidget(card);

    QVBoxLayout* mouseBody = nullptr;
    auto* mouseCard = makeCard(tr("MOUSE"), content, &mouseBody);
    auto* mouseForm = makeForm();
    mouseForm->addRow(tr("T-Bar"), note(tr("Drag past halfway to finish a transition, release "
                                           "before halfway to return to preview."),
                                 mouseCard));
    mouseForm->addRow(tr("Fader"), note(tr("Double-click a channel fader to reset it to 0 dB."),
                                 mouseCard));
    mouseForm->addRow(tr("Input tile"), note(tr("Double-click opens input properties; right-click "
                                                "shows preview, cut, scene and remove."),
                                 mouseCard));
    mouseBody->addLayout(mouseForm);
    layout->addWidget(mouseCard);

    return pageShell(tr("Shortcuts"), tr("Bindings that are active in this build. They are not "
                                         "user-editable yet."),
        content);
}

QWidget* SettingsDialog::buildAboutPage()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QVBoxLayout* aboutBody = nullptr;
    auto* aboutCard = makeCard(tr("MIXVERA"), content, &aboutBody);
    auto* headline = new QLabel(tr("Live production switcher"), aboutCard);
    headline->setObjectName("settingsPageTitle");
    aboutBody->addWidget(headline);
    aboutBody->addWidget(note(tr("Scene compositing, capture inputs, CUT / AUTO / T-Bar "
                                 "transitions, a vMix style workspace and ffmpeg recording, "
                                 "built with Qt 6 Widgets."),
        aboutCard));

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(6);
    grid->setColumnStretch(1, 1);
    int row = 0;
    const auto add = [&](const QString& key, const QString& value) {
        auto* keyLabel = new QLabel(key, aboutCard);
        keyLabel->setObjectName("aboutKey");
        auto* valueLabel = new QLabel(value, aboutCard);
        valueLabel->setObjectName("aboutValue");
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(keyLabel, row, 0, Qt::AlignLeft | Qt::AlignTop);
        grid->addWidget(valueLabel, row, 1);
        ++row;
    };
    add(tr("Version"), QCoreApplication::applicationVersion());
    add(tr("Qt"), QString::fromLatin1(qVersion()));
    add(tr("Compiler"), compilerName());
    add(tr("Platform"), QSysInfo::prettyProductName());
    add(tr("Project file"), QDir(QStandardPaths::writableLocation(
                                     QStandardPaths::AppDataLocation))
                                .filePath("project.json"));
    add(tr("Licence"), tr("Third-party notices ship with the source tree"));
    aboutBody->addLayout(grid);
    layout->addWidget(aboutCard);

    QVBoxLayout* creditBody = nullptr;
    auto* creditCard = makeCard(tr("DESIGN REFERENCES"), content, &creditBody);
    creditBody->addWidget(note(tr("The workspace borrows publicly observable layout and colour "
                                  "roles from OBS Studio and vMix: preview on the left, program "
                                  "on the right, a vertical transition cluster, an input bar, a "
                                  "dockable mixer and category based settings. No OBS or vMix "
                                  "code, icons or resources are included."),
        creditCard));
    layout->addWidget(creditCard);

    return pageShell(tr("About"), tr("Build information and design provenance."), content);
}

void SettingsDialog::selectTheme(theme::ThemeId id, bool notify)
{
    theme_ = id;
    for (auto& [cardId, card] : themeCards_) {
        const bool selected = cardId == id;
        if (card->property("selected").toBool() != selected) {
            card->setProperty("selected", selected);
            card->style()->unpolish(card);
            card->style()->polish(card);
            card->update();
        }
    }
    if (notify) {
        emit themePreviewRequested(id);
    }
}

void SettingsDialog::updateAspectLabel()
{
    if (!aspect_) {
        return;
    }
    const int divisor = std::gcd(width_->value(), height_->value());
    if (divisor <= 0) {
        aspect_->clear();
        return;
    }
    aspect_->setText(tr("%1:%2").arg(width_->value() / divisor).arg(height_->value() / divisor));
}

theme::ThemeId SettingsDialog::theme() const
{
    return theme_;
}

void SettingsDialog::setTheme(theme::ThemeId id)
{
    selectTheme(id, false);
}

void SettingsDialog::setValues(int width, int height, int fps,
    const QString& recordingDirectory, const QString& streamUrl, const QString& streamKey)
{
    width_->setValue(width);
    height_->setValue(height);
    const int index = fps_->findData(fps);
    fps_->setCurrentIndex(index < 0 ? fps_->findData(30) : index);
    recordingDirectory_->setText(recordingDirectory);
    streamUrl_->setText(streamUrl);
    streamKey_->setText(streamKey);

    for (int row = 0; row < preset_->count(); ++row) {
        if (preset_->itemData(row).toSize() == QSize(width, height)) {
            preset_->setCurrentIndex(row);
            break;
        }
    }
    updateAspectLabel();
}

int SettingsDialog::canvasWidth() const { return width_->value(); }
int SettingsDialog::canvasHeight() const { return height_->value(); }
int SettingsDialog::fps() const { return fps_->currentData().toInt(); }
QString SettingsDialog::recordingDirectory() const { return recordingDirectory_->text().trimmed(); }

QString SettingsDialog::streamUrl() const
{
    return streamUrl_->text().trimmed();
}

QString SettingsDialog::streamKey() const { return streamKey_->text().trimmed(); }

QString SettingsDialog::language() const
{
    return language_ ? language_->currentData().toString() : QString();
}

void SettingsDialog::setLanguage(const QString& code)
{
    if (!language_) {
        return;
    }
    if (code.isEmpty()) {
        language_->setCurrentIndex(0);
        return;
    }
    const int index = language_->findData(code);
    if (index >= 0) {
        language_->setCurrentIndex(index);
    }
}

bool SettingsDialog::restoreSession() const
{
    return restoreSession_ && restoreSession_->isChecked();
}

} // namespace mixvera::ui
