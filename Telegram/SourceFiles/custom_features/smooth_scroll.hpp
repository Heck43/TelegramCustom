#pragma once

#include "custom_features/custom_settings.hpp"
#include "ui/ui_utility.h"
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QWheelEvent>
#include <QEasingCurve>

namespace CustomFeatures {
namespace SmoothScroll {

inline bool IsEnabled() {
    return Ui::IsSmoothScrollingEnabled();
}

// Применить плавную прокрутку к QScrollBar
inline void ApplySmoothScrolling(QScrollBar* scrollBar, int delta) {
    if (!scrollBar || !IsEnabled()) {
        // Если плавная прокрутка выключена, используем стандартное поведение
        if (scrollBar) {
            scrollBar->setValue(scrollBar->value() - delta);
        }
        return;
    }
    
    // Создаём анимацию для плавной прокрутки
    const int currentValue = scrollBar->value();
    const int targetValue = currentValue - delta;
    
    // Используем QPropertyAnimation для плавного перехода
    auto* animation = new QPropertyAnimation(scrollBar, "value", scrollBar);
    animation->setDuration(250); // 250ms - оптимальная длительность для scroll
    animation->setStartValue(currentValue);
    animation->setEndValue(targetValue);
    animation->setEasingCurve(QEasingCurve::OutCubic); // Плавное замедление
    
    // Удаляем анимацию после завершения
    QObject::connect(animation, &QPropertyAnimation::finished, animation, &QObject::deleteLater);
    
    animation->start();
}

// Обработка wheel event с плавной прокруткой
inline bool HandleWheelEvent(QScrollBar* scrollBar, QWheelEvent* event) {
    if (!scrollBar || !CustomFeatures::GetConfig().smoothScrolling) {
        return false; // Пусть обрабатывается стандартно
    }
    
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        return false;
    }
    
    // Применяем плавную прокрутку
    ApplySmoothScrolling(scrollBar, delta);
    
    event->accept();
    return true; // Событие обработано
}

} // namespace SmoothScroll
} // namespace CustomFeatures
