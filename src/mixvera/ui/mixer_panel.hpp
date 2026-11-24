#pragma once

#include <QWidget>

namespace mixvera::ui {

class MixerPanel final : public QWidget {
    Q_OBJECT

public:
    explicit MixerPanel(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
