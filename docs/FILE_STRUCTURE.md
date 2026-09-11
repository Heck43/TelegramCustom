# File Structure: TelegramCustom

В этом документе приведена полная карта файлов и директорий проекта с акцентом на кастомную функциональность и точки интеграции с upstream-кодом Telegram Desktop.

---

## 1. Карта директорий проекта

```text
tdesktop-dev/
├── .agent_handoff.md            # Компактный handoff для агентов
├── .github/
│   └── workflows/
│       └── build_custom_win.yml # Главный CI/CD пайплайн сборки под Windows x64
├── docs/                        # Внутренняя система документации проекта
│   ├── README.md                # Главный хаб документации
│   ├── PROJECT_OVERVIEW.md      # Описание проекта и возможностей
│   ├── ARCHITECTURE.md          # Архитектура и схемы компонентов
│   ├── FILE_STRUCTURE.md        # Текущий файл (карта файлов)
│   ├── WORKFLOW.md              # Пошаговые сценарии работы
│   ├── CURRENT_STATE.md         # Текущий статус готовности и багов
│   ├── TASKS.md                 # Список задач и TODO
│   ├── ROADMAP.md               # План развития
│   ├── DECISIONS.md             # Принятые архитектурные решения
│   ├── PROBLEMS.md              # Реестр проблем и ошибок
│   ├── CHANGELOG.md             # История изменений по коммитам
│   ├── SETUP.md                 # Руководство по сборке и запуску
│   └── HANDOFF.md               # Точка продолжения для следующего ИИ
├── Telegram/
│   ├── CMakeLists.txt           # Сборочный конфигуратор CMake
│   └── SourceFiles/
│       ├── custom_features/     # [НАШИ ФАЙЛЫ] Каталог кастомных подсистем
│       │   ├── custom_settings.hpp
│       │   ├── in_game_overlay.hpp
│       │   ├── downloads_router.hpp
│       │   ├── clean_urls.hpp
│       │   ├── auto_lock_win.hpp
│       │   └── game_activity_status.hpp
│       ├── settings/sections/   # Настройки клиента
│       │   ├── settings_custom.h
│       │   ├── settings_custom.cpp
│       │   └── settings_main.cpp
│       ├── core/                # Ядро tdesktop
│       │   ├── click_handler_types.cpp
│       │   └── file_utilities.cpp
│       ├── data/                # Модель данных и сущности
│       │   ├── data_document.cpp
│       │   ├── stickers/data_stickers.cpp
│       │   └── components/sponsored_messages.cpp
│       ├── chat_helpers/        # Хелперы чата
│       │   └── stickers_list_widget.cpp
│       ├── platform/win/        # Платформенный слой Windows
│       │   ├── main_window_win.cpp
│       │   └── windows_dlls.h
│       └── window/themes/       # Система тем оформления
│           ├── window_theme.cpp
│           └── window_themes_embedded.cpp
└── download_log.py              # Скрипт для скачивания логов сборок GitHub Actions
```

---

## 2. Подробное описание кастомных файлов

### `Telegram/SourceFiles/custom_features/custom_settings.hpp`
* **Назначение:** Определение структуры настроек `ClientConfig` и сохранение/загрузка конфигурации.
* **Ответственность:** Управление флагами (активность оверлея, автоблокировка, роутер загрузок, очистка ссылок, 300 стикеров, список разрешенных процессов игр).
* **Связи:** Подключается почти во всех кастомных хуках. Предоставляет глобальную функцию `GetConfig()`.
* **Критичность:** Максимальная. Падение или повреждение структуры ломает всю конфигурацию.

### `Telegram/SourceFiles/custom_features/in_game_overlay.hpp`
* **Назначение:** Реализация внутриигрового оверлея Telegram.
* **Содержимое:**
  - `InGameOverlayManager` (синглтон, перехватчик клавиш через `SetWindowsHookEx(WH_KEYBOARD_LL)`).
  - `InGameOverlayWidget` (главное безрамочное графическое окно оверлея `960x640`).
  - `GeneratePeerAvatarPixmap()` (генерация круглых аватарок из сессии).
  - `FormatPeerStatusText()` (расчет онлайна и статуса собеседника).
  - `GenerateMediaThumbnailPixmap()` (извлечение и отрисовка превью фото и гифок).
  - `loadHistorySlice()` (прямой MTProto-запрос сообщений и вставка в `history->addOlderSlice`).
* **Критичность:** Высокая. Основной фронт текущей работы.

### `Telegram/SourceFiles/custom_features/downloads_router.hpp`
* **Назначение:** Категоризация загружаемых файлов по папкам.
* **Содержимое:** Метод `CategorizeFile(fileName)` и `GetSuggestedDownloadPath(fileName)`.
* **Связи:** Вызывается из `core/file_utilities.cpp` и `data/data_document.cpp`.
* **Критичность:** Средняя. При сбое файлы скачиваются в дефолтную папку.

### `Telegram/SourceFiles/custom_features/clean_urls.hpp`
* **Назначение:** Очистка внешних ссылок от маркетинговых меток трекинга.
* **Содержимое:** Функция `CleanTrackingParameters(rawUrl)` (вырезает `utm_*`, `si`, `fbclid`, `yclid` и др.).
* **Связи:** Вызывается из `core/click_handler_types.cpp`.
* **Критичность:** Низкая.

### `Telegram/SourceFiles/custom_features/auto_lock_win.hpp`
* **Назначение:** Перехват событий блокировки Windows.
* **Содержимое:** Класс `WindowsSessionLockWatcher` с регистрацией в `WTSRegisterSessionNotification`.
* **Связи:** Вызывается из `platform/win/main_window_win.cpp` при получении события `WM_WTSSESSION_CHANGE`.
* **Критичность:** Низкая.

### `Telegram/SourceFiles/custom_features/game_activity_status.hpp`
* **Назначение:** Детекция запущенных игр через Toolhelp32 snapshot.
* **Содержимое:** Класс `GameActivityDetector` со списком известных exe-файлов (CS2, Dota 2, Minecraft и др.).
* **Критичность:** Низкая (экспериментальная фича).

---

## 3. Модифицированные upstream-файлы tdesktop

### `Telegram/SourceFiles/settings/sections/settings_custom.cpp` & `settings_custom.h`
* **Что изменено:** Создан полноценный раздел настроек `Настройки модификации` в главном меню Telegram Desktop со всеми переключателями и интерактивным селектором игр.

### `Telegram/SourceFiles/settings/sections/settings_main.cpp`
* **Что изменено:** Добавлен пункт перехода в `Настройки модификации` с иконкой шестеренки в основном меню настроек Telegram.

### `Telegram/SourceFiles/platform/win/main_window_win.cpp`
* **Что изменено:**
  1. Добавлена инициализация оверлея: `InGameOverlayManager::Instance().init((HWND)winId())`.
  2. Добавлен перехват `WM_WTSSESSION_CHANGE` для автоблокировки по `Win + L`.

### `Telegram/SourceFiles/data/components/sponsored_messages.cpp`
* **Что изменено:** Во всех методах обработки и добавления спонсорских сообщений добавлена проверка `if (CustomFeatures::GetConfig().hideSponsoredAds) return;`.

### `Telegram/SourceFiles/data/stickers/data_stickers.cpp` & `chat_helpers/stickers_list_widget.cpp`
* **Что изменено:** Захардкоженный лимит недавних стикеров (обычно 20-30) заменен на чтение `CustomFeatures::GetConfig().recentStickersMaxCount` (300).

### `Telegram/SourceFiles/window/themes/window_themes_embedded.cpp` & `window_theme.cpp`
* **Что изменено:** Добавлен таймер периодического опроса реестра DWM на изменение акцентного цвета Windows и мгновенное динамическое перекрашивание палитры темы без перезапуска клиента.

### `Telegram/SourceFiles/core/click_handler_types.cpp`
* **Что изменено:** Добавлен пропуск подтверждения перехода по ссылкам (`directExternalLinks`) и прогон URL через `CleanTrackingParameters`.

### `Telegram/SourceFiles/core/file_utilities.cpp` & `data/data_document.cpp`
* **Что изменено:** Интегрирован вызов `DownloadsRouter::GetSuggestedDownloadPath`.

---

## 4. Скрипты и конфигурация CI/CD

### `.github/workflows/build_custom_win.yml`
* **Назначение:** CI/CD скрипт GitHub Actions для полной сборки проекта.
* **Что делает:**
  1. Разворачивает Windows runner.
  2. Клонирует официальный репозиторий `telegramdesktop/tdesktop` ветки `v7.0.9`.
  3. Копирует поверх наши кастомные файлы из репозитория `Heck43/TelegramCustom`.
  4. Собирает проект через CMake и MSVC 2022.
  5. Архивирует и выкладывает готовый артефакт `TelegramCustom-Windows-x64.zip`.
