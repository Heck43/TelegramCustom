# Architecture: TelegramCustom

Документ описывает архитектурное устройство модификации **TelegramCustom**, её взаимодействие с базовой кодовой базой Telegram Desktop (tdesktop) и внутреннее устройство подсистем.

---

## 1. Общая архитектурная модель

Проект построен по принципу **«модульного наложения» (overlay patch model)** поверх официального репозитория `telegramdesktop/tdesktop` (версия `v7.0.9`). 

Вместо масштабного инвазивного рефакторинга upstream-кода, кастомная функциональность инкапсулирована в отдельном пространстве имен `CustomFeatures` в каталоге `Telegram/SourceFiles/custom_features/`, а в ключевых точках оригинального кода tdesktop сделаны минимальные точечные врезки (hooks/interceptors).

```text
+-----------------------------------------------------------------------+
|                       Telegram Desktop (v7.0.9)                       |
|                                                                       |
|  +---------------------+  +--------------------+  +----------------+  |
|  |     Main::Domain    |  |    Data::Session   |  |   ApiWrap &    |  |
|  |  (Core Application) |  |   (Storage & Model)|  |    MTProto     |  |
|  +----------+----------+  +---------+----------+  +-------+--------+  |
|             |                       |                     |           |
+-------------|-----------------------|---------------------|-----------+
              |                       |                     |
              v                       v                     v
+-----------------------------------------------------------------------+
|                     CustomFeatures Layer (Наш код)                    |
|                                                                       |
|   +---------------------------------------------------------------+   |
|   |                      ClientConfig (INI)                       |   |
|   |           Хранение настроек в %LOCALAPPDATA%/custom_features.ini  |
|   +---------------------------------------------------------------+   |
|         |                |                  |                 |       |
|         v                v                  v                 v       |
|  +--------------+ +---------------+ +---------------+ +------------+  |
|  | InGameOverlay| |DownloadsRouter| | ThemeWatcher  | |  AutoLock  |  |
|  | (Manager &   | | (Категоризация| |(DWM / Registry| | (WTSAPI32  |  |
|  |   Widget)    | |   файлов)     | | Accent Sync)  | |  Win + L)  |  |
|  +--------------+ +---------------+ +---------------+ +------------+  |
+-----------------------------------------------------------------------+
```

---

## 2. Архитектура In-Game Overlay

Оверлей представляет собой полностью независимый графический слой, работающий в рамках того же процесса, но в отдельном окне с флагами инструмента и верхнего z-порядка.

### Архитектурная схема компонентов оверлея:

```text
[ Windows Global Keyboard Hook (WH_KEYBOARD_LL) ]
                         |
           (Shift + ~ / Shift + F11)
                         v
         [ CustomFeatures::InGameOverlayManager ]
                         |
                         +---> Проверка активного процесса (EnumWindows / GetForegroundWindow)
                         |     (Фильтрация explorer.exe, dwm.exe, системных окон)
                         v
         [ CustomFeatures::InGameOverlayWidget ]
                         |
      +------------------+-------------------+
      |                                      |
      v                                      v
[ Левая панель: Диалоги ]             [ Правая панель: Чат ]
- QLineEdit (Поиск)                   - Шапка (Аватар, Имя, Онлайн-статус)
- QScrollArea (_chatListWidget)       - QScrollArea (_messagesWidget)
  * addChatRowWidget()                  * addMessageBubble() (Многострочный wrap)
  * GeneratePeerAvatarPixmap()          * GenerateMediaThumbnailPixmap() (Превью)
                                      - QLineEdit (_msgInput) -> Отправка в API
```

### Поток данных при открытии чата в оверлее:

```text
Пользователь кликает на диалог
            |
            v
InGameOverlayWidget::selectChat(History *history)
            |
            +---> 1. Установка заголовка: имя, аватар и онлайн (FormatPeerStatusText)
            |
            +---> 2. Предварительный рендер кэшированных сообщений (renderActiveMessages)
            |
            +---> 3. Прямой запрос истории в Telegram MTProto API:
                     MTPmessages_GetHistory(peer, offsetId, loadCount=50)
                     |
                     v
             (Ответ серверов Telegram)
                     |
                     +---> history->owner().processUsers()
                     +---> history->owner().processChats()
                     +---> history->addOlderSlice(*histList) -> Создает блоки HistoryItem
                     |
                     v
             renderActiveMessages(true) -> Обновляет переписку с анимацией скролла вниз
```

### Поток данных при загрузке медиа (фото/GIF):

```text
HistoryItem содержит медиа
            |
            v
GenerateMediaThumbnailPixmap()
            |
            +---> photoMedia->wanted(Thumbnail/Large)
            +---> docMedia->goodThumbnailWanted()
            +---> docMedia->thumbnailWanted()
            |
            v
Есть готовое изображение?
  ├── ДА  ---> Возвращает масштабированный QPixmap с DPR 2.0 и скруглением 20px
  └── НЕТ ---> Возвращает сглаженный микро-эскиз (inlineThumbnail)
                 |
        (Фоновый загрузчик завершил скачивание)
                 v
        session->downloaderTaskFinished() / viewRepaintRequest()
                 v
        _mediaUpdateTimer.start(150) -> renderActiveMessages(false)
                 v
        Плейсхолдер бесшовно заменяется на четкое HD-изображение
```

---

## 3. Архитектура темы: Windows / Wallpaper Engine Accent Sync

Для синхронизации цвета с фоновым рисунком или Wallpaper Engine используется фоновый опрос системных источников цвета Windows:

```text
[ Windows DWM / Registry Accent Color ]
  ├── DwmGetColorizationColor()
  ├── HKCU\Software\Microsoft\Windows\DWM\AccentColor (ABGR)
  └── HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Accent\AccentPalette (RGBA)
                         |
                         v  (Каждые 1500 мс)
            [ Window::Theme::BackgroundTimer ]
                         |
                         v  (Цвет изменился?)
            [ Window::Theme::ApplyDefaultWithPath() ]
                         |
                         v
     [ Window::Theme::ComputePaletteWithAccent(color) ]
                         |
                         v
             [ Перерисовка UI Telegram ]
```

---

## 4. Архитектура умного роутера загрузок (DownloadsRouter)

При сохранении любого файла или открытии системного диалога `Save As`:

```text
Пользователь инициирует загрузку файла (data_document.cpp / file_utilities.cpp)
                         |
                         v
     CustomFeatures::DownloadsRouter::GetSuggestedDownloadPath(fileName)
                         |
                         v
         DownloadsRouter::CategorizeFile(extension)
                         |
     ├── .jpg/.png/.webp... -> Downloads/Telegram Desktop/Изображения
     ├── .mp3/.flac/.wav... -> Downloads/Telegram Desktop/Музыка
     ├── .mp4/.mkv/.avi...  -> Downloads/Telegram Desktop/Видео
     ├── .pdf/.docx/.txt... -> Downloads/Telegram Desktop/Документы
     ├── .zip/.rar/.7z...   -> Downloads/Telegram Desktop/Архивы
     └── .exe/.msi/.bat...  -> Downloads/Telegram Desktop/Программы
                         |
                         v
       Создание подкаталога (QDir().mkpath) и возврат результирующего пути
```

---

## 5. Архитектура автоблокировки (Win + L)

```text
Пользователь нажимает Win + L в Windows
                         |
                         v
  Windows посылает сообщение WM_WTSSESSION_CHANGE в главное окно Telegram
                         |
                         v
MainWindow::processEvent() -> WindowsSessionLockWatcher::HandleMessage()
                         |
                   (WTS_SESSION_LOCK)
                         |
                         v  (Проверка настройки autoLockOnWindowsLock)
            Core::App().domain().local().lockByPasscode()
                         |
                         v
          Telegram переходит в заблокированное состояние (Passcode Lock)
```

---

## 6. Механизм хранения настроек (ClientConfig)

Все пользовательские переключатели модификации сохраняются в стандартном формате Windows INI:
- **Расположение:** `%LOCALAPPDATA%/TelegramCustom/custom_features.ini` (или каталог AppData Qt).
- **Класс:** `CustomFeatures::ClientConfig` (синглтон через функцию `GetConfig()`).
- **Синхронизация:** При изменении любого чекбокса в меню `Settings -> Custom Features` вызывается `GetConfig().save()`, который синхронизирует значения на диск через `QSettings::sync()`.
