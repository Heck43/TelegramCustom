#pragma once

#ifdef Q_OS_WIN

#include <windows.h>
#include <wtsapi32.h>
#include "custom_features/custom_settings.hpp"

#pragma comment(lib, "wtsapi32.lib")

namespace CustomFeatures {

class WindowsSessionLockWatcher {
public:
    static void Register(HWND hWnd, std::function<void()> onLockCallback) {
        if (!hWnd) return;
        _callback = onLockCallback;
        WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);
    }

    static void Unregister(HWND hWnd) {
        if (hWnd) {
            WTSUnRegisterSessionNotification(hWnd);
        }
    }

    static bool HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_WTSSESSION_CHANGE) {
            if (wParam == WTS_SESSION_LOCK) {
                if (GetConfig().autoLockOnWindowsLock && _callback) {
                    _callback();
                    return true;
                }
            }
        }
        return false;
    }

private:
    inline static std::function<void()> _callback = nullptr;
};

} // namespace CustomFeatures

#endif // Q_OS_WIN
