#include "mixvera/ui/media/compositor.hpp"

#include "mixvera/ui/media/chroma_key.hpp"

#include <QLinearGradient>
#include <QPainter>
#include <QTransform>

#include <algorithm>

namespace mixvera::ui {

namespace {

constexpr qreal kPi = 3.14159265358979323846;

/// Chroma parameters are applied to the frame after the provider produces it,
/// so tuning them must not tear down and reopen a camera or capture device.
std::map<std::string, std::string> providerParams(const core::InputDescriptor& input)
{
    std::map<std::string, std::string> params;
    for (const auto& [key, value] : input.params) {
        if (key.compare(0, 6, "chroma") != 0) {
            params[key] = value;
        }
    }
    return params;
}

bool sameSettings(const core::InputDescriptor& left, const core::InputDescriptor& right)
{
    return left.kind == right.kind && left.name == right.name
        && providerParams(left) == providerParams(right);
}

/// Draws a whole-canvas image as a flying card: scaled and rotated about its
/// own centre. Used by the Fly transition.
void drawFlyingCard(QPainter& painter, const QImage& image, const QSizeF& canvas,
    const QPointF& center, qreal scale, qreal rotation, qreal opacity)
{
    if (image.isNull() || opacity <= 0.001) {
        return;
    }
    painter.save();
    painter.setOpacity(std::clamp(opacity, 0.0, 1.0));
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(rotation);
    transform.scale(scale, scale);
    transform.translate(-canvas.width() / 2.0, -canvas.height() / 2.0);
    painter.setTransform(transform);
    painter.drawImage(QPointF(0, 0), image);
    painter.restore();
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
    for (auto& [id, entry] : entries_) {
        if (entry.provider) {
            entry.provider->setCanvasSize(canvas_);
        }
    }
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
        if (it != entries_.end() && it->second.provider
            && it->second.descriptor.kind == input.kind
            && it->second.provider->update(input)) {
            it->second.descriptor = input;
            continue;
        }

        auto provider = createSourceProvider(input);
        if (!provider) {
            continue;
        }
        provider->setCanvasSize(canvas_);
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
        if (frame.isNull()) {
            continue;
        }
        const ChromaKeySettings chroma = chromaKeySettings(entry.descriptor);
        if (chroma.enabled && inputSupportsChromaKey(entry.descriptor.kind)) {
            frames_[id] = applyChromaKey(frame, chroma);
        } else {
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
    const std::vector<core::InputDescriptor>& inputs, bool opaqueBackground) const
{
    QImage canvas(canvas_, QImage::Format_ARGB32_Premultiplied);
    if (opaqueBackground) {
        canvas = black();
    } else {
        canvas.fill(Qt::transparent);
    }
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

QImage Compositor::overlayImage(const core::InputDescriptor& input,
    const std::vector<core::InputDescriptor>& inputs) const
{
    if (input.isScene()) {
        return renderScene(input, inputs, false);
    }
    return sourceImage(input.id);
}

QImage Compositor::applyOverlays(const QImage& base,
    const std::vector<core::InputDescriptor>& inputs,
    const std::vector<core::OverlayLayer>& overlays) const
{
    QImage canvas = base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (int layer = 1; layer <= 4; ++layer) {
        const core::OverlayLayer* overlay = nullptr;
        for (const auto& candidate : overlays) {
            if (candidate.layer == layer) {
                overlay = &candidate;
                break;
            }
        }
        if (!overlay || !overlay->enabled || overlay->inputId.empty()) {
            continue;
        }

        const core::InputDescriptor* input = nullptr;
        for (const auto& candidate : inputs) {
            if (candidate.id == overlay->inputId) {
                input = &candidate;
                break;
            }
        }
        if (!input) {
            continue;
        }

        const QImage image = overlayImage(*input, inputs);
        if (image.isNull()) {
            continue;
        }
        painter.setOpacity(1.0);
        drawCover(painter, image, QRectF(QPointF(0, 0), QSizeF(canvas_)));
    }
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

    case core::TransitionKind::Fly: {
        painter.fillRect(bounds, QColor("#050608"));
        drawFlyingCard(painter, from, QSizeF(canvas_),
            QPointF(bounds.width() * (0.50 - 0.30 * t),
                bounds.height() * (0.50 - 0.30 * t)),
            1.0 - 0.30 * t, -12.0 * t, 1.0 - t);
        drawFlyingCard(painter, to, QSizeF(canvas_),
            QPointF(bounds.width() * (0.82 - 0.32 * t),
                bounds.height() * (0.80 - 0.30 * t)),
            0.55 + 0.45 * t, 12.0 * (1.0 - t), t);
        break;
    }

    case core::TransitionKind::Cube: {
        // A horizontal cube spin: the outgoing face compresses to an edge,
        // then the incoming face opens from the same edge. Shading fakes the
        // unlit side of the cube without a 3D renderer.
        painter.fillRect(bounds, QColor("#050608"));
        const qreal halfWidth = bounds.width() / 2.0;
        const qreal phase = t < 0.5F ? t / 0.5F : (1.0F - t) / 0.5F;
        const qreal faceWidth = std::max<qreal>(1.0,
            bounds.width() * std::cos(phase * kPi / 2.0));
        if (t < 0.5F) {
            const qreal progress = t / 0.5F;
            const QRectF face(halfWidth - faceWidth / 2.0 + bounds.width() * 0.04 * progress,
                0, faceWidth, bounds.height());
            painter.setOpacity(1.0 - 0.12 * progress);
            painter.drawImage(face, from);
            painter.setOpacity(1.0);
            QLinearGradient shade(face.left(), 0, face.right(), 0);
            shade.setColorAt(0.0, QColor(0, 0, 0, 0));
            shade.setColorAt(1.0, QColor(0, 0, 0, 180));
            painter.fillRect(face, shade);
        } else {
            const qreal progress = (t - 0.5F) / 0.5F;
            const QRectF face(halfWidth - faceWidth / 2.0
                    - bounds.width() * 0.04 * (1.0 - progress),
                0, faceWidth, bounds.height());
            painter.setOpacity(0.88 + 0.12 * progress);
            painter.drawImage(face, to);
            painter.setOpacity(1.0);
            QLinearGradient shade(face.left(), 0, face.right(), 0);
            shade.setColorAt(0.0, QColor(0, 0, 0, 180));
            shade.setColorAt(1.0, QColor(0, 0, 0, 0));
            painter.fillRect(face, shade);
        }
        painter.setPen(QPen(QColor(255, 255, 255, 36), 1));
        painter.drawLine(QPointF(halfWidth, 0), QPointF(halfWidth, bounds.height()));
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
    QString detail = it->second.provider->detail();
    const ChromaKeySettings chroma = chromaKeySettings(it->second.descriptor);
    if (chroma.enabled && inputSupportsChromaKey(it->second.descriptor.kind)) {
        detail += QStringLiteral(" · Chroma: %1").arg(chromaKeyLabel(chroma));
    }
    return detail;
}

} // namespace mixvera::ui