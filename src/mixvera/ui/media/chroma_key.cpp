#include "mixvera/ui/media/chroma_key.hpp"

#include <QCoreApplication>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace mixvera::ui {

namespace {

QString param(const core::InputDescriptor& input, const char* key)
{
    const auto it = input.params.find(key);
    return it == input.params.end() ? QString() : QString::fromStdString(it->second);
}

int intParam(const core::InputDescriptor& input, const char* key, int fallback)
{
    bool ok = false;
    const int value = param(input, key).toInt(&ok);
    return ok ? std::clamp(value, 0, 100) : fallback;
}

QColor colorParam(const core::InputDescriptor& input, const char* key, const QColor& fallback)
{
    const QColor value(param(input, key));
    return value.isValid() ? value : fallback;
}

double distanceTo(const QColor& color, int red, int green, int blue)
{
    const int sum = color.red() + color.green() + color.blue();
    const int pixelSum = red + green + blue;
    if (sum <= 0 || pixelSum <= 0) {
        return 1.0;
    }
    const double dr = static_cast<double>(red) / pixelSum - static_cast<double>(color.red()) / sum;
    const double dg = static_cast<double>(green) / pixelSum - static_cast<double>(color.green()) / sum;
    const double db = static_cast<double>(blue) / pixelSum - static_cast<double>(color.blue()) / sum;
    return std::sqrt(dr * dr + dg * dg + db * db);
}

double smoothstep(double value)
{
    const double t = std::clamp(value, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

} // namespace

ChromaKeySettings chromaKeySettings(const core::InputDescriptor& input)
{
    ChromaKeySettings settings;
    settings.enabled = param(input, "chromaEnabled") == "1";
    settings.color = colorParam(input, "chromaColor", QColor(0, 255, 0));
    settings.similarity = intParam(input, "chromaSimilarity", 35);
    settings.smoothness = intParam(input, "chromaSmoothness", 18);
    settings.spill = intParam(input, "chromaSpill", 60);
    return settings;
}

bool inputSupportsChromaKey(core::InputKind kind)
{
    switch (kind) {
    case core::InputKind::ScreenCapture:
    case core::InputKind::WindowCapture:
    case core::InputKind::Camera:
    case core::InputKind::Image:
    case core::InputKind::MediaFile:
        return true;
    default:
        return false;
    }
}

QString chromaKeyLabel(const ChromaKeySettings& settings)
{
    if (settings.color.green() >= 180 && settings.color.red() < 100
        && settings.color.blue() < 100) {
        return QCoreApplication::translate("ChromaKey", "Green");
    }
    if (settings.color.blue() >= 180 && settings.color.red() < 100
        && settings.color.green() < 100) {
        return QCoreApplication::translate("ChromaKey", "Blue");
    }
    return settings.color.name().toUpper();
}

QImage applyChromaKey(const QImage& frame, const ChromaKeySettings& settings)
{
    if (frame.isNull() || !settings.enabled) {
        return frame;
    }

    QImage image = frame.convertToFormat(QImage::Format_ARGB32);
    if (image.isNull()) {
        return frame;
    }

    // Compare normalised RGB chromaticity rather than raw Cb/Cr: a dark,
    // unevenly lit green screen keeps the same chromaticity as a bright one,
    // so shadows and folds key out with the rest of the backdrop.
    const double threshold = 0.03 + settings.similarity * 0.0055;
    const double feather = 0.004 + settings.smoothness * 0.0022;
    const double spill = std::clamp(settings.spill, 0, 100) / 100.0;

    const bool greenKey = settings.color.green() > settings.color.red() + 60
        && settings.color.green() > settings.color.blue() + 60;
    const bool blueKey = settings.color.blue() > settings.color.red() + 60
        && settings.color.blue() > settings.color.green() + 60;

    for (int y = 0; y < image.height(); ++y) {
        auto* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = row[x];
            const int sourceAlpha = qAlpha(pixel);
            if (sourceAlpha == 0) {
                continue;
            }
            int red = qRed(pixel);
            int green = qGreen(pixel);
            int blue = qBlue(pixel);

            // Near-black pixels have no reliable hue; keep them.
            if (red + green + blue < 24) {
                continue;
            }

            const double distance = distanceTo(settings.color, red, green, blue);
            double coverage = 1.0;
            if (distance <= threshold) {
                coverage = 0.0;
            } else if (distance < threshold + feather) {
                coverage = smoothstep((distance - threshold) / feather);
            }
            if (coverage <= 0.0) {
                row[x] = qRgba(red, green, blue, 0);
                continue;
            }

            // Despill: pull the key channel down towards the other two so a
            // green fringe on hair or shoulders does not survive the key.
            if (greenKey && spill > 0.0 && green > std::max(red, blue)) {
                green -= static_cast<int>((green - std::max(red, blue)) * spill);
            } else if (blueKey && spill > 0.0 && blue > std::max(red, green)) {
                blue -= static_cast<int>((blue - std::max(red, green)) * spill);
            }

            const int alpha = static_cast<int>(
                std::lround(coverage * static_cast<double>(sourceAlpha)));
            row[x] = qRgba(std::clamp(red, 0, 255), std::clamp(green, 0, 255),
                std::clamp(blue, 0, 255), std::clamp(alpha, 0, 255));
        }
    }

    return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

} // namespace mixvera::ui