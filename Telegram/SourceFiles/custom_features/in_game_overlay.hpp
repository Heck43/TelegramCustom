#pragma once

#ifdef Q_OS_WIN

#include <windows.h>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QMouseEvent>
#include "custom_features/custom_settings.hpp"

namespace CustomFeatures {

class InGameOverlayWidget : public QWidget {
public:
    explicit InGameOverlayWidget(QWidget *parent = nullptr)
        : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool) {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        resize(420, 560);
        setupUI();
    }

    void toggleVisibility() {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Полупрозрачный современный стеклянный фон (Glass / Acrylic style)
        QColor bgColor(24, 28, 36, 230);
        p.setBrush(bgColor);
        p.setPen(QPen(QColor(255, 255, 255, 30), 1.5));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            _dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            e->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (e->buttons() & Qt::LeftButton) {
            move(e->globalPosition().toPoint() - _dragPos);
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
        closeBtn->setStyleSheet("QPushButton { color: #8E94A0; background: transparent; border: none; font-size: 14px; font-weight: bold; border-radius: 12px; } QPushButton:hover { color: #FFFFFF; background: rgba(255,255,255,0.1); }");
        connect(closeBtn, &QPushButton::clicked, this, &InGameOverlayWidget::hide);

        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(closeBtn);
        mainLayout->addLayout(headerLayout);

        // Подсказка горячей клавиши
        auto *hintLabel = new QLabel("Быстрый вызов: Shift + ~ (Тильда)", this);
        hintLabel->setStyleSheet("color: #6C7883; font-size: 11px;");
        mainLayout->addWidget(hintLabel);

        // Список быстрых чатов (Placeholder с красивым списком)
        auto *contentBox = new QWidget(this);
        contentBox->setStyleSheet("background: rgba(0, 0, 0, 0.25); border-radius: 12px;");
        auto *contentLayout = new QVBoxLayout(contentBox);
        contentLayout->setContentsMargins(12, 12, 12, 12);

        auto *chatInfo = new QLabel("💬 Оверлей активен поверх ваших игр.\nПереписывайтесь с тиммейтами не сворачивая игру!", contentBox);
        chatInfo->setStyleSheet("color: #D1D5DB; font-size: 12px; line-height: 1.4;");
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
        if (!GetConfig().enableInGameOverlay) return;
        _hwnd = mainWindowHwnd;

        // Регистрация глобальной комбинации клавиш Shift + ~ (VK_OEM_3 = ~ / ё)
        RegisterHotKey(_hwnd, 0x5447, MOD_SHIFT, VK_OEM_3);

        if (!_overlayWidget) {
            _overlayWidget = new InGameOverlayWidget();
        }
    }

    void handleHotKey(WPARAM wParam) {
        if (wParam == 0x5447 && _overlayWidget) {
            _overlayWidget->toggleVisibility();
        }
    }

    void cleanup() {
        if (_hwnd) {
            UnregisterHotKey(_hwnd, 0x5447);
        }
    }

private:
    HWND _hwnd = nullptr;
    InGameOverlayWidget *_overlayWidget = nullptr;
};

} // namespace CustomFeatures

#endif // Q_OS_WIN
