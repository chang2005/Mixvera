#pragma once

#include <QString>
#include <QVector>

#include <cstdint>

namespace mixvera::ui {

struct CaptureWindow {
    quintptr handle = 0;
    QString title;
    QString process;
};

/// Visible top level windows that can be captured, ordered by title. The list
/// excludes our own process and windows without a title.
QVector<CaptureWindow> listCaptureWindows();

} // namespace mixvera::ui