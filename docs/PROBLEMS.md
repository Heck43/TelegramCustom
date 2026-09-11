# Problem Registry: TelegramCustom

В этом реестре собраны все проблемы, ошибки, баги и спорные места, которые встречались в проекте.

---

### PRB-001: Ошибка компиляции MSVC на коммите `02b0b7f` (`rpl::start_with_next`)
- **Статус:** **АКТИВНА / БЛОКЕР СБОРКИ (CRITICAL)**
- **Где находится:** `Telegram/SourceFiles/custom_features/in_game_overlay.hpp` (строки 355–375 в методе `subscribeToDataUpdates()`).
- **Как проявляется:** Сборка в GitHub Actions (Run #33093353897) падает с ошибками:
  `error C2039: 'start_with_next': is not a member of 'rpl'`,
  `error C2593: 'operator |' is ambiguous`.
- **Причина:** Использован синтаксис pipe-оператора rpl `session->downloaderTaskFinished() | rpl::start_with_next(...)` без подключения полного заголовочного файла `<rpl/rpl.h>` или с неподдерживаемой сигнатурой.
- **Что уже пробовали:** Первоначальный коммит `02b0b7f`.
- **Что сработало:** Пока не исправлено.
- **Что не сработало:** Прямой вызов pipe-оператора с `rpl::start_with_next`.
- **Что нужно сделать:** Заменить вызов на нативный метод подписки продюсера:
  ```cpp
  session->downloaderTaskFinished().start([=] {
      if (this->isVisible() && _activeHistory) {
          _mediaUpdateTimer.start(150);
      }
  }, _lifetime);
  ```
  (Аналогично для `viewRepaintRequest` и `chatsListChanges`).
- **Критичность:** **Критично.** Без этого не собирается `Telegram.exe`.

---

### PRB-002: Ошибка компиляции `OnlineTextActive: is not a member of Data`
- **Статус:** **ИСПРАВЛЕНО (RESOLVED)**
- **Где находилась:** `in_game_overlay.hpp`, коммит `82ed6b5`.
- **Как проявлялась:** Сборка #33078682876 упала на объявлении `Data::OnlineTextActive`.
- **Причина:** Был подключен `data_user.h`, в котором объявлены свойства пользователя, но функции форматирования текста онлайна находятся в `data/data_peer_values.h`.
- **Что сработало:** Добавление `#include "data/data_peer_values.h"` в коммите `b9b0ca3`. Билд #33084682939 стал зеленым.

---

### PRB-003: Краш клиента при попытке рендера превью гифок
- **Статус:** **ИСПРАВЛЕНО (RESOLVED)**
- **Где находилась:** `in_game_overlay.hpp`, коммит `603ee99`.
- **Как проявлялась:** При открытии чата с гифками клиент вылетал с ошибкой ассерта: `Assertion Failed! "(_flags & Flag::GoodThumbnailWanted) != 0"`.
- **Причина:** Нарушение контракта tdesktop: чтение `goodThumbnail()` требует предварительного вызова `goodThumbnailWanted()`.
- **Что сработало:** Добавление `docMedia->goodThumbnailWanted()` перед чтением изображения в коммите `4c9fc66`.

---

### PRB-004: Обрезание многострочных сообщений по высоте
- **Статус:** **ИСПРАВЛЕНО (RESOLVED)**
- **Где находилась:** `in_game_overlay.hpp`, метод `addMessageBubble`.
- **Как проявлялась:** Длинный текст в облачке сообщения обрезался по нижнему краю, показывая только верхнюю строчку.
- **Причина:** `QVBoxLayout::addWidget(bubble, 0, AlignLeft)` ломал расчет `heightForWidth` в Qt.
- **Что сработало:** Упаковка пузыря в `rowWidget` во всю ширину с `QHBoxLayout` и `addStretch` в коммите `82ed6b5`.

---

### PRB-005: Отсутствие сообщений в не открывавшихся ранее чатах («Марат»)
- **Статус:** **В ПРОЦЕССЕ ВЕРИФИКАЦИИ (PENDING VERIFICATION)**
- **Где находится:** `in_game_overlay.hpp`, методы `selectChat` и `loadHistorySlice`.
- **Как проявлялась:** При переключении на чат в оверлее отображалось только последнее сообщение, остальная переписка была пустой (скриншот `uploaded_media_1_1787846899901.png`).
- **Причина:** Вызов `session->api().requestHistory()` не создавал структуры `HistoryItem` в `history->blocks`.
- **Что сделано:** Написан метод `loadHistorySlice`, запрашивающий `MTPmessages_GetHistory` и скармливающий ответ в `history->addOlderSlice(*histList)`.
- **Что нужно сделать:** Собрать билд (после фикса PRB-001) и протестировать на клиенте.

---

### PRB-006: Размытые / пиксельные миниатюры фото и анимаций
- **Статус:** **В ПРОЦЕССЕ ВЕРИФИКАЦИИ (PENDING VERIFICATION)**
- **Где находится:** `in_game_overlay.hpp`, метод `GenerateMediaThumbnailPixmap`.
- **Как проявлялась:** Изображения и гифки отображались в виде увеличенных микро-эскизов 20x20 px (скриншот `uploaded_media_0_1787846899901.png`).
- **Причина:** Оверлей не запрашивал скачивание полноразмерных эскизов и не перерисовывался после их фонового скачивания.
- **Что сделано:** Добавлены запросы `photoMedia->wanted(Thumbnail/Large)`, `docMedia->videoThumbnailWanted()` и реактивное обновление по таймеру `_mediaUpdateTimer`.
- **Что нужно сделать:** Собрать билд (после фикса PRB-001) и протестировать на клиенте.
