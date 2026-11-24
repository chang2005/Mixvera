#pragma once

#include <QSlider>

namespace mixvera::ui {

class TBarWidget final : public QSlider {
    Q_OBJECT

public:
    explicit TBarWidget(QWidget* parent = nullptr);

signals:
    void positionChanged(int value);
};

} // namespace mixvera::ui
