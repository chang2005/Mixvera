#include "mixvera/ui/settings_dialog.hpp"

#include "mixvera/ui/media/recorder.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace mixvera::ui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Mixvera Settings"));
    setMinimumWidth(520);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    auto* videoForm = new QFormLayout;
    preset_ = new QComboBox(this);
    preset_->addItem("1920 × 1080 (Full HD)", QSize(1920, 1080));
    preset_->addItem("1280 × 720 (HD)", QSize(1280, 720));
    preset_->addItem("2560 × 1440 (QHD)", QSize(2560, 1440));
    preset_->addItem("3840 × 2160 (4K UHD)", QSize(3840, 2160));
    videoForm->addRow(tr("Canvas preset"), preset_);

    auto* sizeRow = new QHBoxLayout;
    width_ = new QSpinBox(this);
    width_->setRange(320, 7680);
    width_->setValue(1920);
    height_ = new QSpinBox(this);
    height_->setRange(180, 4320);
    height_->setValue(1080);
    sizeRow->addWidget(width_);
    sizeRow->addWidget(new QLabel("×", this));
    sizeRow->addWidget(height_);
    sizeRow->addStretch();
    videoForm->addRow(tr("Canvas size"), sizeRow);

    fps_ = new QComboBox(this);
    for (int value : {24, 25, 30, 50, 60}) {
        fps_->addItem(QString::number(value), value);
    }
    fps_->setCurrentIndex(fps_->findData(30));
    videoForm->addRow(tr("Frame rate"), fps_);
    root->addLayout(videoForm);

    root->addWidget(new QLabel(tr("RECORDING"), this));
    auto* recordRow = new QHBoxLayout;
    recordingDirectory_ = new QLineEdit(this);
    recordingDirectory_->setPlaceholderText(tr("recordings"));
    auto* browse = new QPushButton(tr("Browse…"), this);
    recordRow->addWidget(recordingDirectory_, 1);
    recordRow->addWidget(browse);
    root->addLayout(recordRow);

    root->addWidget(new QLabel(tr("STREAMING"), this));
    auto* streamForm = new QFormLayout;
    streamUrl_ = new QLineEdit(this);
    streamUrl_->setPlaceholderText("rtmp://live.example.com/app/streamkey");
    streamForm->addRow(tr("RTMP URL"), streamUrl_);
    streamKey_ = new QLineEdit(this);
    streamKey_->setEchoMode(QLineEdit::Password);
    streamKey_->setPlaceholderText(tr("Merged into the URL when set"));
    streamForm->addRow(tr("Stream key"), streamKey_);
    root->addLayout(streamForm);

    const QString ffmpeg = Recorder::ffmpegPath();
    auto* ffmpegLabel = new QLabel(this);
    ffmpegLabel->setObjectName("controlLabel");
    ffmpegLabel->setText(ffmpeg.isEmpty()
            ? tr("ffmpeg not found — recording and streaming are disabled")
            : tr("ffmpeg: %1").arg(ffmpeg));
    ffmpegLabel->setWordWrap(true);
    root->addWidget(ffmpegLabel);
    root->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(preset_, &QComboBox::currentIndexChanged, this, [this] {
        const QSize size = preset_->currentData().toSize();
        width_->setValue(size.width());
        height_->setValue(size.height());
    });
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString directory = QFileDialog::getExistingDirectory(this,
            tr("Recording Folder"), recordingDirectory_->text());
        if (!directory.isEmpty()) {
            recordingDirectory_->setText(directory);
        }
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
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

} // namespace mixvera::ui