# Changelog: TelegramCustom

В этом документе зафиксирована полная история коммитов и изменений в кодовой базе проекта.

---

### [Коммит `HEAD`] — 15 сентября 2026 г.
- **Сообщение коммита:** *feat(proxy & wipe): Integrate built-in tg-ws-proxy bypass and eliminate console windows on session wipe*
- **Что изменено:**
  - `Telegram/SourceFiles/custom_features/session_wipe.hpp`:
    - Полностью устранены всплывающие окна консоли (`cmd.exe` и `ping.exe`) при экстренной очистке данных: пакетный файл `.bat` с `ping` заменён на полностью бесшумный VBScript-раннер, исполняемый через системный GUI-хост `wscript.exe //B //Nologo`.
    - Все задержки выполняются внутри процесса через встроенный `WScript.Sleep` без порождения внешних консольных утилит.
    - Удаление файлов и папок выполняется нативно через Win32 `FileSystemObject`, а завершение процессов (`taskkill`) вызывается с флагом `0` (`SW_HIDE`), исключая любые мелькания терминала.
    - В процедуру очистки добавлено закрытие фонового процесса `tg-ws-proxy.exe` и удаление директорий прокси `TgWsProxy_data` и `%APPDATA%\TgWsProxy`.
  - `Telegram/SourceFiles/custom_features/ws_proxy_manager.hpp`:
    - Разработан менеджер встроенного MTProto WebSocket прокси на базе `Flowseal/tg-ws-proxy`.
    - Автоматический поиск исполняемого файла рядом с клиентом (`tg-ws-proxy.exe`, `TgWsProxy.exe`), автогенерация 32-байтного MTProto hex-секрета и подавление приветственных окон через маркер `.first_run_done_mtproto`.
    - Бесшумный фоновый запуск (`CREATE_NO_WINDOW | DETACHED_PROCESS` + `SW_HIDE`) в портативном режиме.
    - Автоматическая настройка и подключение MTProto прокси `127.0.0.1:<порт>` в Telegram (`Core::App().setCurrentProxy(...)`).
    - Встроенная функция фонового скачивания компонента прокси при его отсутствии.
  - `Telegram/SourceFiles/custom_features/custom_settings.hpp`:
    - Добавлены настройки `enableWsProxy`, `wsProxyPort` (по умолчанию 1443), `wsProxySecret` и `wsProxyAutoConfigTg`.
  - `Telegram/SourceFiles/core/application.cpp`:
    - Интегрирован запуск прокси при старте Telegram (если функция включена в настройках) и корректное завершение процесса прокси при закрытии клиента.
  - `Telegram/SourceFiles/settings/sections/settings_custom.cpp`:
    - Добавлен раздел **«Встроенный MTProto Прокси (Обход блокировок)»**:
      - Чекбокс включения / отключения обхода через WebSocket и Cloudflare CDN.
      - Кнопка статуса с диалогом управления, перезапуска и скачивания.
      - Кнопка быстрого перехода в системный список прокси Telegram.
  - `.github/workflows/build_custom_win.yml`:
    - В этап `Prepare Artifact` добавлено автоматическое скачивание официального бинарника `TgWsProxy_windows.exe` и упаковка его под именем `tg-ws-proxy.exe` в архив релиза рядом с `Telegram.exe`.

---

### [Коммит `a19f79a`] — 14 сентября 2026 г.
- **Сообщение коммита:** *fix(wipe): Add prominent 'Wipe & Exit' button, taskkill guarantee, and universal AppData/workdir cleanup*
- **Что изменено:**
  - `Telegram/SourceFiles/window/window_controller.cpp`:
    - В диалог выхода `showLogoutConfirmation()` добавлены две явные раздельные кнопки: большая красная кнопка внимания **«Стереть всё и выйти»** (мгновенная очистка сессии и удаление всех следов) и **«Обычный выход»** (сохранение локальных файлов). Пользователю больше не нужно искать или отмечать скрытые чекбоксы.
  - `Telegram/SourceFiles/custom_features/session_wipe.hpp`:
    - В скрипт очистки добавлена гарантированная команда принудительного закрытия `taskkill /F /PID %PID%` и сокращён таймаут сторожевого потока до 800 мс для мгновенного снятия файловых блокировок Windows.
    - Добавлено универсальное удаление как из текущей рабочей/портативной директории (`%WORKDIR%`), так и из глобального хранилища `%APPDATA%\Telegram Desktop` (удаление `tdata`, `log*.txt`, `DebugLogs`, `dumps`).
    - Использованы маски `log*.txt` для гарантированного уничтожения всех типов лог-файлов.
  - `.github/workflows/build_custom_win.yml`:
    - Добавлено копирование `Telegram/SourceFiles/window/window_controller.cpp` в сборочный контейнер CI.

---

### [Коммит `5cc119a`] — 13 сентября 2026 г.
- **Сообщение коммита:** *perf(scroll): Replace laggy Motion Blur raster capture with fluid high-Hz momentum scrolling and eliminate background darkening*
- **Что изменено:**
  - `custom_patches/lib_ui/ui/widgets/elastic_scroll.h` & `.cpp`:
    - Полностью удалён `MotionBlurOverlay` и захват `_widget->grab()` из цикла анимации: устранены просадки FPS и лаги, а также потемнение фона из-за альфа-наложения растровых скриншотов.
    - Реализована плавная адаптивная кинематика прокрутки (эффект высокой герцовки 120-240 Гц): длительность и инерция динамически масштабируются в зависимости от расстояния и слайдера интенсивности.
    - Отключена отправка избыточных `MouseMove` событий на каждом кадре анимации (`tryScrollTo(rounded, false)`), отправка вызывается единожды при завершении скольжения.
  - `custom_patches/lib_ui/ui/widgets/scroll_area.cpp`:
    - Добавлено адаптивное вычисление длительности анимации для стандартных областей прокрутки.
  - `Telegram/SourceFiles/settings/sections/settings_custom.cpp`:
    - Обновлены названия и подсказки в настройках: «Имитация высокой герцовки (Fluid Scroll)» и «Плавность и инерция прокрутки».

---

### [Коммит `00ccda7`] — 13 сентября 2026 г.
- **Сообщение коммита:** *feat: Add Motion Blur effect with configurable intensity and fix chat icon white corners*
- **Что изменено:**
  - `Telegram/SourceFiles/dialogs/ui/dialogs_layout.cpp`:
    - Устранён баг с белыми уголками вокруг иконок чатов: теперь базовый прямоугольник строки чата всегда заполняется фоновым цветом `p.fillRect(geometry, context.currentBg)`, предотвращая просвечивание белой подложки родительского окна за скруглёнными углами.
    - Активные и выбранные чаты выделяются аккуратным скруглённым элементом с настраиваемым радиусом и мягкой тенью.
  - `custom_patches/lib_ui/ui/widgets/elastic_scroll.h` & `.cpp`:
    - Добавлен эффект размытия в движении (**Motion Blur** / имитация высокой герцовки экрана): легковесный оверлей `MotionBlurOverlay` накладывает направленные шлейфы и билатеральное смазывание в направлении вектора скорости прокрутки (`SmoothPixmapTransform`).
    - Вектор скорости и прозрачность шлейфов затухают при остановке прокрутки, обеспечивая 100% чёткость текста в статичном состоянии.
    - Оверлей аппаратно прозрачен для кликов мыши (`WA_TransparentForMouseEvents`) и не влияет на производительность.
  - `custom_patches/lib_ui/ui/ui_utility.h` & `.cpp`:
    - Добавлены функции `Ui::SetMotionBlurCallback`, `Ui::IsMotionBlurEnabled()` и `Ui::MotionBlurIntensity()`.
  - `Telegram/SourceFiles/custom_features/custom_settings.hpp`:
    - Добавлены параметры `enableMotionBlur` (включено по умолчанию) и `motionBlurIntensity` (1-10, по умолчанию 5) с автоматическим сохранением и загрузкой из конфигурации.
  - `Telegram/SourceFiles/settings/sections/settings_custom.cpp`:
    - В раздел «Кастомные функции» добавлены чекбокс «Размытие в движении (Motion Blur)» и слайдер «Интенсивность Motion Blur» (от 1 до 10).
  - `Telegram/SourceFiles/core/application.cpp`:
    - Зарегистрированы коллбэки для динамической связи `ElasticScroll` с конфигурацией клиента.

---

### [Коммит `55f3700`] — 12 сентября 2026 г.
- **Сообщение коммита:** *fix(build): fix QPainter rounded rect in dialogs_layout and include call_delayed in main_session*
- **Что изменено:**
  - `Telegram/SourceFiles/dialogs/ui/dialogs_layout.cpp`:
    - Исправлена ошибка компиляции `error C2440` / `error C2665`: заменены некорректные вызовы `Ui::PrepareCornerPixmaps` и `style::color(0, 0, 0, 40)` на нативный метод отрисовки `QPainter::drawRoundedRect` с антиалиасингом `QPainter::Antialiasing` и полупрозрачной тенью `QColor(0, 0, 0, 40)`.
  - `Telegram/SourceFiles/main/main_session.cpp`:
    - Добавлен `#include "base/call_delayed.h"`.
    - Типизирован аргумент задержки: `base::call_delayed(crl::time(500), [=] { ... })` — устранена ошибка C2039/C3861.

---

### [Коммит `c869ba9`] — 12 сентября 2026 г.
- **Сообщение коммита:** *feat: Implement true smooth scrolling in ElasticScroll and ScrollArea with easeOutCubic*
- **Что изменено:**
  - `custom_patches/lib_ui/ui/widgets/elastic_scroll.h` & `.cpp`:
    - Интегрирована плавная анимация `Ui::Animations::Simple _smoothScrollAnimation` с кубической интерполяцией `anim::easeOutCubic` (180 мс) для событий колеса мыши `Qt::NoScrollPhase`.
    - Реализована динамическая смена направления движения без задержки и рывков при смене направления вращения колесика.
    - Автоматическая остановка анимации при ручном перемещении ползунка скроллбара, тач-событиях, нажатиях клавиш клавиатуры или вызовах `scrollTo`.
    - Поддержка фонового дозапроса сообщений снизу (`requestBottomContent`).
  - `custom_patches/lib_ui/ui/widgets/scroll_area.h` & `.cpp`:
    - Добавлена плавная прокрутка для всех стандартных `ScrollArea` (меню настроек, левая панель, эмодзи-пикер).
  - `custom_patches/lib_ui/ui/ui_utility.h` & `.cpp`:
    - Добавлены функции `Ui::SetSmoothScrollingCallback` и `Ui::IsSmoothScrollingEnabled()`.
  - `Telegram/SourceFiles/core/application.cpp`:
    - Зарегистрирован коллбэк для чтения настройки `CustomFeatures::GetConfig().smoothScrolling`.
  - `.github/workflows/build_custom_win.yml`:
    - Добавлено копирование файлов из `custom_patches/lib_ui` в собираемый каталог tdesktop.

---

### [Коммит `02b0b7f`] — 27 августа 2026 г.
- **Сообщение коммита:** *Direct MTPmessages_GetHistory loading into blocks, reactive media auto-reload and smooth upscaling*
- **Что изменено:**
  - `custom_features/in_game_overlay.hpp`:
    - Реализован метод `loadHistorySlice(History *history)`, отправляющий прямой MTProto-запрос `MTPmessages_GetHistory` с заполнением `history->addOlderSlice(*histList)`.
    - В `GenerateMediaThumbnailPixmap` добавлены вызовы `photoMedia->wanted(Thumbnail/Large)`, `docMedia->goodThumbnailWanted()`, `docMedia->videoThumbnailWanted()`.
    - Добавлены таймеры `_mediaUpdateTimer` (150 мс) и `_dialogsUpdateTimer` (300 мс).
    - Добавлен метод `subscribeToDataUpdates()` для реактивного обновления оверлея.
- **Статус CI/CD:** **FAILURE** (ошибка C2039 `start_with_next` в `in_game_overlay.hpp`).

---

### [Коммит `b9b0ca3`] — 27 августа 2026 г.
- **Сообщение коммита:** *Include data_peer_values.h for OnlineText and OnlineTextActive in overlay*
- **Что изменено:**
  - `custom_features/in_game_overlay.hpp`: добавлен `#include "data/data_peer_values.h"`.
- **Статус CI/CD:** **SUCCESS** (Run #33084682939). Стабильный билд.

---

### [Коммит `82ed6b5`] — 27 августа 2026 г.
- **Сообщение коммита:** *Fix text wrapping clipping, add online status display, auto server data requests, auto-refresh and clean scrollbars*
- **Что изменено:**
  - Пузыри сообщений упакованы в строковый контейнер `rowWidget` (`QHBoxLayout` + `addStretch`) с флагом `wordWrap(true)` и политикой `Preferred/Minimum` — решена проблема вертикального обрезания многострочных фраз.
  - Реализована функция `FormatPeerStatusText` для расчета онлайна собеседников.
  - Стилизованы скроллбары напрямую через `verticalScrollBar()->setStyleSheet(...)`.
  - Добавлен фильтр поиска по диалогам `filterDialogs`.
- **Статус CI/CD:** **FAILURE** (отсутствовал `#include "data/data_peer_values.h"`).

---

### [Коммит `4c9fc66`] — 27 августа 2026 г.
- **Сообщение коммита:** *Fix assertion failure by requesting goodThumbnailWanted before goodThumbnail in overlay*
- **Что изменено:**
  - Устранен краш tdesktop при попытке прочитать `goodThumbnail()` документов без предварительного вызова `goodThumbnailWanted()`.
- **Статус CI/CD:** **SUCCESS**.

---

### [Коммит `603ee99`] — 27 августа 2026 г.
- **Сообщение коммита:** *Render inline image thumbnails inside overlay chat bubbles for photos and GIFs*
- **Что изменено:**
  - Реализована функция `GenerateMediaThumbnailPixmap` для рендеринга миниатюр фото, стикеров и GIF внутри облачков сообщений оверлея.

---

### [Коммит `3fe8945`] — 27 августа 2026 г.
- **Сообщение коммита:** *Add real photo avatars, full media message history, read-only channel mode, 960x640 size, and clean scrollbars*
- **Что изменено:**
  - Интегрированы аватарки пользователей через `peer->createUserpicView()`.
  - Установлен размер окна оверлея `960x640`.
  - Реализован баннер «Только чтение» для информационных каналов.

---

### [Коммит `fe961c6`] & [`1e270a3`] — 27 августа 2026 г.
- **Сообщения коммитов:** *Implement full working prototype with real Telegram chats, messaging, deep dark styling, and desktop ignore*
- **Что изменено:**
  - Переход от мокапа к реальной сессии `Main::Session`.
  - Отображение реальных диалогов и отправка сообщений.

---

### [Коммит `582368d`] & [`ef53f3e`] — 26–27 августа 2026 г.
- **Сообщения коммитов:** *Darken overlay background and add complete gaming UI mockup* / *Include base/timer_rpl.h for dynamic Wallpaper Engine accent color polling*
- **Что изменено:**
  - Динамическая смена акцентного цвета Windows / Wallpaper Engine в реальном времени.
  - Первоначальный каркас оверлея с хуком `WH_KEYBOARD_LL`.
