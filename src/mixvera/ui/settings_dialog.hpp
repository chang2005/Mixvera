#pragma once

#include "mixvera/ui/theme.hpp"

#include <QDialog>
#include <QString>

#include <vector>

class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QStackedWidget;

namespace mixvera::ui {

/// OBS style settings window: category navigation on the left, a stacked page
/// on the right and a footer with Cancel / OK. Theme cards preview live on the
/// running window; Cancel restores the previous theme.
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

    theme::ThemeId theme() const;
    void setTheme(theme::ThemeId id);

    bool restoreSession() const;

signals:
    void themePreviewRequested(mixvera::ui::theme::ThemeId id);

private:
    QWidget* buildGeneralPage();
    QWidget* buildVideoPage();
    QWidget* buildAudioPage();
    QWidget* buildOutputPage();
    QWidget* buildShortcutsPage();
    QWidget* buildAboutPage();
    QWidget* pageShell(const QString& title, const QString& hint, QWidget* content);

    void selectTheme(theme::ThemeId id, bool notify);
    void updateAspectLabel();

    theme::ThemeId theme_ = theme::ThemeId::Charcoal;
    std::vector<std::pair<theme::ThemeId, QFrame*>> themeCards_;

    QListWidget* nav_ = nullptr;
    QStackedWidget* pages_ = nullptr;

    QComboBox* preset_ = nullptr;
    QSpinBox* width_ = nullptr;
    QSpinBox* height_ = nullptr;
    QComboBox* fps_ = nullptr;
    QLabel* aspect_ = nullptr;
    QComboBox* language_ = nullptr;
    QCheckBox* restoreSession_ = nullptr;
    QLineEdit* recordingDirectory_ = nullptr;
    QLineEdit* streamUrl_ = nullptr;
    QLineEdit* streamKey_ = nullptr;
};

} // namespace mixvera::ui
