#pragma once

#include <QWidget>

namespace mixvera::ui {

class MultiviewPanel final : public QWidget {
    Q_OBJECT

public:
    explicit MultiviewPanel(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
