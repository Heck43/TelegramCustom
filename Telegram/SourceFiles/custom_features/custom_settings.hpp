#pragma once

#include <QString>

namespace CustomFeatures {

struct ClientConfig {
    // 1. Ссылки и приватность
    bool cleanTrackingUrls = true;        // Очистка UTM/si меток
    bool directExternalLinks = true;       // Переход без подтверждения
    bool enableDownloadsRouter = true;     // Умная сортировка файлов по папкам

    // 2. Стикеры и медиа
    bool unlimitedRecentStickers = true;   // Расширенный список стикеров (1000 шт.)
    int recentStickersMaxCount = 1000;     // Лимит недавних стикеров

    // 3. Гейминг и система
    bool enableInGameOverlay = true;       // Оверлей в играх (Shift + ~)
    bool enableGameStatus = true;          // Авто-статус игры в профиле
    bool autoLockOnWindowsLock = true;     // Авто-блокировка при Win + L

    // 4. Интерфейс (UI)
    bool enableMicaBackdrop = true;        // Эффект стекла Windows 11 (Mica / Acrylic)
    bool hideStoriesBar = false;           // Скрыть плашку историй
    bool hideSponsoredAds = true;          // Скрыть спонсорские посты
    bool hidePremiumPromos = true;         // Скрыть промо Premium
};

// Глобальный синглтон настроек для быстрого доступа из любых компонентов UI и Core
inline ClientConfig& GetConfig() {
    static ClientConfig config;
    return config;
}

} // namespace CustomFeatures
