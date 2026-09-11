#pragma once

#include "custom_features/custom_settings.hpp"
#include <QEasingCurve>

namespace CustomFeatures {
namespace UI {

// Получить скругление для разных типов элементов
inline int GetChatBorderRadius() {
    return GetConfig().chatBorderRadius;
}

inline int GetMessageBorderRadius() {
    return GetConfig().messageBorderRadius;
}

inline int GetButtonBorderRadius() {
    return GetConfig().buttonBorderRadius;
}

// Тени
inline bool AreSoftShadowsEnabled() {
    return GetConfig().enableSoftShadows;
}

// Анимации
inline int GetAnimationDuration(int baseMs) {
    const int speed = GetConfig().animationSpeed;
    if (speed == 0) return 0; // Мгновенно
    
    // Масштабирование: 0=instant, 5=normal(100%), 10=slow(200%)
    const float multiplier = speed / 5.0f;
    return static_cast<int>(baseMs * multiplier);
}

inline QEasingCurve GetSmoothEasing() {
    // Используем кубическую кривую для плавности (UI/UX best practice)
    return QEasingCurve::OutCubic;
}

// Spacing/Density
inline int GetUISpacing(int baseSpacing) {
    const int density = GetConfig().uiDensity;
    // 0=compact(60%), 5=normal(100%), 10=spacious(140%)
    const float multiplier = 0.6f + (density * 0.08f);
    return static_cast<int>(baseSpacing * multiplier);
}

// Размер шрифта
inline int GetFontSize() {
    return GetConfig().fontSize;
}

// Плавная прокрутка
inline bool IsSmoothScrollingEnabled() {
    return GetConfig().smoothScrolling;
}

// Применить стиль тени (для использования в QSS или QPainter)
inline QString GetSoftShadowStyle() {
    if (!AreSoftShadowsEnabled()) {
        return QString();
    }
    // Мягкая тень: offset 0 2px, blur 8px, rgba(0,0,0,0.1)
    // Для Qt: используется через QGraphicsDropShadowEffect
    return "rgba(0, 0, 0, 0.1)";
}

// CSS border-radius для QSS
inline QString GetBorderRadiusStyle(int radius) {
    return QString("border-radius: %1px;").arg(radius);
}

} // namespace UI
} // namespace CustomFeatures
