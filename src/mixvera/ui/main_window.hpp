#pragma once

#include <QMainWindow>
#include <memory>

class QLabel;
class QPushButton;
class QSlider;
class QStatusBar;

namespace mixvera::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onTake();
    void onTBarChanged(int value);
    void onReplaySave();

private:
    void buildMenuBar();
    void buildCentralWidget();
    void buildDocks();
    void refreshStatus();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mixvera::ui
