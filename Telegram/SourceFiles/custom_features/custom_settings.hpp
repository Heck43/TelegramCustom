#pragma once

#include <QString>
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
    bool unlimitedRecentStickers = true;   // Расширенный список стикеров (1000 шт.)
    int recentStickersMaxCount = 1000;     // Лимит недавних стикеров

    // 3. Гейминг и система
    bool enableInGameOverlay = false;      // Оверлей в играх (Shift + ~)
    bool enableGameStatus = false;         // Авто-статус игры в профиле
    bool autoLockOnWindowsLock = false;    // Авто-блокировка при Win + L (по умолчанию выкл, пока не задан пароль)

    // 4. Интерфейс (UI)
    bool enableMicaBackdrop = true;        // Эффект стекла Windows 10/11 (Acrylic / Mica)
    bool modernRoundedStyle = true;        // Современный стиль интерфейса (плавные скругления)
    bool hideStoriesBar = false;           // Скрыть плашку историй
    bool hideSponsoredAds = true;          // Скрыть спонсорские посты
    bool hidePremiumPromos = true;         // Скрыть промо Premium

    void load() {
        const auto path = getSettingsFilePath();
        QSettings s(path, QSettings::IniFormat);

        cleanTrackingUrls = s.value("cleanTrackingUrls", cleanTrackingUrls).toBool();
        directExternalLinks = s.value("directExternalLinks", directExternalLinks).toBool();
        enableDownloadsRouter = s.value("enableDownloadsRouter", enableDownloadsRouter).toBool();
        unlimitedRecentStickers = s.value("unlimitedRecentStickers", unlimitedRecentStickers).toBool();
        recentStickersMaxCount = s.value("recentStickersMaxCount", recentStickersMaxCount).toInt();
        enableInGameOverlay = s.value("enableInGameOverlay", enableInGameOverlay).toBool();
        enableGameStatus = s.value("enableGameStatus", enableGameStatus).toBool();
        autoLockOnWindowsLock = s.value("autoLockOnWindowsLock", autoLockOnWindowsLock).toBool();
        enableMicaBackdrop = s.value("enableMicaBackdrop", enableMicaBackdrop).toBool();
        modernRoundedStyle = s.value("modernRoundedStyle", modernRoundedStyle).toBool();
        hideStoriesBar = s.value("hideStoriesBar", hideStoriesBar).toBool();
        hideSponsoredAds = s.value("hideSponsoredAds", hideSponsoredAds).toBool();
        hidePremiumPromos = s.value("hidePremiumPromos", hidePremiumPromos).toBool();
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
        s.setValue("enableGameStatus", enableGameStatus);
        s.setValue("autoLockOnWindowsLock", autoLockOnWindowsLock);
        s.setValue("enableMicaBackdrop", enableMicaBackdrop);
        s.setValue("modernRoundedStyle", modernRoundedStyle);
        s.setValue("hideStoriesBar", hideStoriesBar);
        s.setValue("hideSponsoredAds", hideSponsoredAds);
        s.setValue("hidePremiumPromos", hidePremiumPromos);
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
