#pragma once

#include <QWidget>
#include <string>
#include <vector>

class QComboBox;
class QLabel;
class QPushButton;

namespace mixvera::services {
class OutputService;
class ReplayService;
} // namespace mixvera::services

namespace mixvera::ui {

class OutputPanel final : public QWidget {
public:
    explicit OutputPanel(QWidget* parent = nullptr);

    void refresh();

private:
    struct OutputRow {
        std::string id;
        QLabel* dot = nullptr;
        QLabel* state = nullptr;
        QPushButton* button = nullptr;
    };

    std::vector<OutputRow> rows_;
    QPushButton* recordButton_ = nullptr;
    QPushButton* saveButton_ = nullptr;
    QComboBox* speedBox_ = nullptr;
    QLabel* replayState_ = nullptr;
    services::OutputService* outputs_ = nullptr;
    services::ReplayService* replay_ = nullptr;
};

} // namespace mixvera::ui
