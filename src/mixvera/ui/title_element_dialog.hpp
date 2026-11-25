#pragma once

#include "mixvera/ui/media/title_package.hpp"

#include <QColor>
#include <QDialog>
#include <QElapsedTimer>
#include <QHash>
#include <QSize>
#include <QVector>

class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTimer;

namespace mixvera::ui {

/// Editors for the named text, colour, image and ticker objects of a vMix or
/// SinSam GT title package. The dialog owns a live preview and returns only the
/// per-element overrides, leaving every untouched default in the package.
class TitleElementDialog final : public QDialog {
    Q_OBJECT

public:
    TitleElementDialog(const TitlePackage& package,
        const QHash<QString, QString>& textValues,
        const QHash<QString, QColor>& colourValues,
        const QHash<QString, QString>& imageValues, QSize canvas,
        QWidget* parent = nullptr);

    QHash<QString, QString> textValues() const;
    QHash<QString, QColor> colourValues() const;
    QHash<QString, QString> imageValues() const;

private:
    void buildUi();
    void refreshList();
    void selectElement(int row);
    void updatePreview();
    void editText();
    void chooseColour();
    void resetColour();
    void editImagePath();
    void browseImage();
    void resetImage();
    void showColour(const QColor& colour);
    const TitleElementInfo* currentInfo() const;

    TitlePackage package_;
    QVector<TitleElementInfo> elements_;
    QHash<QString, QString> texts_;
    QHash<QString, QColor> colours_;
    QHash<QString, QString> images_;
    QSize canvas_;
    int currentRow_ = -1;
    bool updating_ = false;

    QListWidget* list_ = nullptr;
    QLabel* nameLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLabel* propertyHint_ = nullptr;
    QLabel* preview_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QPlainTextEdit* textEdit_ = nullptr;
    QPushButton* colourButton_ = nullptr;
    QLabel* colourValue_ = nullptr;
    QPushButton* colourReset_ = nullptr;
    QLineEdit* imagePath_ = nullptr;
    QPushButton* imageBrowse_ = nullptr;
    QPushButton* imageReset_ = nullptr;
    QTimer* previewTimer_ = nullptr;
    QElapsedTimer previewClock_;
};

} // namespace mixvera::ui