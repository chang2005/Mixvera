#pragma once

#include "mixvera/core/domain_types.hpp"
#include "mixvera/ui/media/source_provider.hpp"

#include <QImage>
#include <QSize>
#include <QString>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mixvera::ui {

/// Owns one provider per source, caches the latest frames and composites
/// scenes into a canvas sized image. One refresh per display tick keeps screen
/// grabs and camera frames to a predictable rate.
class Compositor {
public:
    Compositor();
    ~Compositor();

    void setCanvasSize(QSize size);
    QSize canvasSize() const { return canvas_; }
    void setFrameRate(int fps);

    void sync(const std::vector<core::InputDescriptor>& inputs);
    void refresh(const std::vector<core::InputDescriptor>& inputs);

    QImage renderInput(const std::vector<core::InputDescriptor>& inputs,
        const std::string& id) const;
    QImage mix(const QImage& from, const QImage& to, core::TransitionKind kind,
        float position) const;
    /// Draws enabled overlay layers 1..4 on top of an already mixed frame.
    /// Scene overlays are rendered with a transparent background so their
    /// lower thirds and logos keep their alpha.
    QImage applyOverlays(const QImage& base,
        const std::vector<core::InputDescriptor>& inputs,
        const std::vector<core::OverlayLayer>& overlays) const;
    QImage black() const;
    QImage fadeToBlack(const QImage& source, float amount) const;

    QImage thumbnail(const std::string& id, QSize size) const;
    bool hasFrame(const std::string& id) const;
    QString detail(const std::string& id) const;

private:
    struct Entry {
        core::InputDescriptor descriptor;
        std::unique_ptr<SourceProvider> provider;
    };

    QImage sourceImage(const std::string& id) const;
    QImage overlayImage(const core::InputDescriptor& input,
        const std::vector<core::InputDescriptor>& inputs) const;
    void drawCover(QPainter& painter, const QImage& image, const QRectF& target) const;
    QImage renderScene(const core::InputDescriptor& scene,
        const std::vector<core::InputDescriptor>& inputs,
        bool opaqueBackground = true) const;

    QSize canvas_{1920, 1080};
    int frameRate_ = 30;
    std::map<std::string, Entry> entries_;
    std::map<std::string, QImage> frames_;
};

} // namespace mixvera::ui