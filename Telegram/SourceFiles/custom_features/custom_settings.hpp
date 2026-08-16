#pragma once

#include <QString>

namespace CustomFeatures {

struct ClientConfig {
    // 1. Навигация и чаты
    bool showFirstMessageButton = true;
    bool enableChatTypeTabs = true;
    bool collapsePinnedMessages = false;
    bool enhancedTextSelection = true;

    // 2. Ссылки и сеть
    bool cleanTrackingUrls = true;        // Очистка UTM/si меток
    bool directExternalLinks = true;       // Переход без подтверждения
    bool speedBoostEnabled = true;         // Многопоточное скачивание
    int downloadThreadCount = 8;           // Количество параллельных потоков

    // 3. Стикеры и сообщения
    bool unlimitedRecentStickers = true;   // Расширенный список стикеров
    int recentStickersMaxCount = 300;      // Лимит недавних стикеров (вместо 20-30)
    bool instantTranslatorEnabled = true;  // Встроенный переводчик
    QString translatorService = "google";  // "google", "deepl", "yandex"

    // 4. Интерфейс (UI)
    bool compactFolderSidebar = true;      // Компактный вертикальный сайдбар папок
    bool hideStoriesBar = false;           // Скрыть плашку историй
    bool hideSponsoredAds = true;          // Скрыть спонсорские посты
    bool hidePremiumPromos = true;         // Скрыть промо Premium
    QString customFontFamily = "";         // Пользовательский шрифт (если пусто - по умолчанию)
    int customFontSizeDelta = 0;           // Смещение размера шрифта (-2..+6)
};

// Глобальный синглтон настроек для быстрого доступа из любых компонентов UI и Core
inline ClientConfig& GetConfig() {
    static ClientConfig config;
    return config;
}

} // namespace CustomFeatures
