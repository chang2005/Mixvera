#include "mixvera/ui/media/compositor.hpp"

#include <QPainter>

#include <algorithm>

namespace mixvera::ui {

namespace {

bool sameSettings(const core::InputDescriptor& left, const core::InputDescriptor& right)
{
    return left.kind == right.kind && left.params == right.params
        && left.name == right.name;
}

} // namespace

Compositor::Compositor() = default;
Compositor::~Compositor() = default;

void Compositor::setCanvasSize(QSize size)
{
    if (!size.isValid() || size.isEmpty()) {
        return;
    }
    canvas_ = size;
}

void Compositor::setFrameRate(int fps)
{
    frameRate_ = std::clamp(fps, 1, 120);
}

void Compositor::sync(const std::vector<core::InputDescriptor>& inputs)
{
    std::map<std::string, bool> seen;
    for (const auto& input : inputs) {
        seen[input.id] = true;
        if (input.isScene()) {
            continue;
        }
        const auto it = entries_.find(input.id);
        if (it != entries_.end() && sameSettings(it->second.descriptor, input)) {
            it->second.descriptor = input;
            continue;
        }

        auto provider = createSourceProvider(input);
        if (!provider) {
            continue;
        }
        Entry entry;
        entry.descriptor = input;
        entry.provider = std::move(provider);
        entries_[input.id] = std::move(entry);
    }

    for (auto it = entries_.begin(); it != entries_.end();) {
        if (seen.find(it->first) == seen.end()) {
            frames_.erase(it->first);
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void Compositor::refresh(const std::vector<core::InputDescriptor>& inputs)
{
    sync(inputs);
    for (auto& [id, entry] : entries_) {
        if (!entry.provider) {
            continue;
        }
        const QImage frame = entry.provider->frame();
        if (!frame.isNull()) {
            frames_[id] = frame;
        }
    }
}

QImage Compositor::sourceImage(const std::string& id) const
{
    const auto it = frames_.find(id);
    return it == frames_.end() ? QImage() : it->second;
}

void Compositor::drawCover(QPainter& painter, const QImage& image,
    const QRectF& target) const
{
    if (image.isNull() || target.width() <= 0.0 || target.height() <= 0.0) {
        return;
    }
    const qreal scale = std::max(target.width() / image.width(),
        target.height() / image.height());
    const QSizeF scaled(image.width() * scale, image.height() * scale);
    const QRectF source(target.center().x() - scaled.width() / 2.0,
        target.center().y() - scaled.height() / 2.0, scaled.width(), scaled.height());
    painter.drawImage(source, image);
}

QImage Compositor::black() const
{
    QImage image(canvas_, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor("#050608"));
    return image;
}

QImage Compositor::renderInput(const std::vector<core::InputDescriptor>& inputs,
    const std::string& id) const
{
    const auto it = std::find_if(inputs.begin(), inputs.end(),
        [&](const auto& input) { return input.id == id; });
    if (it == inputs.end()) {
        return black();
    }
    if (it->isScene()) {
        return renderScene(*it, inputs);
    }

    QImage canvas = black();
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    drawCover(painter, sourceImage(id), QRectF(QPointF(0, 0), QSizeF(canvas_)));
    return canvas;
}

QImage Compositor::renderScene(const core::InputDescriptor& scene,
    const std::vector<core::InputDescriptor>& inputs) const
{
    QImage canvas = black();
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal width = canvas_.width();
    const qreal height = canvas_.height();
    for (const auto& item : scene.items) {
        if (!item.visible || item.opacity <= 0.001F) {
            continue;
        }
        const QImage image = sourceImage(item.inputId);
        if (image.isNull()) {
            continue;
        }
        painter.setOpacity(std::clamp(item.opacity, 0.0F, 1.0F));
        const QRectF target(item.x * width, item.y * height, item.width * width,
            item.height * height);
        drawCover(painter, image, target);
    }
    painter.setOpacity(1.0);
    return canvas;
}

QImage Compositor::mix(const QImage& from, const QImage& to, core::TransitionKind kind,
    float position) const
{
    const float t = std::clamp(position, 0.0F, 1.0F);
    QImage canvas = black();
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF bounds(0, 0, canvas_.width(), canvas_.height());

    switch (kind) {
    case core::TransitionKind::Cut:
        painter.drawImage(bounds, t >= 0.5F ? to : from);
        break;

    case core::TransitionKind::Fade:
        painter.drawImage(bounds, from);
        painter.setOpacity(t);
        painter.drawImage(bounds, to);
        break;

    case core::TransitionKind::Wipe: {
        painter.drawImage(bounds, from);
        painter.save();
        painter.setClipRect(QRectF(0, 0, bounds.width() * t, bounds.height()));
        painter.drawImage(bounds, to);
        painter.restore();
        if (t > 0.001F && t < 0.999F) {
            painter.setPen(QPen(QColor(255, 255, 255, 60), 2));
            painter.drawLine(QPointF(bounds.width() * t, 0),
                QPointF(bounds.width() * t, bounds.height()));
        }
        break;
    }

    case core::TransitionKind::Slide:
        painter.drawImage(bounds.translated(-bounds.width() * t, 0), from);
        painter.drawImage(bounds.translated(bounds.width() * (1.0 - t), 0), to);
        break;

    case core::TransitionKind::Zoom: {
        painter.setOpacity(1.0F - t);
        const qreal fromScale = 1.0 + 0.18 * t;
        const QSizeF fromSize(bounds.width() * fromScale, bounds.height() * fromScale);
        painter.drawImage(QRectF(bounds.center().x() - fromSize.width() / 2.0,
                             bounds.center().y() - fromSize.height() / 2.0,
                             fromSize.width(), fromSize.height()),
            from);
        painter.setOpacity(t);
        const qreal toScale = 0.82 + 0.18 * t;
        const QSizeF toSize(bounds.width() * toScale, bounds.height() * toScale);
        painter.drawImage(QRectF(bounds.center().x() - toSize.width() / 2.0,
                             bounds.center().y() - toSize.height() / 2.0, toSize.width(),
                             toSize.height()),
            to);
        break;
    }
    }
    painter.setOpacity(1.0);
    return canvas;
}

QImage Compositor::fadeToBlack(const QImage& source, float amount) const
{
    if (amount <= 0.001F) {
        return source;
    }
    QImage canvas = source;
    QPainter painter(&canvas);
    painter.setOpacity(std::clamp(amount, 0.0F, 1.0F));
    painter.fillRect(canvas.rect(), QColor("#000000"));
    return canvas;
}

QImage Compositor::thumbnail(const std::string& id, QSize size) const
{
    const QImage image = sourceImage(id);
    QImage target(size, QImage::Format_ARGB32_Premultiplied);
    target.fill(QColor("#0b0d12"));
    if (image.isNull()) {
        return target;
    }
    QPainter painter(&target);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    drawCover(painter, image, QRectF(QPointF(0, 0), QSizeF(size)));
    return target;
}

bool Compositor::hasFrame(const std::string& id) const
{
    return frames_.find(id) != frames_.end();
}

QString Compositor::detail(const std::string& id) const
{
    const auto it = entries_.find(id);
    if (it == entries_.end() || !it->second.provider) {
        return QString();
    }
    return it->second.provider->detail();
}

} // namespace mixvera::ui