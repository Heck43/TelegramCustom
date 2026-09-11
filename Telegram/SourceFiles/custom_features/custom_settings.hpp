#pragma once

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>

namespace CustomFeatures {

struct ClientConfig {
    // 1. Ссылки и приватность
    bool cleanTrackingUrls = true;        // Очистка UTM/si меток
    bool directExternalLinks = true;       // Переход без подтверждения
    bool enableDownloadsRouter = true;     // Умная сортировка файлов по папкам

    // 2. Стикеры и медиа
    bool unlimitedRecentStickers = true;   // Расширенный список стикеров (300 шт.)
    int recentStickersMaxCount = 300;      // Лимит недавних стикеров (300)

    // 3. Гейминг и система
    bool enableInGameOverlay = true;       // Оверлей в играх (Shift + ~)
    bool overlayAllGames = false;          // false = только в выбранных играх, true = во всех
    QStringList overlayAllowedGames = { "javaw.exe", "cs2.exe", "dota2.exe", "RobloxPlayerBeta.exe", "GTA5.exe" };
    bool enableGameStatus = false;         // Авто-статус игры в профиле
    bool autoLockOnWindowsLock = false;    // Авто-блокировка при Win + L

    // 4. Интерфейс (UI)
    bool syncWindowsAccentColor = true;    // Цвет акцента из Windows (под обои / Wallpaper Engine)
    bool hideStoriesBar = false;           // Скрыть плашку историй
    bool hideSponsoredAds = true;          // Скрыть спонсорские посты и рекламные плашки вверху
    bool hidePremiumPromos = true;         // Скрыть промо Premium

    // 5. Производительность
    bool fastStartup = true;               // Быстрый запуск (отложенная загрузка)
    bool delayStickersLoad = true;         // Отложить загрузку стикеров
    bool delayStoriesLoad = true;          // Отложить загрузку Stories

    void load() {
        const auto path = getSettingsFilePath();
        QSettings s(path, QSettings::IniFormat);

        cleanTrackingUrls = s.value("cleanTrackingUrls", cleanTrackingUrls).toBool();
        directExternalLinks = s.value("directExternalLinks", directExternalLinks).toBool();
        enableDownloadsRouter = s.value("enableDownloadsRouter", enableDownloadsRouter).toBool();
        unlimitedRecentStickers = s.value("unlimitedRecentStickers", unlimitedRecentStickers).toBool();
        recentStickersMaxCount = s.value("recentStickersMaxCount", recentStickersMaxCount).toInt();
        enableInGameOverlay = s.value("enableInGameOverlay", enableInGameOverlay).toBool();
        overlayAllGames = s.value("overlayAllGames", overlayAllGames).toBool();
        overlayAllowedGames = s.value("overlayAllowedGames", overlayAllowedGames).toStringList();
        enableGameStatus = s.value("enableGameStatus", enableGameStatus).toBool();
        autoLockOnWindowsLock = s.value("autoLockOnWindowsLock", autoLockOnWindowsLock).toBool();
        syncWindowsAccentColor = s.value("syncWindowsAccentColor", syncWindowsAccentColor).toBool();
        hideStoriesBar = s.value("hideStoriesBar", hideStoriesBar).toBool();
        hideSponsoredAds = s.value("hideSponsoredAds", hideSponsoredAds).toBool();
        hidePremiumPromos = s.value("hidePremiumPromos", hidePremiumPromos).toBool();
        fastStartup = s.value("fastStartup", fastStartup).toBool();
        delayStickersLoad = s.value("delayStickersLoad", delayStickersLoad).toBool();
        delayStoriesLoad = s.value("delayStoriesLoad", delayStoriesLoad).toBool();
    }

    void save() const {
        const auto path = getSettingsFilePath();
        QSettings s(path, QSettings::IniFormat);

        s.setValue("cleanTrackingUrls", cleanTrackingUrls);
        s.setValue("directExternalLinks", directExternalLinks);
        s.setValue("enableDownloadsRouter", enableDownloadsRouter);
        s.setValue("unlimitedRecentStickers", unlimitedRecentStickers);
        s.setValue("recentStickersMaxCount", recentStickersMaxCount);
        s.setValue("enableInGameOverlay", enableInGameOverlay);
        s.setValue("overlayAllGames", overlayAllGames);
        s.setValue("overlayAllowedGames", overlayAllowedGames);
        s.setValue("enableGameStatus", enableGameStatus);
        s.setValue("autoLockOnWindowsLock", autoLockOnWindowsLock);
        s.setValue("syncWindowsAccentColor", syncWindowsAccentColor);
        s.setValue("hideStoriesBar", hideStoriesBar);
        s.setValue("hideSponsoredAds", hideSponsoredAds);
        s.setValue("hidePremiumPromos", hidePremiumPromos);
        s.setValue("fastStartup", fastStartup);
        s.setValue("delayStickersLoad", delayStickersLoad);
        s.setValue("delayStoriesLoad", delayStoriesLoad);
        s.sync();
    }

private:
    static QString getSettingsFilePath() {
        const auto base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        QDir().mkpath(base);
        return base + "/custom_features.ini";
    }
};

// Глобальный синглтон настроек с автоматической загрузкой при старте
inline ClientConfig& GetConfig() {
    static ClientConfig config = [] {
        ClientConfig cfg;
        cfg.load();
        return cfg;
    }();
    return config;
}

} // namespace CustomFeatures
