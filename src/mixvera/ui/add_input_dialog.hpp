#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QColor>
#include <QDialog>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;

namespace mixvera::ui {

/// Creates a new input, or edits the capture settings of an existing one.
class AddInputDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AddInputDialog(QWidget* parent = nullptr);

    void loadFrom(const core::InputDescriptor& input);
    void lockKind(bool locked);

    core::InputDescriptor descriptor() const;

private:
    void buildUi();
    void updatePage();
    void refreshName();
    void browseFile();
    void pickColour();

    QComboBox* type_ = nullptr;
    QLineEdit* name_ = nullptr;
    QStackedWidget* pages_ = nullptr;

    QComboBox* monitor_ = nullptr;
    QComboBox* window_ = nullptr;
    QComboBox* camera_ = nullptr;
    QComboBox* pattern_ = nullptr;
    QLineEdit* file_ = nullptr;
    QPushButton* browse_ = nullptr;
    QPlainTextEdit* text_ = nullptr;
    QSpinBox* textSize_ = nullptr;
    QComboBox* align_ = nullptr;
    QPushButton* colour_ = nullptr;
    QColor colourValue_ = QColor("#284cb8");

    QString id_;
};

} // namespace mixvera::ui