#include "mixvera/ui/video_surface.hpp"

#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QTime>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace mixvera::ui {

VideoSurface::VideoSurface(QWidget* parent)
    : QWidget(parent)
    , timer_(new QTimer(this))
{
    setMinimumSize(160, 90);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
    timer_->setInterval(40);
    connect(timer_, &QTimer::timeout, this, [this] { tick(); });
    timer_->start();
}

void VideoSurface::setInputId(const QString& inputId)
{
    if (inputId_ == inputId) {
        return;
    }
    inputId_ = inputId;
    update();
}

void VideoSurface::setRole(Role role)
{
    role_ = role;
    update();
}

void VideoSurface::setTransition(float position)
{
    transition_ = std::clamp(position, 0.0F, 1.0F);
    update();
}

void VideoSurface::setCompact(bool compact)
{
    compact_ = compact;
    update();
}

void VideoSurface::setSelected(bool selected)
{
    selected_ = selected;
    update();
}

QColor VideoSurface::accentColor() const
{
    switch (role_) {
    case Role::Program:
        return QColor("#ff4d5e");
    case Role::Preview:
        return QColor("#38d996");
    case Role::Multiview:
        return selected_ ? QColor("#53c8ff") : QColor("#344252");
    }
    return QColor("#344252");
}

void VideoSurface::tick()
{
    phase_ += 0.035;
    ++frame_;
    update();
}

void VideoSurface::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    timer_->start();
}

void VideoSurface::hideEvent(QHideEvent* event)
{
    timer_->stop();
    QWidget::hideEvent(event);
}

void VideoSurface::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF bounds = rect().adjusted(2, 2, -2, -2);
    painter.fillRect(rect(), QColor("#05070b"));

    QLinearGradient background(bounds.topLeft(), bounds.bottomRight());
    background.setColorAt(0.0, QColor("#101a26"));
    background.setColorAt(0.55, QColor("#0b111a"));
    background.setColorAt(1.0, QColor("#070a0f"));
    painter.fillRect(bounds, background);

    const int seed = static_cast<int>(qHash(inputId_) & 0x7FFFFFFFU);
    const int bars = compact_ ? 7 : 9;
    const qreal barWidth = bounds.width() / bars;
    for (int i = 0; i < bars; ++i) {
        const int hue = (seed + i * 41) % 360;
        const int value = 150 + ((seed >> (i % 8)) & 0x3f);
        QColor color = QColor::fromHsv(hue, 150, std::min(value, 235));
        color.setAlpha(compact_ ? 92 : 120);
        QRectF bar(bounds.left() + i * barWidth, bounds.top(), barWidth + 1, bounds.height());
        painter.fillRect(bar, color);
    }

    painter.save();
    painter.setClipRect(bounds);
    for (int band = 0; band < 5; ++band) {
        const qreal y = bounds.top()
            + std::fmod(phase_ * (34.0 + band * 9.0) + band * 47.0, std::max(1.0, bounds.height()));
        QLinearGradient bandGradient(bounds.left(), y, bounds.right(), y);
        bandGradient.setColorAt(0.0, QColor(255, 255, 255, 0));
        bandGradient.setColorAt(0.5, QColor(255, 255, 255, compact_ ? 8 : 14));
        bandGradient.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.fillRect(QRectF(bounds.left(), y, bounds.width(), compact_ ? 8 : 14), bandGradient);
    }
    painter.restore();

    painter.setPen(QPen(QColor(255, 255, 255, 12), 1));
    const int scanStep = compact_ ? 6 : 10;
    for (int y = static_cast<int>(bounds.top()); y < static_cast<int>(bounds.bottom()); y += scanStep) {
        painter.drawLine(QPointF(bounds.left(), y), QPointF(bounds.right(), y));
    }

    const QRectF safe = bounds.adjusted(bounds.width() * 0.08, bounds.height() * 0.12,
        -bounds.width() * 0.08, -bounds.height() * 0.12);
    painter.setPen(QPen(QColor(255, 255, 255, compact_ ? 18 : 30), 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(safe, compact_ ? 3 : 6, compact_ ? 3 : 6);

    const QString displayName = inputId_.toUpper().replace('-', ' ');
    QFont nameFont(QStringLiteral("Bahnschrift"), compact_ ? 10 : 28, QFont::DemiBold);
    nameFont.setLetterSpacing(QFont::AbsoluteSpacing, compact_ ? 0.8 : 2.0);
    painter.setFont(nameFont);
    painter.setPen(QColor(255, 255, 255, compact_ ? 215 : 245));
    const QRectF nameRect = bounds.adjusted(12, 0, -12, 0);
    painter.drawText(nameRect, Qt::AlignCenter, displayName);

    if (!compact_) {
        QFont subtitleFont(QStringLiteral("Bahnschrift"), 10);
        subtitleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.5);
        painter.setFont(subtitleFont);
        painter.setPen(QColor(205, 220, 235, 150));
        painter.drawText(nameRect.adjusted(0, 52, 0, 0), Qt::AlignCenter, "SIMULATED FEED / DEMO ENGINE");
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(accentColor());
    const QRectF statusRect(bounds.left() + 10, bounds.top() + 10, compact_ ? 54 : 96, compact_ ? 18 : 24);
    painter.drawRoundedRect(statusRect, 4, 4);
    QFont badgeFont(QStringLiteral("Bahnschrift"), compact_ ? 8 : 10, QFont::DemiBold);
    painter.setFont(badgeFont);
    painter.setPen(QColor("#071015"));
    const QString badge = role_ == Role::Program ? "PGM" : role_ == Role::Preview ? "PVW" : "INPUT";
    painter.drawText(statusRect, Qt::AlignCenter, badge);

    QFont monoFont(QStringLiteral("Consolas"), compact_ ? 8 : 11);
    painter.setFont(monoFont);
    painter.setPen(QColor(230, 240, 250, 180));
    const QString timecode = QTime::currentTime().toString("HH:mm:ss")
        + QString(":%1").arg(frame_ % 25, 2, 10, QLatin1Char('0'));
    if (!compact_) {
        painter.drawText(bounds.adjusted(0, 10, -12, 0), Qt::AlignRight | Qt::AlignTop, timecode);
    }

    if (transition_ > 0.001F) {
        const qreal x = bounds.left() + bounds.width() * transition_;
        QLinearGradient wipe(x - 28, 0, x + 28, 0);
        wipe.setColorAt(0.0, QColor(0, 0, 0, 0));
        wipe.setColorAt(0.5, QColor(83, 200, 255, 55));
        wipe.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(QRectF(x - 28, bounds.top(), 56, bounds.height()), wipe);
        painter.setPen(QPen(QColor("#53c8ff"), compact_ ? 1 : 2));
        painter.drawLine(QPointF(x, bounds.top()), QPointF(x, bounds.bottom()));
        const QString mix = QString("MIX %1%").arg(static_cast<int>(transition_ * 100));
        painter.setFont(monoFont);
        painter.setPen(QColor("#8ee7ff"));
        painter.drawText(bounds.adjusted(0, 0, 0, -10), Qt::AlignRight | Qt::AlignBottom, mix);
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(accentColor(), compact_ ? 1 : 2));
    painter.drawRoundedRect(bounds, compact_ ? 4 : 8, compact_ ? 4 : 8);
}

} // namespace mixvera::ui
