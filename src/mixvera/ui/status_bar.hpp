#pragma once

#include <QStatusBar>
#include <memory>

namespace mixvera::ui {

class StatusBar final : public QStatusBar {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);
    ~StatusBar() override;

    void setTransition(float position);
    void setOutputsActive(int active, int total);

private:
    class impl_t;
    std::unique_ptr<impl_t> impl_;
};

} // namespace mixvera::ui
