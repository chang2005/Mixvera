#pragma once

#include <QWidget>

namespace mixvera::ui {

class MixerPanel final : public QWidget {
public:
    explicit MixerPanel(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
