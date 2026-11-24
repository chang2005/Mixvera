#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

class QTimer;

namespace mixvera::ui {

class VideoSurface : public QWidget {
public:
    enum class Role {
        Program,
        Preview,
        Multiview,
    };

    explicit VideoSurface(QWidget* parent = nullptr);

    void setInputId(const QString& inputId);
    void setRole(Role role);
    void setTransition(float position);
    void setCompact(bool compact);
    void setSelected(bool selected);

    QString inputId() const { return inputId_; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void tick();
    QColor accentColor() const;

    QTimer* timer_ = nullptr;
    QString inputId_ = "camera-1";
    Role role_ = Role::Program;
    float transition_ = 0.0F;
    bool compact_ = false;
    bool selected_ = false;
    qreal phase_ = 0.0;
    int frame_ = 0;
};

} // namespace mixvera::ui
