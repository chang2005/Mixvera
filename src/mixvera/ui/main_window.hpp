#pragma once

#include <QMainWindow>
#include <memory>

class QString;

namespace mixvera::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onCut();
    void onAuto();
    void onAutoTick();
    void onTBarChanged(int value);
    void onTBarReleased();
    void onPreviewSelected(const QString& inputId);
    void refresh();

private:
    QWidget* buildHeader();
    QWidget* buildMonitors();
    QWidget* buildTransitionCard();
    QWidget* buildSidebar();
    void updateTransition(float position);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mixvera::ui
