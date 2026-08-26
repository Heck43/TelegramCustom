#pragma once

#ifdef Q_OS_WIN

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
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
        resize(680, 480);
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

        // Более глубокий затемненный полупрозрачный фон для удобства в играх
        QColor bgColor(9, 12, 17, 246);
        p.setBrush(bgColor);
        p.setPen(QPen(QColor(255, 255, 255, 30), 1.2));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 18, 18);
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

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            hide();
            e->accept();
        } else {
            QWidget::keyPressEvent(e);
        }
    }

private:
    void setupUI() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(14, 12, 14, 12);
        rootLayout->setSpacing(10);

        // 1. Верхняя панель (Header / Title bar)
        auto *headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(8);

        auto *appIconLabel = new QLabel("🎮", this);
        appIconLabel->setStyleSheet("font-size: 16px;");

        auto *titleLabel = new QLabel("Telegram Overlay", this);
        titleLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 14px; font-family: 'Segoe UI', sans-serif;");

        auto *onlineDot = new QLabel("● В сети", this);
        onlineDot->setStyleSheet("color: #22C55E; font-size: 11px; font-weight: 600; margin-left: 4px;");

        headerLayout->addWidget(appIconLabel);
        headerLayout->addWidget(titleLabel);
        headerLayout->addWidget(onlineDot);
        headerLayout->addStretch();

        auto *hotkeyHint = new QLabel("Shift + ~", this);
        hotkeyHint->setStyleSheet("color: #717A8C; font-size: 11px; background: rgba(255,255,255,0.06); padding: 3px 8px; border-radius: 6px;");
        headerLayout->addWidget(hotkeyHint);

        auto *closeBtn = new QPushButton("✕", this);
        closeBtn->setFixedSize(24, 24);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet("QPushButton { color: #8E94A0; background: rgba(255,255,255,0.08); border: none; font-size: 13px; font-weight: bold; border-radius: 12px; } QPushButton:hover { color: #FFFFFF; background: rgba(239,68,68,0.7); }");
        connect(closeBtn, &QPushButton::clicked, this, &InGameOverlayWidget::hide);
        headerLayout->addWidget(closeBtn);

        rootLayout->addLayout(headerLayout);

        // 2. Основная рабочая область (Split: Список диалогов + Окно текущего чата)
        auto *mainSplitter = new QHBoxLayout();
        mainSplitter->setSpacing(10);

        // --- Левая колонка: Список чатов (Ширина ~210px) ---
        auto *leftSidebar = new QWidget(this);
        leftSidebar->setFixedWidth(210);
        leftSidebar->setStyleSheet("background: rgba(16, 21, 30, 0.7); border-radius: 12px;");
        auto *leftLayout = new QVBoxLayout(leftSidebar);
        leftLayout->setContentsMargins(8, 10, 8, 10);
        leftLayout->setSpacing(6);

        auto *searchBar = new QLineEdit(leftSidebar);
        searchBar->setPlaceholderText("🔍 Поиск диалогов...");
        searchBar->setStyleSheet("QLineEdit { background: rgba(255,255,255,0.06); color: #E2E8F0; border: none; border-radius: 8px; padding: 6px 10px; font-size: 11px; } QLineEdit:focus { background: rgba(255,255,255,0.1); }");
        leftLayout->addWidget(searchBar);

        // Список чатов (макет элементов)
        auto createChatItem = [leftSidebar](const QString &avatarText, const QString &avatarColor, const QString &name, const QString &preview, const QString &time, const QString &badge) {
            auto *item = new QWidget(leftSidebar);
            item->setCursor(Qt::PointingHandCursor);
            item->setStyleSheet(badge.isEmpty() ? "QWidget:hover { background: rgba(255,255,255,0.05); border-radius: 8px; }" : "QWidget { background: rgba(255,255,255,0.08); border-radius: 8px; }");
            auto *itemLayout = new QHBoxLayout(item);
            itemLayout->setContentsMargins(6, 6, 6, 6);
            itemLayout->setSpacing(8);

            auto *avatar = new QLabel(avatarText, item);
            avatar->setFixedSize(32, 32);
            avatar->setAlignment(Qt::AlignCenter);
            avatar->setStyleSheet(QString("background: %1; color: #FFFFFF; font-weight: bold; border-radius: 16px; font-size: 13px;").arg(avatarColor));
            itemLayout->addWidget(avatar);

            auto *infoLayout = new QVBoxLayout();
            infoLayout->setSpacing(2);
            auto *topRow = new QHBoxLayout();
            auto *nameLabel = new QLabel(name, item);
            nameLabel->setStyleSheet("color: #F8FAFC; font-weight: 600; font-size: 12px;");
            auto *timeLabel = new QLabel(time, item);
            timeLabel->setStyleSheet("color: #64748B; font-size: 10px;");
            topRow->addWidget(nameLabel);
            topRow->addStretch();
            topRow->addWidget(timeLabel);

            auto *bottomRow = new QHBoxLayout();
            auto *msgLabel = new QLabel(preview, item);
            msgLabel->setStyleSheet("color: #94A3B8; font-size: 11px;");
            bottomRow->addWidget(msgLabel);
            bottomRow->addStretch();
            if (!badge.isEmpty()) {
                auto *badgeLabel = new QLabel(badge, item);
                badgeLabel->setStyleSheet("background: #3B82F6; color: #FFFFFF; font-size: 9px; font-weight: bold; border-radius: 7px; padding: 1px 5px;");
                bottomRow->addWidget(badgeLabel);
            }

            infoLayout->addLayout(topRow);
            infoLayout->addLayout(bottomRow);
            itemLayout->addLayout(infoLayout);

            return item;
        };

        leftLayout->addWidget(createChatItem("A", "#E11D48", "Astartes", "Да все в порядке солнце <3", "16:27", "2"));
        leftLayout->addWidget(createChatItem("D", "#6366F1", "Discord & Game", "Го в катку вечером!", "15:40", ""));
        leftLayout->addWidget(createChatItem("M", "#10B981", "Музыка & Chill", "🎵 Отличный плейлист", "Вчера", ""));
        leftLayout->addWidget(createChatItem("T", "#0284C7", "Telegram Desktop", "Обновление готово ✓", "24 авг", ""));
        leftLayout->addStretch();

        mainSplitter->addWidget(leftSidebar);

        // --- Правая колонка: Окно активного чата ---
        auto *chatArea = new QWidget(this);
        chatArea->setStyleSheet("background: rgba(16, 21, 30, 0.7); border-radius: 12px;");
        auto *chatLayout = new QVBoxLayout(chatArea);
        chatLayout->setContentsMargins(12, 10, 12, 10);
        chatLayout->setSpacing(8);

        // Шапка чата
        auto *chatHeader = new QHBoxLayout();
        auto *chatAvatar = new QLabel("A", chatArea);
        chatAvatar->setFixedSize(28, 28);
        chatAvatar->setAlignment(Qt::AlignCenter);
        chatAvatar->setStyleSheet("background: #E11D48; color: #FFFFFF; font-weight: bold; border-radius: 14px; font-size: 12px;");
        
        auto *chatTitleLayout = new QVBoxLayout();
        chatTitleLayout->setSpacing(1);
        auto *chatName = new QLabel("Astartes", chatArea);
        chatName->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 12px;");
        auto *chatStatus = new QLabel("в сети (печатает...)", chatArea);
        chatStatus->setStyleSheet("color: #38BDF8; font-size: 10px;");
        chatTitleLayout->addWidget(chatName);
        chatTitleLayout->addWidget(chatStatus);

        chatHeader->addWidget(chatAvatar);
        chatHeader->addLayout(chatTitleLayout);
        chatHeader->addStretch();

        auto *callBtn = new QPushButton("📞", chatArea);
        callBtn->setFixedSize(26, 26);
        callBtn->setCursor(Qt::PointingHandCursor);
        callBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.07); color: #FFFFFF; border: none; border-radius: 13px; font-size: 11px; } QPushButton:hover { background: rgba(255,255,255,0.15); }");
        chatHeader->addWidget(callBtn);

        chatLayout->addLayout(chatHeader);

        // Область сообщений (макет пузырей)
        auto *msgContainer = new QWidget(chatArea);
        msgContainer->setStyleSheet("background: rgba(0, 0, 0, 0.25); border-radius: 8px;");
        auto *msgListLayout = new QVBoxLayout(msgContainer);
        msgListLayout->setContentsMargins(10, 10, 10, 10);
        msgListLayout->setSpacing(8);

        auto *msgIn1 = new QLabel("Ты то как мой дорогой в целом? Что делаешь? 💕", msgContainer);
        msgIn1->setStyleSheet("background: rgba(30, 41, 59, 0.85); color: #F1F5F9; border-radius: 10px; padding: 7px 12px; font-size: 11px;");
        msgListLayout->addWidget(msgIn1, 0, Qt::AlignLeft);

        auto *msgOut1 = new QLabel("Да все в порядке солнце <3\nПока что вот тоже отдыхаю, играю :3  ✓✓", msgContainer);
        msgOut1->setStyleSheet("background: rgba(2, 132, 199, 0.7); color: #FFFFFF; border-radius: 10px; padding: 7px 12px; font-size: 11px;");
        msgListLayout->addWidget(msgOut1, 0, Qt::AlignRight);

        auto *msgIn2 = new QLabel("О, ты в оверлее Telegram? Выглядит очень круто! ✨", msgContainer);
        msgIn2->setStyleSheet("background: rgba(30, 41, 59, 0.85); color: #F1F5F9; border-radius: 10px; padding: 7px 12px; font-size: 11px;");
        msgListLayout->addWidget(msgIn2, 0, Qt::AlignLeft);

        msgListLayout->addStretch();
        chatLayout->addWidget(msgContainer);

        // Поле ввода сообщения
        auto *inputLayout = new QHBoxLayout();
        inputLayout->setSpacing(6);

        auto *attachBtn = new QPushButton("📎", chatArea);
        attachBtn->setFixedSize(28, 28);
        attachBtn->setCursor(Qt::PointingHandCursor);
        attachBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.06); color: #94A3B8; border: none; border-radius: 14px; font-size: 12px; } QPushButton:hover { color: #FFFFFF; background: rgba(255,255,255,0.12); }");
        inputLayout->addWidget(attachBtn);

        auto *msgInput = new QLineEdit(chatArea);
        msgInput->setPlaceholderText("Написать сообщение... (Enter для отправки)");
        msgInput->setStyleSheet("QLineEdit { background: rgba(255,255,255,0.07); color: #FFFFFF; border: 1px solid rgba(255,255,255,0.1); border-radius: 14px; padding: 5px 12px; font-size: 11px; } QLineEdit:focus { border: 1px solid rgba(56, 189, 248, 0.6); }");
        inputLayout->addWidget(msgInput);

        auto *emojiBtn = new QPushButton("😊", chatArea);
        emojiBtn->setFixedSize(28, 28);
        emojiBtn->setCursor(Qt::PointingHandCursor);
        emojiBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.06); color: #94A3B8; border: none; border-radius: 14px; font-size: 12px; } QPushButton:hover { color: #FFFFFF; background: rgba(255,255,255,0.12); }");
        inputLayout->addWidget(emojiBtn);

        auto *sendBtn = new QPushButton("➤", chatArea);
        sendBtn->setFixedSize(28, 28);
        sendBtn->setCursor(Qt::PointingHandCursor);
        sendBtn->setStyleSheet("QPushButton { background: #0284C7; color: #FFFFFF; border: none; border-radius: 14px; font-size: 12px; font-weight: bold; } QPushButton:hover { background: #0369A1; }");
        inputLayout->addWidget(sendBtn);

        chatLayout->addLayout(inputLayout);

        mainSplitter->addWidget(chatArea);

        rootLayout->addLayout(mainSplitter);

        // 3. Подвал с подсказками управления
        auto *footerLabel = new QLabel("⌨ Shift + ~ | Shift + F11 | Ctrl + Shift + O | Esc чтобы закрыть  •  Перетаскивайте окно мышью", this);
        footerLabel->setAlignment(Qt::AlignCenter);
        footerLabel->setStyleSheet("color: #64748B; font-size: 10px; padding-top: 2px;");
        rootLayout->addWidget(footerLabel);
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
