#pragma once

#include <QStatusBar>

class QLabel;

namespace mixvera::ui {

class StatusBar final : public QStatusBar {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setEngineText(const QString& text);
    void setCanvasText(const QString& text);
    void setOutputText(const QString& text);
    void setProjectText(const QString& text);
    void setOnAir(bool live);
    void setClock(const QString& text);

private:
    QLabel* engine_ = nullptr;
    QLabel* canvas_ = nullptr;
    QLabel* outputs_ = nullptr;
    QLabel* project_ = nullptr;
    QLabel* onAir_ = nullptr;
    QLabel* clock_ = nullptr;
};

} // namespace mixvera::ui