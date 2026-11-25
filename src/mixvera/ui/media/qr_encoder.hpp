#pragma once

#include <QVector>

namespace mixvera::ui {

/// Encodes UTF-8 text in QR byte mode using error-correction level M and the
/// smallest version (1-10) that fits, selecting the mask with the lowest
/// standard penalty. Returns the module matrix without the quiet zone
/// (true = dark); an empty matrix means the text exceeds the supported size
/// (213 bytes at level M).
QVector<QVector<bool>> qrMatrix(const QString& text);

} // namespace mixvera::ui