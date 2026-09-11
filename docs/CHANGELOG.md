# Changelog: TelegramCustom

В этом документе зафиксирована полная история коммитов и изменений в кодовой базе проекта.

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
