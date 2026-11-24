#include "mixvera/ui/platform/win_capture.hpp"

#ifdef Q_OS_WIN
#include <windows.h>

#include <QStringList>

#include <algorithm>
#include <vector>

namespace mixvera::ui {

namespace {

struct EnumState {
    DWORD processId = 0;
    QVector<CaptureWindow> windows;
};

BOOL CALLBACK collectWindow(HWND handle, LPARAM parameter)
{
    auto* state = reinterpret_cast<EnumState*>(parameter);
    if (!IsWindowVisible(handle)) {
        return TRUE;
    }
    if (GetWindowTextLengthW(handle) == 0) {
        return TRUE;
    }

    DWORD owner = 0;
    GetWindowThreadProcessId(handle, &owner);
    if (owner == state->processId) {
        return TRUE;
    }

    // Skip tool windows and windows without a caption.
    const LONG_PTR style = GetWindowLongPtrW(handle, GWL_STYLE);
    if ((style & WS_CHILD) != 0) {
        return TRUE;
    }
    if (IsIconic(handle)) {
        return TRUE;
    }

    wchar_t title[512] = {};
    GetWindowTextW(handle, title, 512);
    if (title[0] == L'\0') {
        return TRUE;
    }

    CaptureWindow entry;
    entry.handle = reinterpret_cast<quintptr>(handle);
    entry.title = QString::fromWCharArray(title);
    entry.process = QString::number(owner);
    state->windows.push_back(entry);
    return TRUE;
}

} // namespace

QVector<CaptureWindow> listCaptureWindows()
{
    EnumState state;
    state.processId = GetCurrentProcessId();
    EnumWindows(&collectWindow, reinterpret_cast<LPARAM>(&state));

    std::sort(state.windows.begin(), state.windows.end(),
        [](const auto& left, const auto& right) {
            return left.title.compare(right.title, Qt::CaseInsensitive) < 0;
        });
    return state.windows;
}

} // namespace mixvera::ui

#else

namespace mixvera::ui {

QVector<CaptureWindow> listCaptureWindows()
{
    return {};
}

} // namespace mixvera::ui

#endif