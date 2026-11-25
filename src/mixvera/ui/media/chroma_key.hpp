#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QColor>
#include <QImage>
#include <QString>

namespace mixvera::ui {

/// Per-input chroma key parameters. SinSam exposes this as the "blue/green
/// dedicated key" (蓝绿专用抠图); Mixvera keeps the same shape: a key colour,
/// a similarity threshold, an edge softness and spill suppression.
struct ChromaKeySettings {
    bool enabled = false;
    QColor color = QColor(0, 255, 0);
    int similarity = 35;
    int smoothness = 18;
    int spill = 60;
};

/// Reads chroma params from an input descriptor. Invalid or missing values
/// fall back to the defaults above.
ChromaKeySettings chromaKeySettings(const core::InputDescriptor& input);

/// Only capture and file-backed picture sources take a key. Titles, colours
/// and test patterns are generated internally and keep their alpha untouched.
bool inputSupportsChromaKey(core::InputKind kind);

/// "Green", "Blue" or the custom colour name, used in tiles and menus.
QString chromaKeyLabel(const ChromaKeySettings& settings);

/// Applies a chroma key to a frame and returns a premultiplied image with the
/// keyed colour made transparent. Pixels below the key similarity become
/// transparent, those above similarity + smoothness stay opaque, and the band
/// between them is feathered. Spill suppression pulls the key colour out of
/// the remaining edge pixels.
QImage applyChromaKey(const QImage& frame, const ChromaKeySettings& settings);

} // namespace mixvera::ui