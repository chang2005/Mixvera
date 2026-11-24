#include "mixvera/ui/mixer_panel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace mixvera::ui {

namespace {

class LevelMeter final : public QWidget {
public:
    explicit LevelMeter(QWidget* parent = nullptr, bool vertical = true)
        : QWidget(parent)
        , vertical_(vertical)
    {
        setFixedWidth(vertical ? 12 : 120);
        if (!vertical) {
            setFixedHeight(12);
        }
    }

    void setLevel(float db)
    {
        const float clamped = std::clamp(db, -60.0F, 0.0F);
        // Release ballistics: fast rise, slow fall.
        level_ = clamped > level_ ? clamped : level_ - (level_ - clamped) * 0.12F;
        if (clamped > level_) {
            level_ = clamped;
        }
        if (level_ < -59.5F) {
            level_ = -60.0F;
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor("#0B0D12"));

        const qreal fraction = (level_ + 60.0F) / 60.0F;
        if (vertical_) {
            const qreal barHeight = height() * std::clamp(fraction, 0.0, 1.0);
            QRectF bar(2, height() - barHeight, width() - 4, barHeight);
            QLinearGradient gradient(0, height(), 0, 0);
            gradient.setColorAt(0.0, QColor("#25A231"));
            gradient.setColorAt(0.72, QColor("#37D247"));
            gradient.setColorAt(0.88, QColor("#E5AF24"));
            gradient.setColorAt(1.0, QColor("#C01C37"));
            painter.fillRect(bar, gradient);
        } else {
            const qreal widthValue = width() * std::clamp(fraction, 0.0, 1.0);
            QLinearGradient gradient(0, 0, width(), 0);
            gradient.setColorAt(0.0, QColor("#25A231"));
            gradient.setColorAt(0.72, QColor("#37D247"));
            gradient.setColorAt(0.88, QColor("#E5AF24"));
            gradient.setColorAt(1.0, QColor("#C01C37"));
            painter.fillRect(QRectF(1, 2, widthValue, height() - 4), gradient);
        }

        painter.setPen(QColor("#323540"));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

private:
    bool vertical_ = true;
    float level_ = -60.0F;
};

QLabel* caption(const QString& text, const char* objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QString::fromLatin1(objectName));
    return label;
}

} // namespace

MixerPanel::MixerPanel(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("mixerPanel");
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(6);

    auto* header = new QVBoxLayout;
    header->setSpacing(2);
    header->addWidget(caption(tr("AUDIO MIXER"), "dockTitle", this));
    deviceLabel_ = caption(tr("MASTER"), "controlLabel", this);
    header->addWidget(deviceLabel_);
    header->addStretch();
    root->addLayout(header);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    stripArea_ = new QWidget(scroll_);
    stripArea_->setObjectName("stripArea");
    auto* strips = new QHBoxLayout(stripArea_);
    strips->setContentsMargins(0, 0, 0, 0);
    strips->setSpacing(5);
    strips->addStretch();
    scroll_->setWidget(stripArea_);
    root->addWidget(scroll_, 1);

    auto* master = new QFrame(this);
    master->setObjectName("channelStrip");
    master->setFixedWidth(74);
    auto* masterLayout = new QVBoxLayout(master);
    masterLayout->setContentsMargins(5, 4, 5, 4);
    masterLayout->setSpacing(3);
    masterLayout->addWidget(caption(tr("MASTER"), "channelName", master));
    auto* meterRow = new QHBoxLayout;
    meterRow->setSpacing(4);
    auto* meter = new LevelMeter(master);
    meter->setObjectName("masterMeter");
    masterMeter_ = meter;
    meterRow->addWidget(meter);
    auto* dbLabel = caption(tr("-60 dB"), "channelDb", master);
    meterRow->addWidget(dbLabel);
    masterLayout->addLayout(meterRow, 1);
    root->addWidget(master);

    connect(meter, &QWidget::destroyed, this, [this] { masterMeter_ = nullptr; });
}

void MixerPanel::setChannels(const std::vector<core::InputDescriptor>& inputs,
    const std::vector<core::AudioChannelState>& channels)
{
    inputs_ = inputs;
    channels_ = channels;
    rebuildChannels();
}

void MixerPanel::rebuildChannels()
{
    auto* layout = qobject_cast<QHBoxLayout*>(stripArea_->layout());
    if (!layout) {
        return;
    }
    while (layout->count() > 1) {
        QLayoutItem* item = layout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (const auto& input : inputs_) {
        if (!input.hasAudio) {
            continue;
        }

        float gain = 0.0F;
        bool muted = false;
        for (const auto& channel : channels_) {
            if (channel.inputId == input.id) {
                gain = channel.gainDb;
                muted = channel.muted;
                break;
            }
        }

        auto* strip = new QFrame(stripArea_);
        strip->setObjectName("channelStrip");
        strip->setFixedWidth(70);
        auto* stripLayout = new QVBoxLayout(strip);
        stripLayout->setContentsMargins(4, 4, 4, 4);
        stripLayout->setSpacing(3);

        auto* name = caption(QString::fromStdString(input.name), "channelName", strip);
        name->setToolTip(QString::fromStdString(input.name));
        stripLayout->addWidget(name);

        auto* slider = new QSlider(Qt::Vertical, strip);
        slider->setRange(-60, 12);
        slider->setValue(static_cast<int>(std::lround(gain)));
        slider->setFixedHeight(64);
        stripLayout->addWidget(slider, 0, Qt::AlignHCenter);

        auto* db = caption(QString("%1 dB").arg(static_cast<int>(std::lround(gain))),
            "channelDb", strip);
        db->setAlignment(Qt::AlignCenter);
        stripLayout->addWidget(db);

        auto* mute = new QToolButton(strip);
        mute->setObjectName("muteButton");
        mute->setText(tr("M"));
        mute->setCheckable(true);
        mute->setChecked(muted);
        mute->setToolTip(tr("Mute"));
        stripLayout->addWidget(mute);

        const QString id = QString::fromStdString(input.id);
        connect(slider, &QSlider::valueChanged, this, [this, id, db](int value) {
            db->setText(QString("%1 dB").arg(value));
            emit gainChanged(id, static_cast<float>(value));
        });
        connect(mute, &QToolButton::toggled, this, [this, id](bool checked) {
            emit muteChanged(id, checked);
        });

        layout->insertWidget(layout->count() - 1, strip);
    }
}

void MixerPanel::setMasterLevel(float levelDb)
{
    if (auto* meter = dynamic_cast<LevelMeter*>(masterMeter_)) {
        meter->setLevel(levelDb);
        meter->update();
    }
    auto* master = masterMeter_ ? masterMeter_->parentWidget() : nullptr;
    if (master) {
        const auto labels = master->findChildren<QLabel*>("channelDb");
        for (auto* label : labels) {
            label->setText(QString("%1 dB").arg(static_cast<int>(std::lround(levelDb))));
        }
    }
}

void MixerPanel::setAudioDeviceStatus(const QString& text, bool active)
{
    deviceLabel_->setText(text);
    deviceLabel_->setStyleSheet(active ? "color:#59D966;" : "color:#969696;");
}

} // namespace mixvera::ui