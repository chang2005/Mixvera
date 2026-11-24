#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

namespace mixvera::ui {

/// A monitor showing a composed canvas. Paint only: the main window decides
/// what to render, which keeps PGM, PVW and record in sync from one tick.
class StageView final : public QWidget {
    Q_OBJECT

public:
    enum class Role {
        Program,
        Preview,
    };

    explicit StageView(Role role, QWidget* parent = nullptr);

    void setFrame(const QImage& frame);
    const QImage& frame() const { return frame_; }
    void setSourceName(const QString& name);
    void setTransition(float position);
    void setLive(bool live);

    QSize sizeHint() const override { return QSize(960, 540); }
    QSize minimumSizeHint() const override { return QSize(320, 180); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Role role_;
    QImage frame_;
    QString sourceName_;
    float transition_ = 0.0F;
    bool live_ = true;
};

} // namespace mixvera::ui