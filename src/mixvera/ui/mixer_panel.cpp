#include "mixvera/ui/mixer_panel.hpp"

#include "mixvera/core/application.hpp"
#include "mixvera/services/audio_service.hpp"
#include "mixvera/services/input_service.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>

namespace mixvera::ui {

MixerPanel::MixerPanel(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(150);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(8);

    auto& app = core::Application::instance();
    auto& inputs = app.services().get<services::InputService>();
    auto* audio = &app.services().get<services::AudioService>();

    for (const auto& input : inputs.inputs()) {
        const QString inputId = QString::fromStdString(input.id);
        auto* strip = new QFrame(this);
        strip->setObjectName("mixerStrip");
        auto* stripLayout = new QVBoxLayout(strip);
        stripLayout->setContentsMargins(8, 6, 8, 6);
        stripLayout->setSpacing(4);

        auto* name = new QLabel(QString::fromStdString(input.name), strip);
        name->setObjectName("mixerName");
        name->setAlignment(Qt::AlignCenter);

        auto* gain = new QLabel("0 dB", strip);
        gain->setObjectName("mixerGain");
        gain->setAlignment(Qt::AlignCenter);

        auto* fader = new QSlider(Qt::Vertical, strip);
        fader->setRange(-60, 12);
        fader->setValue(0);
        fader->setObjectName("mixerFader");
        fader->setMinimumHeight(70);

        auto* mute = new QPushButton("MUTE", strip);
        mute->setObjectName("muteButton");
        mute->setCheckable(true);

        stripLayout->addWidget(name);
        stripLayout->addWidget(gain);
        stripLayout->addWidget(fader, 1, Qt::AlignHCenter);
        stripLayout->addWidget(mute);

        layout->addWidget(strip);

        QObject::connect(fader, &QSlider::valueChanged, this, [audio, inputId, gain](int value) {
            audio->setGain(inputId.toStdString(), static_cast<float>(value));
            gain->setText(QString("%1 dB").arg(value));
        });
        QObject::connect(mute, &QPushButton::toggled, this, [audio, inputId, mute](bool muted) {
            audio->setMuted(inputId.toStdString(), muted);
            mute->setText(muted ? "MUTED" : "MUTE");
        });
    }

    auto* routing = new QFrame(this);
    routing->setObjectName("routingPanel");
    auto* routingLayout = new QVBoxLayout(routing);
    routingLayout->setContentsMargins(10, 6, 10, 6);
    routingLayout->setSpacing(5);

    auto* routingHeader = new QHBoxLayout;
    auto* routingTitle = new QLabel("BUS ROUTING", routing);
    routingTitle->setObjectName("cardTitle");
    auto* routingHint = new QLabel("6 MIX BUSES", routing);
    routingHint->setObjectName("cardHint");
    routingHeader->addWidget(routingTitle);
    routingHeader->addStretch();
    routingHeader->addWidget(routingHint);
    routingLayout->addLayout(routingHeader);

    auto* matrix = new QGridLayout;
    matrix->setHorizontalSpacing(5);
    matrix->setVerticalSpacing(4);
    auto* sourceHeader = new QLabel("SOURCE", routing);
    sourceHeader->setObjectName("routeHeader");
    matrix->addWidget(sourceHeader, 0, 0);
    for (int busIndex = 0; busIndex < 6; ++busIndex) {
        auto* busHeader = new QLabel(QString("BUS %1").arg(busIndex + 1), routing);
        busHeader->setObjectName("routeHeader");
        busHeader->setAlignment(Qt::AlignCenter);
        matrix->addWidget(busHeader, 0, busIndex + 1);
    }

    int matrixRow = 1;
    for (const auto& input : inputs.inputs()) {
        std::uint32_t mask = 1;
        for (const auto& channel : audio->channels()) {
            if (channel.inputId == input.id) {
                mask = channel.mixerMask;
                break;
            }
        }

        auto* source = new QLabel(QString::fromStdString(input.name).toUpper(), routing);
        source->setObjectName("routeSource");
        matrix->addWidget(source, matrixRow, 0);

        for (int busIndex = 0; busIndex < 6; ++busIndex) {
            auto* route = new QPushButton(routing);
            route->setObjectName("routeButton");
            route->setCheckable(true);
            route->setChecked((mask & (1U << busIndex)) != 0);
            route->setCursor(Qt::PointingHandCursor);
            route->setFixedHeight(20);
            route->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            if (busIndex == 0) {
                route->setToolTip("Program bus");
            }
            matrix->addWidget(route, matrixRow, busIndex + 1);

            const std::string inputId = input.id;
            QObject::connect(route, &QPushButton::toggled, this,
                [audio, inputId, busIndex](bool enabled) {
                    std::uint32_t current = 0;
                    for (const auto& channel : audio->channels()) {
                        if (channel.inputId == inputId) {
                            current = channel.mixerMask;
                            break;
                        }
                    }
                    const std::uint32_t bit = 1U << busIndex;
                    current = enabled ? (current | bit) : (current & ~bit);
                    audio->setMixerMask(inputId, current);
                });
        }
        ++matrixRow;
    }
    routingLayout->addLayout(matrix);
    layout->addWidget(routing, 1);

    auto* master = new QFrame(this);
    master->setObjectName("masterStrip");
    auto* masterLayout = new QVBoxLayout(master);
    masterLayout->setContentsMargins(12, 6, 12, 6);
    auto* masterName = new QLabel("MASTER", master);
    masterName->setObjectName("mixerName");
    masterName->setAlignment(Qt::AlignCenter);
    auto* masterGain = new QLabel("-6 dB", master);
    masterGain->setObjectName("mixerGain");
    masterGain->setAlignment(Qt::AlignCenter);
    auto* masterFader = new QSlider(Qt::Vertical, master);
    masterFader->setRange(-60, 12);
    masterFader->setValue(-6);
    masterFader->setObjectName("masterFader");
    auto* bus = new QLabel("BUS 1", master);
    bus->setObjectName("busBadge");
    bus->setAlignment(Qt::AlignCenter);
    masterLayout->addWidget(masterName);
    masterLayout->addWidget(masterGain);
    masterLayout->addWidget(masterFader, 1, Qt::AlignHCenter);
    masterLayout->addWidget(bus);
    layout->addWidget(master);

    QObject::connect(masterFader, &QSlider::valueChanged, masterGain,
        [masterGain](int value) { masterGain->setText(QString("%1 dB").arg(value)); });
}

} // namespace mixvera::ui
