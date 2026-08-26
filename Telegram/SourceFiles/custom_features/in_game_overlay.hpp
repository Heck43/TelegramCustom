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
#include <algorithm>
#include "crl/crl_on_main.h"
#include "custom_features/custom_settings.hpp"
#include "core/application.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_row.h"
#include "dialogs/dialogs_key.h"
#include "apiwrap.h"
#include "api/api_send_progress.h"
#include "ui/painter.h"
#include "ui/empty_userpic.h"
#include "base/unixtime.h"

namespace CustomFeatures {

inline Main::Session *GetActiveSession() {
    if (Core::App().domain().started() && Core::App().domain().active().sessionExists()) {
        return &Core::App().domain().active().session();
    }
    return nullptr;
}

inline QPixmap GeneratePeerAvatarPixmap(PeerData *peer, int size) {
    if (!peer) return QPixmap();
    const int pxSize = size * 2;
    QPixmap pix(pxSize, pxSize);
    pix.fill(Qt::transparent);
    {
        Painter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        if (peer->isSelf()) {
            Ui::EmptyUserpic::PaintSavedMessages(p, 0, 0, pxSize, pxSize);
        } else if (peer->isRepliesChat()) {
            Ui::EmptyUserpic::PaintRepliesMessages(p, 0, 0, pxSize, pxSize);
        } else {
            auto view = peer->createUserpicView();
            peer->loadUserpic();
            peer->paintUserpicLeft(p, view, 0, 0, pxSize, pxSize);
        }
    }
    pix.setDevicePixelRatio(2.0);
    return pix;
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
        resize(960, 640);
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
                        if (++count >= 30) break;
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

        const QString peerName = history->peer ? history->peer->name() : "Диалог";
        _chatHeaderName->setText(peerName);
        _chatHeaderAvatar->setPixmap(GeneratePeerAvatarPixmap(history->peer, 36));

        const bool isChannel = history->peer && history->peer->isChannel() && !history->peer->isMegagroup();
        const bool isGroup = history->peer && (history->peer->isChat() || history->peer->isMegagroup());

        if (isChannel) {
            _chatHeaderStatus->setText("📢 Канал");
            _inputContainer->hide();
            _channelBanner->show();
        } else if (isGroup) {
            _chatHeaderStatus->setText("👥 Группа");
            _inputContainer->show();
            _channelBanner->hide();
        } else {
            _chatHeaderStatus->setText("👤 Личные сообщения");
            _inputContainer->show();
            _channelBanner->hide();
        }

        // Очищаем текущие сообщения
        QLayoutItem *child;
        while ((child = _messagesLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }

        // Собираем последние сообщения с конца (от новых к старым)
        std::vector<HistoryItem*> recentMessages;
        for (auto blockIt = history->blocks.rbegin(); blockIt != history->blocks.rend(); ++blockIt) {
            auto *block = blockIt->get();
            for (auto msgIt = block->messages.rbegin(); msgIt != block->messages.rend(); ++msgIt) {
                if (const auto item = (*msgIt)->data()) {
                    recentMessages.push_back(item);
                    if (recentMessages.size() >= 45) break;
                }
            }
            if (recentMessages.size() >= 45) break;
        }

        // Разворачиваем в хронологический порядок (сверху вниз)
        std::reverse(recentMessages.begin(), recentMessages.end());

        for (const auto item : recentMessages) {
            const bool out = item->out();
            QString text = item->originalText().text;
            if (text.isEmpty()) {
                text = item->notificationText().text;
            }
            if (text.isEmpty()) {
                text = item->isService() ? "Уведомление" : "[Медиасообщение]";
            }

            const auto date = base::unixtime::parse(item->date());
            const QString timeStr = date.toString("HH:mm");

            addMessageBubble(text, out, timeStr);
        }

        _messagesLayout->addStretch();
        QTimer::singleShot(30, [=] {
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

        const auto now = QDateTime::currentDateTime();
        addMessageBubble(text, true, now.toString("HH:mm"));
        _msgInput->clear();

        QTimer::singleShot(30, [=] {
            if (_msgScrollArea && _msgScrollArea->verticalScrollBar()) {
                _msgScrollArea->verticalScrollBar()->setValue(_msgScrollArea->verticalScrollBar()->maximum());
            }
        });
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Ультра-темный фон (#080B10, 99% непрозрачный)
        QColor bgColor(8, 11, 16, 252);
        p.setBrush(bgColor);
        p.setPen(QPen(QColor(255, 255, 255, 35), 1.5));
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
        const int unread = history->unreadCount();

        QString lastMsg = "...";
        if (const auto last = history->lastMessage()) {
            const QString t = last->originalText().text;
            lastMsg = !t.isEmpty() ? t : last->notificationText().text;
            if (lastMsg.isEmpty()) lastMsg = "[Медиа]";
        }

        const auto item = new QWidget(_chatListWidget);
        item->setCursor(Qt::PointingHandCursor);
        item->setStyleSheet("QWidget { background: rgba(255,255,255,0.03); border-radius: 12px; } QWidget:hover { background: rgba(255,255,255,0.09); }");
        
        auto *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(8, 8, 8, 8);
        itemLayout->setSpacing(10);

        // Реальная аватарка собеседника
        auto *avatar = new QLabel(item);
        avatar->setFixedSize(38, 38);
        avatar->setPixmap(GeneratePeerAvatarPixmap(history->peer, 38));
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
        auto *msgLabel = new QLabel(lastMsg.left(30) + (lastMsg.length() > 30 ? "..." : ""), item);
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

    void addMessageBubble(const QString &text, bool out, const QString &timeStr) {
        auto *bubble = new QWidget(_messagesWidget);
        auto *bubbleLayout = new QVBoxLayout(bubble);
        bubbleLayout->setContentsMargins(12, 8, 12, 8);
        bubbleLayout->setSpacing(3);

        auto *msgLabel = new QLabel(text, bubble);
        msgLabel->setWordWrap(true);
        msgLabel->setStyleSheet("color: #FFFFFF; font-size: 12px; line-height: 1.4;");
        bubbleLayout->addWidget(msgLabel);

        auto *timeLabel = new QLabel(timeStr + (out ? "  ✓✓" : ""), bubble);
        timeLabel->setAlignment(Qt::AlignRight);
        timeLabel->setStyleSheet(out ? "color: rgba(255,255,255,0.7); font-size: 10px;" : "color: #94A3B8; font-size: 10px;");
        bubbleLayout->addWidget(timeLabel);

        bubble->setMaximumWidth(520);

        if (out) {
            bubble->setStyleSheet("background: #0284C7; border-radius: 14px; margin: 2px 0px;");
            _messagesLayout->addWidget(bubble, 0, Qt::AlignRight);
        } else {
            bubble->setStyleSheet("background: rgba(30, 41, 59, 0.94); border-radius: 14px; margin: 2px 0px;");
            _messagesLayout->addWidget(bubble, 0, Qt::AlignLeft);
        }
    }

    void setupUI() {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(18, 14, 18, 14);
        rootLayout->setSpacing(12);

        // 1. Верхняя панель (Header)
        auto *headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(10);

        auto *appIconLabel = new QLabel("🎮", this);
        appIconLabel->setStyleSheet("font-size: 20px;");

        auto *titleLabel = new QLabel("Telegram Game Overlay", this);
        titleLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 16px; font-family: 'Segoe UI', sans-serif;");

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
        closeBtn->setFixedSize(28, 28);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet("QPushButton { color: #8E94A0; background: rgba(255,255,255,0.08); border: none; font-size: 14px; font-weight: bold; border-radius: 14px; } QPushButton:hover { color: #FFFFFF; background: rgba(239,68,68,0.7); }");
        connect(closeBtn, &QPushButton::clicked, this, &InGameOverlayWidget::hide);
        headerLayout->addWidget(closeBtn);

        rootLayout->addLayout(headerLayout);

        // 2. Основная область (Диалоги + Чат)
        auto *mainSplitter = new QHBoxLayout();
        mainSplitter->setSpacing(14);

        // --- Левая колонка: Реальный список диалогов (Ширина 300px) ---
        auto *leftSidebar = new QWidget(this);
        leftSidebar->setFixedWidth(300);
        leftSidebar->setStyleSheet("background: rgba(14, 18, 26, 0.9); border-radius: 14px;");
        auto *leftLayout = new QVBoxLayout(leftSidebar);
        leftLayout->setContentsMargins(10, 10, 10, 10);
        leftLayout->setSpacing(8);

        auto *searchBar = new QLineEdit(leftSidebar);
        searchBar->setPlaceholderText("🔍 Поиск диалогов...");
        searchBar->setStyleSheet("QLineEdit { background: rgba(255,255,255,0.07); color: #E2E8F0; border: none; border-radius: 10px; padding: 8px 12px; font-size: 12px; } QLineEdit:focus { background: rgba(255,255,255,0.12); }");
        leftLayout->addWidget(searchBar);

        auto *chatScrollArea = new QScrollArea(leftSidebar);
        chatScrollArea->setWidgetResizable(true);
        chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        chatScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical { background: transparent; width: 5px; margin: 0px; } QScrollBar::handle:vertical { background: rgba(255,255,255,0.18); border-radius: 2px; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
        
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
        chatArea->setStyleSheet("background: rgba(14, 18, 26, 0.9); border-radius: 14px;");
        auto *chatLayout = new QVBoxLayout(chatArea);
        chatLayout->setContentsMargins(16, 12, 16, 12);
        chatLayout->setSpacing(10);

        // Шапка чата
        auto *chatHeader = new QHBoxLayout();
        chatHeader->setSpacing(10);
        _chatHeaderAvatar = new QLabel(chatArea);
        _chatHeaderAvatar->setFixedSize(36, 36);
        
        auto *chatTitleLayout = new QVBoxLayout();
        chatTitleLayout->setSpacing(1);
        _chatHeaderName = new QLabel("Выберите диалог", chatArea);
        _chatHeaderName->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 14px;");
        _chatHeaderStatus = new QLabel("в сети", chatArea);
        _chatHeaderStatus->setStyleSheet("color: #38BDF8; font-size: 11px;");
        chatTitleLayout->addWidget(_chatHeaderName);
        chatTitleLayout->addWidget(_chatHeaderStatus);

        chatHeader->addWidget(_chatHeaderAvatar);
        chatHeader->addLayout(chatTitleLayout);
        chatHeader->addStretch();

        auto *callBtn = new QPushButton("📞", chatArea);
        callBtn->setFixedSize(30, 30);
        callBtn->setCursor(Qt::PointingHandCursor);
        callBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.07); color: #FFFFFF; border: none; border-radius: 15px; font-size: 13px; } QPushButton:hover { background: rgba(255,255,255,0.15); }");
        chatHeader->addWidget(callBtn);

        chatLayout->addLayout(chatHeader);

        // Область сообщений со скроллом
        _msgScrollArea = new QScrollArea(chatArea);
        _msgScrollArea->setWidgetResizable(true);
        _msgScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _msgScrollArea->setStyleSheet("QScrollArea { background: rgba(0,0,0,0.35); border: none; border-radius: 12px; } QScrollBar:vertical { background: transparent; width: 5px; margin: 0px; } QScrollBar::handle:vertical { background: rgba(255,255,255,0.18); border-radius: 2px; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
        
        _messagesWidget = new QWidget();
        _messagesWidget->setStyleSheet("background: transparent;");
        _messagesLayout = new QVBoxLayout(_messagesWidget);
        _messagesLayout->setContentsMargins(14, 14, 14, 14);
        _messagesLayout->setSpacing(8);
        _messagesLayout->addStretch();
        _msgScrollArea->setWidget(_messagesWidget);

        chatLayout->addWidget(_msgScrollArea);

        // Баннер для каналов
        _channelBanner = new QWidget(chatArea);
        auto *bannerLayout = new QHBoxLayout(_channelBanner);
        bannerLayout->setContentsMargins(12, 8, 12, 8);
        auto *bannerText = new QLabel("📢 Этот канал открыт в режиме только для чтения", _channelBanner);
        bannerText->setAlignment(Qt::AlignCenter);
        bannerText->setStyleSheet("color: #94A3B8; font-size: 12px; font-weight: 500;");
        bannerLayout->addWidget(bannerText);
        _channelBanner->setStyleSheet("background: rgba(255,255,255,0.05); border-radius: 12px;");
        _channelBanner->hide();
        chatLayout->addWidget(_channelBanner);

        // Поле ввода сообщения
        _inputContainer = new QWidget(chatArea);
        auto *inputLayout = new QHBoxLayout(_inputContainer);
        inputLayout->setContentsMargins(0, 0, 0, 0);
        inputLayout->setSpacing(8);

        _msgInput = new QLineEdit(_inputContainer);
        _msgInput->setPlaceholderText("Написать сообщение... (Enter для отправки)");
        _msgInput->setStyleSheet("QLineEdit { background: rgba(255,255,255,0.08); color: #FFFFFF; border: 1px solid rgba(255,255,255,0.12); border-radius: 18px; padding: 9px 16px; font-size: 13px; } QLineEdit:focus { border: 1px solid rgba(56, 189, 248, 0.6); }");
        connect(_msgInput, &QLineEdit::returnPressed, this, &InGameOverlayWidget::sendCurrentMessage);
        inputLayout->addWidget(_msgInput);

        auto *sendBtn = new QPushButton("➤", _inputContainer);
        sendBtn->setFixedSize(38, 38);
        sendBtn->setCursor(Qt::PointingHandCursor);
        sendBtn->setStyleSheet("QPushButton { background: #0284C7; color: #FFFFFF; border: none; border-radius: 19px; font-size: 15px; font-weight: bold; } QPushButton:hover { background: #0369A1; }");
        connect(sendBtn, &QPushButton::clicked, this, &InGameOverlayWidget::sendCurrentMessage);
        inputLayout->addWidget(sendBtn);

        chatLayout->addWidget(_inputContainer);

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
    QWidget *_inputContainer = nullptr;
    QWidget *_channelBanner = nullptr;
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
