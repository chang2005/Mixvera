#include "mixvera/ui/stage_view.hpp"

#include <QFont>
#include <QPainter>
#include <QTime>

#include <algorithm>
#include <cmath>

namespace mixvera::ui {

StageView::StageView(Role role, QWidget* parent)
    : QWidget(parent)
    , role_(role)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void StageView::setFrame(const QImage& frame)
{
    frame_ = frame;
    update();
}

void StageView::setSourceName(const QString& name)
{
    if (sourceName_ == name) {
        return;
    }
    sourceName_ = name;
    update();
}

void StageView::setTransition(float position)
{
    transition_ = std::clamp(position, 0.0F, 1.0F);
    update();
}

void StageView::setLive(bool live)
{
    live_ = live;
    update();
}

void StageView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#0B0D12"));

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QRectF target = bounds;
    if (!frame_.isNull()) {
        const QSizeF imageSize(frame_.size());
        const qreal scale = std::min(bounds.width() / imageSize.width(),
            bounds.height() / imageSize.height());
        const QSizeF scaled(imageSize.width() * scale, imageSize.height() * scale);
        target = QRectF(bounds.center().x() - scaled.width() / 2.0,
            bounds.center().y() - scaled.height() / 2.0, scaled.width(), scaled.height());
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(target, frame_);
    } else {
        painter.fillRect(bounds, QColor("#13141A"));
        QFont emptyFont(QStringLiteral("Bahnschrift"), 13, QFont::DemiBold);
        emptyFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
        painter.setFont(emptyFont);
        painter.setPen(QColor(150, 160, 175, 170));
        painter.drawText(bounds, Qt::AlignCenter, QStringLiteral("NO SIGNAL"));
    }

    // Role badge, drawn over a soft gradient so it stays readable on video.
    const QRectF header(bounds.left(), bounds.top(), 210, 34);
    QLinearGradient shade(header.topLeft(), header.bottomLeft());
    shade.setColorAt(0.0, QColor(0, 0, 0, 150));
    shade.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(header, shade);

    const QColor accent = role_ == Role::Program ? QColor("#E33B57") : QColor("#37D247");
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    painter.drawRoundedRect(QRectF(bounds.left() + 8, bounds.top() + 8, 46, 18), 2, 2);
    QFont badge(QStringLiteral("Bahnschrift"), 9, QFont::Bold);
    badge.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    painter.setFont(badge);
    painter.setPen(QColor("#0B0D12"));
    painter.drawText(QRectF(bounds.left() + 8, bounds.top() + 8, 46, 18), Qt::AlignCenter,
        role_ == Role::Program ? "PGM" : "PVW");

    QFont nameFont(QStringLiteral("Segoe UI"), 9);
    painter.setFont(nameFont);
    painter.setPen(QColor(230, 235, 245, 205));
    painter.drawText(QRectF(bounds.left() + 60, bounds.top() + 8, bounds.width() - 140, 18),
        Qt::AlignLeft | Qt::AlignVCenter, sourceName_);

    painter.setFont(QFont(QStringLiteral("Consolas"), 9));
    painter.setPen(QColor(200, 210, 225, 170));
    painter.drawText(QRectF(bounds.right() - 150, bounds.top() + 8, 142, 18),
        Qt::AlignRight | Qt::AlignVCenter, QTime::currentTime().toString("HH:mm:ss.zzz"));

    if (transition_ > 0.001F && transition_ < 0.999F) {
        const QString mix = QStringLiteral("MIX %1%").arg(
            static_cast<int>(std::lround(transition_ * 100.0F)));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(40, 76, 184, 210));
        const QRectF pill(bounds.left() + 10, bounds.bottom() - 30, 78, 20);
        painter.drawRoundedRect(pill, 3, 3);
        painter.setPen(QColor("#FFFFFF"));
        painter.drawText(pill, Qt::AlignCenter, mix);
    }

    painter.setPen(QPen(live_ ? accent : QColor("#323540"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(bounds);
}

} // namespace mixvera::ui