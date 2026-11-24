#pragma once

#include <QLabel>

namespace mixvera::ui {

class ProgramView final : public QLabel {
    Q_OBJECT

public:
    explicit ProgramView(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
