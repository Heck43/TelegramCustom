#pragma once

#include <QString>

namespace CustomFeatures {

struct ClientConfig {
    // 1. Навигация и чаты
    bool showFirstMessageButton = true;
    bool enableChatTypeTabs = true;
    bool collapsePinnedMessages = false;
    bool enhancedTextSelection = true;
    bool enableMouseNavigation = true;     // Навигация по истории на Mouse 4 / 5

    // 2. Ссылки и сеть
    bool cleanTrackingUrls = true;        // Очистка UTM/si меток
    bool directExternalLinks = true;       // Переход без подтверждения
    bool speedBoostEnabled = true;         // Многопоточное скачивание
    int downloadThreadCount = 8;           // Количество параллельных потоков
    bool enableDownloadsRouter = true;     // Умная сортировка файлов по папкам

    // 3. Стикеры и медиа
    bool unlimitedRecentStickers = true;   // Расширенный список стикеров (1000 шт.)
    int recentStickersMaxCount = 1000;     // Лимит недавних стикеров
    bool instantTranslatorEnabled = true;  // Встроенный переводчик
    QString translatorService = "google";  // "google", "deepl", "yandex"

    // 4. Гейминг и интеграции
    bool enableInGameOverlay = true;       // Оверлей в играх (Shift + ~)
    bool enableGameStatus = true;          // Авто-статус игры в профиле
    bool autoLockOnWindowsLock = true;     // Авто-блокировка при Win + L

    // 5. Интерфейс (UI)
    bool enableMicaBackdrop = true;        // Эффект стекла Windows 11 (Mica / Acrylic)
    bool compactFolderSidebar = true;      // Компактный вертикальный сайдбар папок
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
