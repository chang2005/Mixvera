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

    const QRectF full = rect().adjusted(0, 0, -1, -1);
    constexpr qreal headerHeight = 24.0;
    const QRectF header(full.left(), full.top(), full.width(), headerHeight);
    const QRectF bounds(full.left(), header.bottom() + 1.0, full.width(),
        std::max<qreal>(0.0, full.height() - headerHeight - 1.0));

    // vMix uses a solid coloured title strip: orange for preview, green for
    // program. Keeping it outside the video area avoids covering the frame.
    const QColor accent = role_ == Role::Program ? QColor("#006400") : QColor("#FF8C00");
    painter.fillRect(header, accent);

    QFont headerFont(QStringLiteral("Segoe UI"), 9, QFont::DemiBold);
    painter.setFont(headerFont);
    painter.setPen(QColor("#FFFFFF"));
    const QString roleText = role_ == Role::Program ? tr("PROGRAM") : tr("PREVIEW");
    const QString name = sourceName_.isEmpty() ? tr("No input") : sourceName_;
    const QString label = roleText + QStringLiteral("  ·  ") + name;
    const QRectF textArea = header.adjusted(8, 0, -86, 0);
    painter.drawText(textArea, Qt::AlignLeft | Qt::AlignVCenter,
        painter.fontMetrics().elidedText(label, Qt::ElideRight,
            static_cast<int>(textArea.width())));

    painter.setFont(QFont(QStringLiteral("Consolas"), 8));
    painter.setPen(QColor(255, 255, 255, 195));
    painter.drawText(header.adjusted(0, 0, -8, 0), Qt::AlignRight | Qt::AlignVCenter,
        QTime::currentTime().toString("HH:mm:ss"));

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
        painter.fillRect(bounds, QColor("#13171B"));
        QFont emptyFont(QStringLiteral("Bahnschrift"), 13, QFont::DemiBold);
        emptyFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
        painter.setFont(emptyFont);
        painter.setPen(QColor(150, 160, 175, 170));
        painter.drawText(bounds, Qt::AlignCenter, tr("NO SIGNAL"));
    }

    if (transition_ > 0.001F && transition_ < 0.999F) {
        const QString mix = QStringLiteral("MIX %1%").arg(
            static_cast<int>(std::lround(transition_ * 100.0F)));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(40, 76, 184, 220));
        const QRectF pill(bounds.left() + 10, bounds.bottom() - 28, 82, 19);
        painter.drawRoundedRect(pill, 2, 2);
        painter.setPen(QColor("#FFFFFF"));
        painter.drawText(pill, Qt::AlignCenter, mix);
    }

    painter.setPen(QPen(live_ ? accent.lighter(115) : accent, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(full);
}

} // namespace mixvera::ui