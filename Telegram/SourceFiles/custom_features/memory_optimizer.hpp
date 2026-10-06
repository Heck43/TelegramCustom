#pragma once

#include <QtCore/QCoreApplication>
#include <QtGui/QPixmapCache>
#include "custom_features/custom_settings.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif // Q_OS_WIN

namespace CustomFeatures {

class MemoryOptimizer {
public:
    static void InitStartupLimits() {
        const auto &cfg = GetConfig();
        const int limitKb = (cfg.pixmapCacheLimitMb > 0 ? cfg.pixmapCacheLimitMb : 32) * 1024;
        QPixmapCache::setCacheLimit(limitKb);
    }

    static void TrimWorkingSet() {
        // 1. Очищаем устаревшие графические растровые буферы Qt
        QPixmapCache::clear();

#ifdef Q_OS_WIN
        // 2. Системный сброс неактивных страниц рабочего набора процесса
        const auto process = GetCurrentProcess();
        if (process) {
            SetProcessWorkingSetSize(process, (SIZE_T)-1, (SIZE_T)-1);
            EmptyWorkingSet(process);
        }
#endif // Q_OS_WIN
    }

    static void OnMinimizedOrHidden() {
        if (GetConfig().trimMemoryOnMinimize) {
            TrimWorkingSet();
        }
    }
};

} // namespace CustomFeatures
