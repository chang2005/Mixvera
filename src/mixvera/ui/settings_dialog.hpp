#pragma once

#include <QDialog>
#include <QString>

class QComboBox;
class QLineEdit;
class QSpinBox;

namespace mixvera::ui {

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setValues(int width, int height, int fps, const QString& recordingDirectory,
        const QString& streamUrl, const QString& streamKey);

    int canvasWidth() const;
    int canvasHeight() const;
    int fps() const;
    QString recordingDirectory() const;
    QString streamUrl() const;
    QString streamKey() const;
    QString language() const;
    void setLanguage(const QString& code);

private:
    QComboBox* preset_ = nullptr;
    QSpinBox* width_ = nullptr;
    QSpinBox* height_ = nullptr;
    QComboBox* fps_ = nullptr;
    QComboBox* language_ = nullptr;
    QLineEdit* recordingDirectory_ = nullptr;
    QLineEdit* streamUrl_ = nullptr;
    QLineEdit* streamKey_ = nullptr;
};

} // namespace mixvera::ui