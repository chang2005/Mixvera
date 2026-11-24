#pragma once

#include <QLabel>

namespace mixvera::ui {

class PreviewView final : public QLabel {
    Q_OBJECT

public:
    explicit PreviewView(QWidget* parent = nullptr);
};

} // namespace mixvera::ui
