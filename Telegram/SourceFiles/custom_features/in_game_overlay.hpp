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

        // Проверяем наличие заголовка
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

            // Отсеиваем системный мусор
            const QString lowerExe = exe.toLower();
            static const QStringList kIgnoreList = {
                "explorer.exe", "svchost.exe", "dwm.exe", "taskhostw.exe",
                "runtimebroker.exe", "searchhost.exe", "shellexperiencehost.exe",
                "telegram.exe", "textinputhost.exe", "applicationframehost.exe",
                "systemsettings.exe", "conhost.exe", "cmd.exe", "powershell.exe",
                "nvidia share.exe", "devenv.exe"
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
        resize(440, 580);
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

        // Полупрозрачный темный фон для любых игр (OpenGL / DirectX / Vulkan)
        QColor bgColor(18, 22, 30, 238);
        p.setBrush(bgColor);
        p.setPen(QPen(QColor(255, 255, 255, 35), 1.5));
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
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(12);

        // Шапка оверлея
        auto *headerLayout = new QHBoxLayout();
        auto *titleLabel = new QLabel("🎮 Telegram Game Overlay", this);
        titleLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 14px;");

        auto *closeBtn = new QPushButton("✕", this);
        closeBtn->setFixedSize(24, 24);
        closeBtn->setStyleSheet("QPushButton { color: #8E94A0; background: transparent; border: none; font-size: 14px; font-weight: bold; border-radius: 12px; } QPushButton:hover { color: #FFFFFF; background: rgba(255,255,255,0.15); }");
        connect(closeBtn, &QPushButton::clicked, this, &InGameOverlayWidget::hide);

        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(closeBtn);
        mainLayout->addLayout(headerLayout);

        // Подсказка горячей клавиши
        auto *hintLabel = new QLabel("Вызов: Shift + ~ (Тильда) • Поддержка OpenGL / DirectX / Vulkan", this);
        hintLabel->setStyleSheet("color: #7A8B9E; font-size: 11px;");
        mainLayout->addWidget(hintLabel);

        // Панель статуса
        auto *contentBox = new QWidget(this);
        contentBox->setStyleSheet("background: rgba(0, 0, 0, 0.35); border-radius: 12px;");
        auto *contentLayout = new QVBoxLayout(contentBox);
        contentLayout->setContentsMargins(14, 14, 14, 14);

        auto *chatInfo = new QLabel("💬 Оверлей активен поверх ваших игр.\n\nВы можете перетаскивать это окно за шапку и общаться не сворачивая игру.", contentBox);
        chatInfo->setStyleSheet("color: #D1D5DB; font-size: 12px; line-height: 1.5;");
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
        if (!_hwnd) return;
        if (GetConfig().enableInGameOverlay) {
            RegisterHotKey(_hwnd, 0x5447, MOD_SHIFT, VK_OEM_3);
            if (!_overlayWidget) {
                _overlayWidget = new InGameOverlayWidget();
            }
        } else {
            UnregisterHotKey(_hwnd, 0x5447);
            if (_overlayWidget) {
                _overlayWidget->hide();
            }
        }
    }

    void handleHotKey(WPARAM wParam) {
        if (wParam == 0x5447 && GetConfig().enableInGameOverlay) {
            if (isTargetGameActive()) {
                if (!_overlayWidget) {
                    _overlayWidget = new InGameOverlayWidget();
                }
                _overlayWidget->toggleVisibility();
            }
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

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) return true;

        WCHAR procName[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProc, 0, procName, &size)) {
            QString fullPath = QString::fromWCharArray(procName);
            QString exeName = QFileInfo(fullPath).fileName();
            CloseHandle(hProc);

            for (const auto &item : allowed) {
                if (exeName.compare(item, Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
            return false;
        }
        CloseHandle(hProc);
        return true;
    }

    void cleanup() {
        if (_hwnd) {
            UnregisterHotKey(_hwnd, 0x5447);
            _hwnd = nullptr;
        }
        if (_overlayWidget) {
            delete _overlayWidget;
            _overlayWidget = nullptr;
        }
    }

private:
    HWND _hwnd = nullptr;
    InGameOverlayWidget *_overlayWidget = nullptr;
};

} // namespace CustomFeatures

#endif // Q_OS_WIN
