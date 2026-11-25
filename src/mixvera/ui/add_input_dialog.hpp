#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QColor>
#include <QDialog>
#include <QHash>

#include <map>
#include <string>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QWidget;

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
    void pickChromaColour();
    void updateChromaControls();
    void updateTitleStyleControls();

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
    QComboBox* titleStyle_ = nullptr;
    QFormLayout* titleForm_ = nullptr;
    QWidget* packageRow_ = nullptr;
    QLineEdit* packagePath_ = nullptr;
    QPushButton* packageBrowse_ = nullptr;
    QPushButton* editElements_ = nullptr;
    QLabel* packageInfo_ = nullptr;
    QLineEdit* subtitle_ = nullptr;
    QPlainTextEdit* captions_ = nullptr;
    QSpinBox* tickerSpeed_ = nullptr;
    QSpinBox* textSize_ = nullptr;
    QComboBox* align_ = nullptr;
    QPushButton* colour_ = nullptr;
    QSpinBox* countdownSeconds_ = nullptr;
    QLineEdit* competition_ = nullptr;
    QLineEdit* homeName_ = nullptr;
    QLineEdit* awayName_ = nullptr;
    QSpinBox* homeScore_ = nullptr;
    QSpinBox* awayScore_ = nullptr;
    QColor colourValue_ = QColor("#284cb8");

    QWidget* chromaPanel_ = nullptr;
    QCheckBox* chromaEnabled_ = nullptr;
    QComboBox* chromaPreset_ = nullptr;
    QPushButton* chromaColour_ = nullptr;
    QSpinBox* chromaSimilarity_ = nullptr;
    QSpinBox* chromaSmoothness_ = nullptr;
    QSpinBox* chromaSpill_ = nullptr;
    QColor chromaValue_ = QColor("#00ff00");

    QHash<QString, QString> elementTexts_;
    QHash<QString, QColor> elementColours_;
    QHash<QString, QString> elementImages_;

    QString id_;
    std::map<std::string, std::string> baseParams_;
};

} // namespace mixvera::ui