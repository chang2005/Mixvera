#include "mixvera/ui/mixer_panel.hpp"

#include "mixvera/ui/theme.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace mixvera::ui {

namespace {

QString formatDb(int value)
{
    if (value <= -60) {
        return QStringLiteral("-∞");
    }
    return QString("%1").arg(value > 0 ? QString("+%1").arg(value) : QString::number(value));
}

QString elide(const QString& text, int width, const QFontMetrics& metrics)
{
    return metrics.elidedText(text, Qt::ElideRight, width);
}

void repolish(QWidget* widget)
{
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

/// Vertical peak meter. With \a scale the widget also paints a -6 / -12 / -24
/// dB ruler, which is what the master strip uses.
class LevelMeter final : public QWidget {
public:
    explicit LevelMeter(QWidget* parent = nullptr, bool scale = false)
        : QWidget(parent)
        , scale_(scale)
    {
        setFixedWidth(scale ? 36 : 16);
        setMinimumHeight(110);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }

    void setLevel(float db)
    {
        const float clamped = std::clamp(db, -60.0F, 0.0F);
        level_ = clamped > level_ ? clamped : level_ - (level_ - clamped) * 0.12F;
        peak_ = std::max(clamped, peak_ - 0.9F);
        if (level_ < -59.5F) {
            level_ = -60.0F;
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const theme::Palette& p = theme::palette();
        QPainter painter(this);
        painter.fillRect(rect(), QColor(p.monitor));

        const int barLeft = scale_ ? width() - 14 : 1;
        const QRectF bar(barLeft, 8, 10, std::max(8, height() - 16));

        const qreal fraction = std::clamp((level_ + 60.0F) / 60.0F, 0.0F, 1.0F);
        const qreal barHeight = bar.height() * fraction;
        if (barHeight > 0.5) {
            QRectF filled(bar.left(), bar.bottom() - barHeight, bar.width(), barHeight);
            QLinearGradient gradient(0, bar.bottom(), 0, bar.top());
            gradient.setColorAt(0.0, QColor(p.meterLow));
            gradient.setColorAt(0.667, QColor(p.meterMid));
            gradient.setColorAt(0.85, QColor(p.meterHigh));
            gradient.setColorAt(1.0, QColor(p.meterPeak));
            painter.fillRect(filled, gradient);
        }

        if (peak_ > -59.5F) {
            const qreal y = bar.bottom()
                - bar.height() * std::clamp((peak_ + 60.0F) / 60.0F, 0.0F, 1.0F);
            painter.setPen(QPen(QColor(p.meterPeak), 1));
            painter.drawLine(QPointF(bar.left(), y), QPointF(bar.right(), y));
        }

        painter.setPen(QPen(QColor(p.border), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(bar.adjusted(0, 0, -1, -1));

        if (scale_) {
            QFont font(QStringLiteral("Consolas"), 7);
            painter.setFont(font);
            for (int db : {0, -6, -12, -24, -48}) {
                const qreal y = bar.bottom()
                    - bar.height() * std::clamp((60.0 + db) / 60.0, 0.0, 1.0);
                painter.setPen(QPen(QColor(p.borderHover), 1));
                painter.drawLine(QPointF(bar.left() - 3, y), QPointF(bar.left() - 1, y));
                painter.setPen(QColor(p.textDim));
                painter.drawText(QRectF(0, y - 6, bar.left() - 5, 12),
                    Qt::AlignRight | Qt::AlignVCenter, QString::number(db));
            }
        }
    }

private:
    bool scale_ = false;
    float level_ = -60.0F;
    float peak_ = -60.0F;
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
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto* header = new QHBoxLayout;
    header->setSpacing(6);
    header->addWidget(caption(tr("AUDIO MIXER"), "dockTitle", this));
    soloChip_ = caption(QString(), "audioDeviceChip", this);
    soloChip_->setProperty("active", true);
    soloChip_->hide();
    header->addWidget(soloChip_);
    header->addStretch();
    deviceLabel_ = caption(tr("NO AUDIO INPUT"), "audioDeviceChip", this);
    deviceLabel_->setProperty("active", false);
    header->addWidget(deviceLabel_);
    root->addLayout(header);

    auto* body = new QHBoxLayout;
    body->setSpacing(6);

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
    body->addWidget(scroll_, 1);

    auto* master = new QFrame(this);
    master->setObjectName("masterStrip");
    master->setFixedWidth(92);
    auto* masterLayout = new QVBoxLayout(master);
    masterLayout->setContentsMargins(6, 6, 6, 6);
    masterLayout->setSpacing(4);
    auto* masterTitle = caption(tr("MASTER"), "channelName", master);
    masterTitle->setAlignment(Qt::AlignCenter);
    masterLayout->addWidget(masterTitle);

    auto* meter = new LevelMeter(master, true);
    meter->setObjectName("masterMeter");
    masterMeter_ = meter;
    masterLayout->addWidget(meter, 1, Qt::AlignHCenter);

    masterDb_ = caption(QStringLiteral("-∞"), "channelDb", master);
    masterDb_->setAlignment(Qt::AlignCenter);
    masterLayout->addWidget(masterDb_);

    auto* bus = caption(tr("PGM"), "mixerHint", master);
    bus->setAlignment(Qt::AlignCenter);
    masterLayout->addWidget(bus);
    body->addWidget(master);
    root->addLayout(body, 1);

    auto* hint = new QLabel(tr("Meter follows the default input device. Channel gain and mute "
                               "are saved with the project; audio is not mixed into the "
                               "recording yet."),
        this);
    hint->setObjectName("mixerHint");
    hint->setWordWrap(true);
    root->addWidget(hint);

    connect(meter, &QWidget::destroyed, this, [this] { masterMeter_ = nullptr; });
}

void MixerPanel::setChannels(const std::vector<core::InputDescriptor>& inputs,
    const std::vector<core::AudioChannelState>& channels)
{
    inputs_ = inputs;
    channels_ = channels;
    rebuildChannels();
    updateSoloState();
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

    int audioChannels = 0;
    for (const auto& input : inputs_) {
        if (!input.hasAudio) {
            continue;
        }

        float gain = 0.0F;
        bool muted = false;
        bool solo = false;
        for (const auto& channel : channels_) {
            if (channel.inputId == input.id) {
                gain = channel.gainDb;
                muted = channel.muted;
                solo = channel.solo;
                break;
            }
        }

        auto* strip = new QFrame(stripArea_);
        strip->setObjectName("channelStrip");
        strip->setProperty("inputId", QString::fromStdString(input.id));
        strip->setFixedWidth(74);
        auto* col = new QVBoxLayout(strip);
        col->setContentsMargins(4, 5, 4, 5);
        col->setSpacing(3);

        auto* index = caption(QString("%1").arg(audioChannels + 1, 2, 10, QLatin1Char('0')),
            "channelIndex", strip);
        index->setAlignment(Qt::AlignCenter);
        col->addWidget(index);

        const QString name = QString::fromStdString(input.name);
        auto* nameLabel = caption(elide(name, 64, fontMetrics()), "channelName", strip);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setToolTip(name);
        col->addWidget(nameLabel);

        auto* db = caption(formatDb(static_cast<int>(std::lround(gain))), "channelDb", strip);
        db->setAlignment(Qt::AlignCenter);
        col->addWidget(db);

        auto* slider = new QSlider(Qt::Vertical, strip);
        slider->setObjectName("channelFader");
        slider->setRange(-60, 12);
        slider->setValue(static_cast<int>(std::lround(gain)));
        slider->setPageStep(3);
        slider->setFixedWidth(26);
        slider->setToolTip(tr("Channel gain. Double-click to reset to 0 dB."));
        slider->installEventFilter(this);
        col->addWidget(slider, 1, Qt::AlignHCenter);

        auto* buttons = new QHBoxLayout;
        buttons->setSpacing(3);
        auto* mute = new QToolButton(strip);
        mute->setObjectName("muteButton");
        mute->setText(QStringLiteral("M"));
        mute->setCheckable(true);
        mute->setChecked(muted);
        mute->setToolTip(tr("Mute this channel"));
        mute->setFixedSize(28, 20);
        auto* soloButton = new QToolButton(strip);
        soloButton->setObjectName("soloButton");
        soloButton->setText(QStringLiteral("S"));
        soloButton->setCheckable(true);
        soloButton->setChecked(solo);
        soloButton->setToolTip(tr("Solo: monitor this channel alone"));
        soloButton->setFixedSize(28, 20);
        buttons->addWidget(mute);
        buttons->addWidget(soloButton);
        col->addLayout(buttons);

        const QString id = QString::fromStdString(input.id);
        connect(slider, &QSlider::valueChanged, this, [this, id, db](int value) {
            db->setText(formatDb(value));
            for (auto& channel : channels_) {
                if (channel.inputId == id.toStdString()) {
                    channel.gainDb = static_cast<float>(value);
                    break;
                }
            }
            emit gainChanged(id, static_cast<float>(value));
        });
        connect(mute, &QToolButton::toggled, this, [this, id](bool checked) {
            for (auto& channel : channels_) {
                if (channel.inputId == id.toStdString()) {
                    channel.muted = checked;
                    break;
                }
            }
            emit muteChanged(id, checked);
        });
        connect(soloButton, &QToolButton::toggled, this, [this, id](bool checked) {
            for (auto& channel : channels_) {
                if (channel.inputId == id.toStdString()) {
                    channel.solo = checked;
                    break;
                }
            }
            updateSoloState();
            emit soloChanged(id, checked);
        });

        layout->insertWidget(layout->count() - 1, strip);
        ++audioChannels;
    }

    if (audioChannels == 0) {
        auto* hint = new QLabel(tr("No inputs with audio yet. Camera and media inputs "
                                   "appear here automatically."),
            stripArea_);
        hint->setObjectName("controlLabel");
        hint->setWordWrap(true);
        hint->setMinimumWidth(180);
        layout->insertWidget(0, hint);
    }
}

void MixerPanel::updateSoloState()
{
    int soloCount = 0;
    for (const auto& channel : channels_) {
        if (channel.solo) {
            ++soloCount;
        }
    }

    if (soloChip_) {
        soloChip_->setText(tr("SOLO %1").arg(soloCount));
        soloChip_->setVisible(soloCount > 0);
        soloChip_->setToolTip(tr("Solo is a monitoring state; it is not stored in the project."));
    }

    const auto strips = stripArea_->findChildren<QFrame*>(QStringLiteral("channelStrip"));
    for (auto* strip : strips) {
        const QString id = strip->property("inputId").toString();
        bool solo = false;
        for (const auto& channel : channels_) {
            if (QString::fromStdString(channel.inputId) == id) {
                solo = channel.solo;
                break;
            }
        }
        const bool dimmed = soloCount > 0 && !solo;
        if (strip->property("dimmed").toBool() != dimmed) {
            strip->setProperty("dimmed", dimmed);
            repolish(strip);
        }
    }
}

void MixerPanel::setMasterLevel(float levelDb)
{
    if (auto* meter = dynamic_cast<LevelMeter*>(masterMeter_)) {
        meter->setLevel(levelDb);
    }
    if (masterDb_) {
        masterDb_->setText(levelDb <= -59.5F ? QStringLiteral("-∞")
                                             : QString("%1").arg(static_cast<int>(std::lround(levelDb))));
    }
}

bool MixerPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        if (auto* slider = qobject_cast<QSlider*>(watched)) {
            if (slider->objectName() == QLatin1String("channelFader")) {
                slider->setValue(0);
                return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void MixerPanel::setAudioDeviceStatus(const QString& text, bool active)
{
    if (!deviceLabel_) {
        return;
    }
    deviceLabel_->setToolTip(active
            ? tr("Live level from %1").arg(text)
            : tr("No capture device: the master meter stays at -∞"));
    deviceLabel_->setText(deviceLabel_->fontMetrics().elidedText(text, Qt::ElideRight, 176));
    if (deviceLabel_->property("active").toBool() != active) {
        deviceLabel_->setProperty("active", active);
        repolish(deviceLabel_);
    }
}

} // namespace mixvera::ui
