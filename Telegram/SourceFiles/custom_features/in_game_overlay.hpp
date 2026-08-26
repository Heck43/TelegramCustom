#pragma once

#ifdef Q_OS_WIN

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QVector>
#include <QtCore/QPair>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include "crl/crl_on_main.h"
#include "custom_features/custom_settings.hpp"
#include "core/application.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_row.h"
#include "dialogs/dialogs_key.h"
#include "apiwrap.h"
#include "api/api_send_progress.h"

namespace CustomFeatures {

inline Main::Session *GetActiveSession() {
    if (Core::App().domain().started() && Core::App().domain().active().sessionExists()) {
        return &Core::App().domain().active().session();
    }
    return nullptr;
}

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
        resize(840, 560);
        setupUI();
    }

    void toggleVisibility() {
        if (isVisible()) {
            hide();
        } else {
            reloadRealData();
            show();
            raise();
            activateWindow();
            SetWindowPos((HWND)winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }

    void reloadRealData() {
        const auto session = GetActiveSession();
        if (!session) return;

        // Очищаем текущий список чатов
        QLayoutItem *child;
        while ((child = _chatListLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }

        History *firstHistory = nullptr;
        if (const auto mainList = session->data().chatsList()) {
            if (const auto indexed = mainList->indexed()) {
                int count = 0;
                for (const auto &row : indexed->all()) {
                    if (const auto history = row->history()) {
                        if (!firstHistory) {
                            firstHistory = history;
                        }
                        addChatRowWidget(history);
                        if (++count >= 20) break;
                    }
                }
            }
        }

        if (!_activeHistory && firstHistory) {
            selectChat(firstHistory);
        } else if (_activeHistory) {
            selectChat(_activeHistory);
        }
    }

    void selectChat(History *history) {
        if (!history) return;
        _activeHistory = history;

        const QString peerName = history->peer ? history->peer->name() : "Чат";
        _chatHeaderName->setText(peerName);
        _chatHeaderAvatar->setText(peerName.left(1).toUpper());
        _chatHeaderStatus->setText("в сети");

        // Загружаем сообщения выбранного чата
        QLayoutItem *child;
        while ((child = _messagesLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }

        int msgCount = 0;
        for (const auto &block : history->blocks) {
            for (const auto &view : block->messages) {
                if (const auto item = view->data()) {
                    const bool out = item->out();
                    const QString text = item->originalText().text;
                    if (!text.isEmpty()) {
                        addMessageBubble(text, out);
                        if (++msgCount >= 30) break;
                    }
                }
            }
            if (msgCount >= 30) break;
        }

        _messagesLayout->addStretch();
        QTimer::singleShot(20, [=] {
            if (_msgScrollArea && _msgScrollArea->verticalScrollBar()) {
                _msgScrollArea->verticalScrollBar()->setValue(_msgScrollArea->verticalScrollBar()->maximum());
            }
        });
    }

    void sendCurrentMessage() {
        if (!_activeHistory || !_msgInput) return;
        const QString text = _msgInput->text().trimmed();
        if (text.isEmpty()) return;

        const auto session = GetActiveSession();
        if (!session) return;

        auto message = Api::MessageToSend(Api::SendAction(_activeHistory));
        message.textWithTags = { text };
        session->api().sendMessage(std::move(message));

        addMessageBubble(text, true);
        _msgInput->clear();

        QTimer::singleShot(20, [=] {
            if (_msgScrollArea && _msgScrollArea->verticalScrollBar()) {
                _msgScrollArea->verticalScrollBar()->setValue(_msgScrollArea->verticalScrollBar()->maximum());
            }
        });
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Насыщенный глубокий темный фон (#080B10, 99% непрозрачный)
        QColor bgColor(8, 11, 16, 252);
        p.setBrush(bgColor);
        p.setPen(QPen(QColor(255, 255, 255, 38), 1.4));
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
    void addChatRowWidget(History *history) {
        if (!history || !history->peer) return;

        const QString name = history->peer->name();
        const QString initial = name.left(1).toUpper();
        const int unread = history->unreadCount();

        QString lastMsg = "...";
        if (const auto last = history->lastMessage()) {
            const QString t = last->originalText().text;
            if (!t.isEmpty()) lastMsg = t;
        }

        auto *item = new QWidget(_chatListWidget);
        item->setCursor(Qt::PointingHandCursor);
        item->setStyleSheet("QWidget { background: rgba(255,255,255,0.03); border-radius: 10px; } QWidget:hover { background: rgba(255,255,255,0.09); }");
        
        auto *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(8, 8, 8, 8);
        itemLayout->setSpacing(10);

        static const QStringList kColors = { "#E11D48", "#6366F1", "#10B981", "#0284C7", "#D97706", "#8B5CF6", "#EC4899" };
        const uint hash = qHash(name);
        const QString avatarBg = kColors[hash % kColors.size()];

        auto *avatar = new QLabel(initial, item);
        avatar->setFixedSize(36, 36);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(QString("background: %1; color: #FFFFFF; font-weight: bold; border-radius: 18px; font-size: 14px;").arg(avatarBg));
        itemLayout->addWidget(avatar);

        auto *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(2);

        auto *topRow = new QHBoxLayout();
        auto *nameLabel = new QLabel(name, item);
        nameLabel->setStyleSheet("color: #F8FAFC; font-weight: 600; font-size: 13px;");
        topRow->addWidget(nameLabel);
        topRow->addStretch();
        infoLayout->addLayout(topRow);

        auto *bottomRow = new QHBoxLayout();
        auto *msgLabel = new QLabel(lastMsg.left(26) + (lastMsg.length() > 26 ? "..." : ""), item);
        msgLabel->setStyleSheet("color: #94A3B8; font-size: 11px;");
        bottomRow->addWidget(msgLabel);
        bottomRow->addStretch();

        if (unread > 0) {
            auto *badge = new QLabel(QString::number(unread), item);
            badge->setStyleSheet("background: #0284C7; color: #FFFFFF; font-size: 10px; font-weight: bold; border-radius: 8px; padding: 2px 6px;");
            bottomRow->addWidget(badge);
        }
        infoLayout->addLayout(bottomRow);

        itemLayout->addLayout(infoLayout);

        auto *clickBtn = new QPushButton(item);
        clickBtn->setStyleSheet("background: transparent; border: none;");
        clickBtn->setCursor(Qt::PointingHandCursor);
        connect(clickBtn, &QPushButton::clicked, this, [=] {
            selectChat(history);
        });

        auto *overlayWrap = new QWidget(_chatListWidget);
        auto *wrapLayout = new QVBoxLayout(overlayWrap);
        wrapLayout->setContentsMargins(0, 0, 0, 0);
        wrapLayout->addWidget(item);

        _chatListLayout->addWidget(overlayWrap);
    }

    void addMessageBubble(const QString &text, bool out) {
        auto *bubble = new QLabel(text + (out ? "  ✓✓" : ""), _messagesWidget);
        bubble->setWordWrap(true);
        bubble->setMaximumWidth(440);

        if (out) {
            bubble->setStyleSheet("background: #0284C7; color: #FFFFFF; border-radius: 12px; padding: 8px 14px; font-size: 12px; margin: 2px 0px;");
            _messagesLayout->addWidget(bubble, 0, Qt::AlignRight);
        } else {
            bubble->setStyleSheet("background: rgba(30, 41, 59, 0.92); color: #F1F5F9; border-radius: 12px; padding: 8px 14px; font-size: 12px; margin: 2px 0px;");
            _messagesLayout->addWidget(bubble, 0, Qt::AlignLeft);
        }
    }

    void setupUI() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(16, 14, 16, 14);
        rootLayout->setSpacing(10);

        // 1. Верхняя панель (Header / Title bar)
        auto *headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(10);

        auto *appIconLabel = new QLabel("🎮", this);
        appIconLabel->setStyleSheet("font-size: 18px;");

        auto *titleLabel = new QLabel("Telegram Game Overlay", this);
        titleLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 15px; font-family: 'Segoe UI', sans-serif;");

        auto *onlineDot = new QLabel("● В сети", this);
        onlineDot->setStyleSheet("color: #22C55E; font-size: 12px; font-weight: 600; margin-left: 4px;");

        headerLayout->addWidget(appIconLabel);
        headerLayout->addWidget(titleLabel);
        headerLayout->addWidget(onlineDot);
        headerLayout->addStretch();

        auto *hotkeyHint = new QLabel("Shift + ~", this);
        hotkeyHint->setStyleSheet("color: #717A8C; font-size: 12px; background: rgba(255,255,255,0.06); padding: 4px 10px; border-radius: 6px;");
        headerLayout->addWidget(hotkeyHint);

        auto *closeBtn = new QPushButton("✕", this);
        closeBtn->setFixedSize(26, 26);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet("QPushButton { color: #8E94A0; background: rgba(255,255,255,0.08); border: none; font-size: 14px; font-weight: bold; border-radius: 13px; } QPushButton:hover { color: #FFFFFF; background: rgba(239,68,68,0.7); }");
        connect(closeBtn, &QPushButton::clicked, this, &InGameOverlayWidget::hide);
        headerLayout->addWidget(closeBtn);

        rootLayout->addLayout(headerLayout);

        // 2. Основная рабочая область (Split: Список диалогов + Окно текущего чата)
        auto *mainSplitter = new QHBoxLayout();
        mainSplitter->setSpacing(12);

        // --- Левая колонка: Реальный список чатов ---
        auto *leftSidebar = new QWidget(this);
        leftSidebar->setFixedWidth(270);
        leftSidebar->setStyleSheet("background: rgba(14, 18, 26, 0.85); border-radius: 14px;");
        auto *leftLayout = new QVBoxLayout(leftSidebar);
        leftLayout->setContentsMargins(10, 10, 10, 10);
        leftLayout->setSpacing(8);

        auto *searchBar = new QLineEdit(leftSidebar);
        searchBar->setPlaceholderText("🔍 Поиск диалогов...");
        searchBar->setStyleSheet("QLineEdit { background: rgba(255,255,255,0.07); color: #E2E8F0; border: none; border-radius: 10px; padding: 7px 12px; font-size: 12px; } QLineEdit:focus { background: rgba(255,255,255,0.12); }");
        leftLayout->addWidget(searchBar);

        auto *chatScrollArea = new QScrollArea(leftSidebar);
        chatScrollArea->setWidgetResizable(true);
        chatScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical { background: transparent; width: 6px; } QScrollBar::handle:vertical { background: rgba(255,255,255,0.15); border-radius: 3px; }");
        
        _chatListWidget = new QWidget();
        _chatListWidget->setStyleSheet("background: transparent;");
        _chatListLayout = new QVBoxLayout(_chatListWidget);
        _chatListLayout->setContentsMargins(0, 0, 0, 0);
        _chatListLayout->setSpacing(6);
        _chatListLayout->addStretch();
        chatScrollArea->setWidget(_chatListWidget);

        leftLayout->addWidget(chatScrollArea);
        mainSplitter->addWidget(leftSidebar);

        // --- Правая колонка: Окно активного чата ---
        auto *chatArea = new QWidget(this);
        chatArea->setStyleSheet("background: rgba(14, 18, 26, 0.85); border-radius: 14px;");
        auto *chatLayout = new QVBoxLayout(chatArea);
        chatLayout->setContentsMargins(14, 12, 14, 12);
        chatLayout->setSpacing(10);

        // Шапка чата
        auto *chatHeader = new QHBoxLayout();
        _chatHeaderAvatar = new QLabel("T", chatArea);
        _chatHeaderAvatar->setFixedSize(32, 32);
        _chatHeaderAvatar->setAlignment(Qt::AlignCenter);
        _chatHeaderAvatar->setStyleSheet("background: #0284C7; color: #FFFFFF; font-weight: bold; border-radius: 16px; font-size: 13px;");
        
        auto *chatTitleLayout = new QVBoxLayout();
        chatTitleLayout->setSpacing(1);
        _chatHeaderName = new QLabel("Выберите диалог", chatArea);
        _chatHeaderName->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 13px;");
        _chatHeaderStatus = new QLabel("в сети", chatArea);
        _chatHeaderStatus->setStyleSheet("color: #38BDF8; font-size: 11px;");
        chatTitleLayout->addWidget(_chatHeaderName);
        chatTitleLayout->addWidget(_chatHeaderStatus);

        chatHeader->addWidget(_chatHeaderAvatar);
        chatHeader->addLayout(chatTitleLayout);
        chatHeader->addStretch();

        auto *callBtn = new QPushButton("📞", chatArea);
        callBtn->setFixedSize(28, 28);
        callBtn->setCursor(Qt::PointingHandCursor);
        callBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.07); color: #FFFFFF; border: none; border-radius: 14px; font-size: 12px; } QPushButton:hover { background: rgba(255,255,255,0.15); }");
        chatHeader->addWidget(callBtn);

        chatLayout->addLayout(chatHeader);

        // Область сообщений со скроллом
        _msgScrollArea = new QScrollArea(chatArea);
        _msgScrollArea->setWidgetResizable(true);
        _msgScrollArea->setStyleSheet("QScrollArea { background: rgba(0,0,0,0.3); border: none; border-radius: 10px; } QScrollBar:vertical { background: transparent; width: 6px; } QScrollBar::handle:vertical { background: rgba(255,255,255,0.15); border-radius: 3px; }");
        
        _messagesWidget = new QWidget();
        _messagesWidget->setStyleSheet("background: transparent;");
        _messagesLayout = new QVBoxLayout(_messagesWidget);
        _messagesLayout->setContentsMargins(12, 12, 12, 12);
        _messagesLayout->setSpacing(8);
        _messagesLayout->addStretch();
        _msgScrollArea->setWidget(_messagesWidget);

        chatLayout->addWidget(_msgScrollArea);

        // Поле ввода сообщения
        auto *inputLayout = new QHBoxLayout();
        inputLayout->setSpacing(8);

        _msgInput = new QLineEdit(chatArea);
        _msgInput->setPlaceholderText("Написать сообщение... (Enter для отправки)");
        _msgInput->setStyleSheet("QLineEdit { background: rgba(255,255,255,0.08); color: #FFFFFF; border: 1px solid rgba(255,255,255,0.12); border-radius: 16px; padding: 8px 14px; font-size: 12px; } QLineEdit:focus { border: 1px solid rgba(56, 189, 248, 0.6); }");
        connect(_msgInput, &QLineEdit::returnPressed, this, &InGameOverlayWidget::sendCurrentMessage);
        inputLayout->addWidget(_msgInput);

        auto *sendBtn = new QPushButton("➤", chatArea);
        sendBtn->setFixedSize(34, 34);
        sendBtn->setCursor(Qt::PointingHandCursor);
        sendBtn->setStyleSheet("QPushButton { background: #0284C7; color: #FFFFFF; border: none; border-radius: 17px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #0369A1; }");
        connect(sendBtn, &QPushButton::clicked, this, &InGameOverlayWidget::sendCurrentMessage);
        inputLayout->addWidget(sendBtn);

        chatLayout->addLayout(inputLayout);

        mainSplitter->addWidget(chatArea);
        rootLayout->addLayout(mainSplitter);

        // 3. Подвал с подсказками
        auto *footerLabel = new QLabel("⌨ Shift + ~ | Shift + F11 | Ctrl + Shift + O | Esc чтобы закрыть  •  Перетаскивайте окно мышью за шапку", this);
        footerLabel->setAlignment(Qt::AlignCenter);
        footerLabel->setStyleSheet("color: #64748B; font-size: 11px; padding-top: 2px;");
        rootLayout->addWidget(footerLabel);
    }

    QPoint _dragPos;
    History *_activeHistory = nullptr;

    QWidget *_chatListWidget = nullptr;
    QVBoxLayout *_chatListLayout = nullptr;

    QScrollArea *_msgScrollArea = nullptr;
    QWidget *_messagesWidget = nullptr;
    QVBoxLayout *_messagesLayout = nullptr;

    QLabel *_chatHeaderAvatar = nullptr;
    QLabel *_chatHeaderName = nullptr;
    QLabel *_chatHeaderStatus = nullptr;
    QLineEdit *_msgInput = nullptr;
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
        HWND foreground = GetForegroundWindow();
        if (!foreground) return false;

        // Не открываться поверх рабочего стола и панели задач Windows
        WCHAR className[256] = { 0 };
        GetClassNameW(foreground, className, 256);
        const QString cls = QString::fromWCharArray(className);
        if (cls == "Progman" || cls == "WorkerW" || cls == "Shell_TrayWnd" || cls == "Shell_SecondaryTrayWnd") {
            return false;
        }

        DWORD pid = 0;
        GetWindowThreadProcessId(foreground, &pid);
        if (!pid) return false;

        if (pid == GetCurrentProcessId()) {
            return true;
        }

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) return false;

        WCHAR procName[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProc, 0, procName, &size)) {
            QString fullPath = QString::fromWCharArray(procName);
            QString exeName = QFileInfo(fullPath).fileName().toLower();
            CloseHandle(hProc);

            // Исключаем проводник и системные оболочки
            if (exeName == "explorer.exe" || exeName == "dwm.exe" || exeName == "searchhost.exe" || exeName == "startmenuexperiencehost.exe") {
                return false;
            }

            if (GetConfig().overlayAllGames) {
                return true;
            }

            const auto allowed = GetConfig().overlayAllowedGames;
            if (allowed.isEmpty()) {
                return false;
            }

            for (const auto &item : allowed) {
                if (exeName.compare(item, Qt::CaseInsensitive) == 0 || exeName.contains(item, Qt::CaseInsensitive)) {
                    return true;
                }
            }
            return false;
        }
        CloseHandle(hProc);
        return false;
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
