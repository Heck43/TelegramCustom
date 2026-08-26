#pragma once

#ifdef Q_OS_WIN

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QPair>
#include <QFileInfo>
#include <QtCore/QTimer>
#include "crl/crl_on_main.h"
#include "custom_features/custom_settings.hpp"

namespace CustomFeatures {

struct RunningAppInfo {
    QString exeName;
    QString windowTitle;
};

inline QVector<RunningAppInfo> GetRunningUserApps() {
    QVector<RunningAppInfo> result;
    
    struct EnumData {
        QVector<RunningAppInfo> *list;
    } data = { &result };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
            return TRUE;
        }

        WCHAR title[256] = { 0 };
        int len = GetWindowTextW(hwnd, title, 256);
        if (len <= 0) {
            return TRUE;
        }

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid) {
            return TRUE;
        }

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) {
            return TRUE;
        }

        WCHAR path[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProc, 0, path, &size)) {
            QString exe = QFileInfo(QString::fromWCharArray(path)).fileName();
            QString winTitle = QString::fromWCharArray(title).trimmed();

            const QString lowerExe = exe.toLower();
            static const QStringList kIgnoreList = {
                "explorer.exe", "svchost.exe", "dwm.exe", "taskhostw.exe",
                "runtimebroker.exe", "searchhost.exe", "shellexperiencehost.exe",
                "telegram.exe", "textinputhost.exe", "applicationframehost.exe",
                "systemsettings.exe", "conhost.exe", "cmd.exe", "powershell.exe",
                "nvidia share.exe", "devenv.exe", "lockapp.exe"
            };

            if (!kIgnoreList.contains(lowerExe) && !winTitle.isEmpty()) {
                auto *list = reinterpret_cast<EnumData*>(lParam)->list;
                bool alreadyExists = false;
                for (const auto &item : *list) {
                    if (item.exeName.compare(exe, Qt::CaseInsensitive) == 0) {
                        alreadyExists = true;
                        break;
                    }
                }
                if (!alreadyExists) {
                    list->push_back({ exe, winTitle });
                }
            }
        }
        CloseHandle(hProc);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    return result;
}

class InGameOverlayWidget : public QWidget {
public:
    explicit InGameOverlayWidget(QWidget *parent = nullptr)
        : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool) {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        resize(460, 520);
        setupUI();
    }

    void toggleVisibility() {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
            SetWindowPos((HWND)winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Полупрозрачный темный фон
        QColor bgColor(18, 22, 30, 245);
        p.setBrush(bgColor);
        p.setPen(QPen(QColor(255, 255, 255, 45), 1.5));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            _dragPos = e->globalPos() - frameGeometry().topLeft();
            e->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (e->buttons() & Qt::LeftButton) {
            move(e->globalPos() - _dragPos);
            e->accept();
        }
    }

private:
    void setupUI() {
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(18, 18, 18, 18);
        mainLayout->setSpacing(14);

        // Шапка оверлея
        auto *headerLayout = new QHBoxLayout();
        auto *titleLabel = new QLabel("🎮 Telegram Game Overlay", this);
        titleLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 15px;");

        auto *closeBtn = new QPushButton("✕", this);
        closeBtn->setFixedSize(26, 26);
        closeBtn->setStyleSheet("QPushButton { color: #8E94A0; background: rgba(255,255,255,0.08); border: none; font-size: 14px; font-weight: bold; border-radius: 13px; } QPushButton:hover { color: #FFFFFF; background: rgba(255,255,255,0.2); }");
        connect(closeBtn, &QPushButton::clicked, this, &InGameOverlayWidget::hide);

        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(closeBtn);
        mainLayout->addLayout(headerLayout);

        // Подсказка горячих клавиш
        auto *hintLabel = new QLabel("Горячие клавиши: Shift + ~ | Shift + F11 | Ctrl + Shift + O", this);
        hintLabel->setStyleSheet("color: #8E9BAE; font-size: 11px;");
        mainLayout->addWidget(hintLabel);

        // Панель статуса
        auto *contentBox = new QWidget(this);
        contentBox->setStyleSheet("background: rgba(0, 0, 0, 0.4); border-radius: 12px;");
        auto *contentLayout = new QVBoxLayout(contentBox);
        contentLayout->setContentsMargins(16, 16, 16, 16);
        contentLayout->setSpacing(10);

        auto *chatInfo = new QLabel("💬 Оверлей успешно активен поверх вашей игры.\n\nВы можете свободно перемещать это окно мышкой по экрану во время игры.", contentBox);
        chatInfo->setStyleSheet("color: #E2E8F0; font-size: 12px; line-height: 1.5;");
        chatInfo->setWordWrap(true);
        contentLayout->addWidget(chatInfo);
        contentLayout->addStretch();

        mainLayout->addWidget(contentBox);
    }

    QPoint _dragPos;
};

class InGameOverlayManager {
public:
    static InGameOverlayManager& Instance() {
        static InGameOverlayManager manager;
        return manager;
    }

    void init(HWND mainWindowHwnd) {
        _hwnd = mainWindowHwnd;
        updateState();
    }

    void updateState() {
        if (GetConfig().enableInGameOverlay) {
            installHook();
            if (!_overlayWidget) {
                _overlayWidget = new InGameOverlayWidget();
            }
        } else {
            removeHook();
            if (_overlayWidget) {
                _overlayWidget->hide();
            }
        }
    }

    void triggerOverlay() {
        if (!GetConfig().enableInGameOverlay) return;
        if (!_overlayWidget) {
            _overlayWidget = new InGameOverlayWidget();
        }
        if (_overlayWidget->isVisible()) {
            _overlayWidget->hide();
        } else if (isTargetGameActive()) {
            _overlayWidget->toggleVisibility();
        }
    }

    void handleHotKey(WPARAM wParam) {
        if (wParam >= 0x5447 && wParam <= 0x544A && GetConfig().enableInGameOverlay) {
            triggerOverlay();
        }
    }

    bool isTargetGameActive() const {
        if (GetConfig().overlayAllGames) {
            return true;
        }

        const auto allowed = GetConfig().overlayAllowedGames;
        if (allowed.isEmpty()) {
            return true;
        }

        HWND foreground = GetForegroundWindow();
        if (!foreground) return true;

        DWORD pid = 0;
        GetWindowThreadProcessId(foreground, &pid);
        if (!pid) return true;

        if (pid == GetCurrentProcessId()) {
            return true;
        }

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) return true;

        WCHAR procName[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProc, 0, procName, &size)) {
            QString fullPath = QString::fromWCharArray(procName);
            QString exeName = QFileInfo(fullPath).fileName();
            CloseHandle(hProc);

            for (const auto &item : allowed) {
                if (exeName.compare(item, Qt::CaseInsensitive) == 0 || exeName.contains(item, Qt::CaseInsensitive)) {
                    return true;
                }
            }
            return false;
        }
        CloseHandle(hProc);
        return true;
    }

    void cleanup() {
        removeHook();
        if (_hwnd) {
            UnregisterHotKey(_hwnd, 0x5447);
            UnregisterHotKey(_hwnd, 0x5448);
            UnregisterHotKey(_hwnd, 0x5449);
            UnregisterHotKey(_hwnd, 0x544A);
            _hwnd = nullptr;
        }
        if (_overlayWidget) {
            delete _overlayWidget;
            _overlayWidget = nullptr;
        }
    }

private:
    void installHook() {
        if (!_hook) {
            _hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
        }
        if (_hwnd) {
            RegisterHotKey(_hwnd, 0x5447, MOD_SHIFT, VK_OEM_3);
            RegisterHotKey(_hwnd, 0x5448, MOD_SHIFT, VK_F11);
            RegisterHotKey(_hwnd, 0x5449, MOD_CONTROL | MOD_SHIFT, 'O');
            RegisterHotKey(_hwnd, 0x544A, MOD_ALT, VK_OEM_3);
        }
    }

    void removeHook() {
        if (_hook) {
            UnhookWindowsHookEx(_hook);
            _hook = nullptr;
        }
    }

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            auto *p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            const bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            bool triggered = false;
            // Shift + ~ (VK_OEM_3 = 0xC0)
            if (shift && (p->vkCode == VK_OEM_3 || p->vkCode == 0xC0)) {
                triggered = true;
            }
            // Shift + F11
            else if (shift && p->vkCode == VK_F11) {
                triggered = true;
            }
            // Ctrl + Shift + O
            else if (ctrl && shift && p->vkCode == 'O') {
                triggered = true;
            }
            // Alt + ~
            else if (alt && (p->vkCode == VK_OEM_3 || p->vkCode == 0xC0)) {
                triggered = true;
            }

            if (triggered && GetConfig().enableInGameOverlay) {
                crl::on_main([] {
                    Instance().triggerOverlay();
                });
            }
        }
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    HWND _hwnd = nullptr;
    HHOOK _hook = nullptr;
    InGameOverlayWidget *_overlayWidget = nullptr;
};

} // namespace CustomFeatures

#endif // Q_OS_WIN
