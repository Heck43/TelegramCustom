# HANDOFF: Инструкция для продолжения разработки

Этот документ предназначен для разработчика или нового ИИ, который принимает проект прямо сейчас. Вся ключевая информация для продолжения работы без потери контекста сведена в этот файл.

---

## Project
**TelegramCustom** — кастомный форк официального Telegram Desktop (tdesktop v7.0.9) для Windows x64 с встроенным игровым оверлеем (In-Game Overlay), блокировкой рекламы, динамическим цветом темы, категоризацией загрузок и расширенным функционалом.

---

## Goal
Создать автономный, стабильный и визуально выверенный внутриигровой оверлей, открывающийся по хоткею `Shift + ~` поверх любых 3D-игр, способный отображать реальные диалоги, полную историю закрытых чатов (через MTProto), четкие HD-превью картинок/гифок и реальный онлайн собеседников.

---

## Current state
- Рабочая ветка git: `main`. Рабочее дерево чистое (`nothing to commit, working tree clean`).
- Последний коммит в репозитории: **`02b0b7f`**.
- Статус сборки в GitHub Actions: **FAILURE** (Run #33093353897).
- Предыдущий коммит **`b9b0ca3`** собирался **УСПЕШНО** (Run #33084682939).

---

## Completed
1. Глобальный перехватчик клавиш `Shift + ~` / `Shift + F11` (`WH_KEYBOARD_LL`) с фильтрацией системных окон.
2. Темный интерфейс `960x640` с возможностью перетаскивания мышью и закрытия по `Esc`.
3. Устранение обрезания многострочного текста в сообщениях через упаковку в `rowWidget` (`QHBoxLayout` + `addStretch`).
4. Нативная стилизация темных скроллбаров на `verticalScrollBar()`.
5. Отображение реального статуса собеседника («в сети» `#38BDF8`, время отсутствия, число участников).
6. Поиск по списку чатов в реальном времени.
7. Баннер «Только чтение» для каналов.
8. Отправка сообщений в активный чат через `session->api().sendMessage(...)`.

---

## In progress
- Исправление ошибки сборки компилятора MSVC в методе `subscribeToDataUpdates()` в файле `in_game_overlay.hpp`.
- Верификация серверной загрузки сообщений закрытых чатов (`loadHistorySlice`) и HD-превью картинок.

---

## Known problems
- **PRB-001 (CRITICAL):** Ошибка `error C2039: 'start_with_next': is not a member of 'rpl'` при сборке в GitHub Actions из-за неверного синтаксиса pipe-оператора в строках 355–375 файла `in_game_overlay.hpp`.

---

## Important decisions
1. **Не убирать `rowWidget`** при добавлении сообщений в `_messagesLayout`. Иначе длинный текст снова обрежется до одной строки.
2. **Не вызывать `docMedia->goodThumbnail()` без `goodThumbnailWanted()`** — иначе клиент крашится на ассерте.
3. **Использовать `history->addOlderSlice(*histList)`** для добавления серверных сообщений в память UI.
4. **Не трогать логику `downloads_router.hpp` и `custom_settings.hpp`** без явной необходимости.

---

## Files to know
- [`Telegram/SourceFiles/custom_features/in_game_overlay.hpp`](../Telegram/SourceFiles/custom_features/in_game_overlay.hpp) — **Основной файл работы.** Содержит весь код оверлея, UI, загрузки истории и миниатюр.
- [`.github/workflows/build_custom_win.yml`](../.github/workflows/build_custom_win.yml) — Пайплайн сборки в GitHub Actions.
- [`Telegram/SourceFiles/settings/sections/settings_custom.cpp`](../Telegram/SourceFiles/settings/sections/settings_custom.cpp) — Раздел настроек модификации.

---

## Remaining tasks
1. Исправить подписку RPL в `in_game_overlay.hpp` (строки 355–375).
2. Запушить коммит в ветку `main` и убедиться в успешной сборке в GitHub Actions.
3. Проверить на собранном бинарнике подгрузку истории в чате «Марат» и четкость миниатюр.

---

## Last action
Проведен полный аудит кодовой базы и создана система документации в каталоге `/docs/`. Исходный код не модифицировался.

---

## Exact stopping point
Работа остановилась на коммите **`02b0b7f`**. Файл `in_game_overlay.hpp` содержит синтаксическую ошибку компиляции C++ в строках 356–374:
```cpp
    void subscribeToDataUpdates() {
        const auto session = GetActiveSession();
        if (!session) return;

        session->downloaderTaskFinished(
        ) | rpl::start_with_next([=] {
            if (this->isVisible() && _activeHistory) {
                _mediaUpdateTimer.start(150);
            }
        }, _lifetime);

        session->data().viewRepaintRequest(
        ) | rpl::start_with_next([=](const auto &) {
            if (this->isVisible() && _activeHistory) {
                _mediaUpdateTimer.start(150);
            }
        }, _lifetime);

        session->data().chatsListChanges(
        ) | rpl::start_with_next([=](auto *) {
            if (this->isVisible()) {
                _dialogsUpdateTimer.start(300);
            }
        }, _lifetime);
    }
```

---

## Next action (Что делать первым шагом)

1. Открыть [`Telegram/SourceFiles/custom_features/in_game_overlay.hpp`](../Telegram/SourceFiles/custom_features/in_game_overlay.hpp).
2. В методе `subscribeToDataUpdates()` заменить вызовы pipe-оператора с `rpl::start_with_next` на нативный метод продюсера `.start(...)`:
   ```cpp
    void subscribeToDataUpdates() {
        const auto session = GetActiveSession();
        if (!session) return;

        session->downloaderTaskFinished(
        ).start([=] {
            if (this->isVisible() && _activeHistory) {
                _mediaUpdateTimer.start(150);
            }
        }, _lifetime);

        session->data().viewRepaintRequest(
        ).start([=](const auto &) {
            if (this->isVisible() && _activeHistory) {
                _mediaUpdateTimer.start(150);
            }
        }, _lifetime);

        session->data().chatsListChanges(
        ).start([=](auto *) {
            if (this->isVisible()) {
                _dialogsUpdateTimer.start(300);
            }
        }, _lifetime);
    }
   ```
3. Выполнить команду:
   ```bash
   git add Telegram/SourceFiles/custom_features/in_game_overlay.hpp docs/
   git commit -m "Fix RPL subscription syntax in in_game_overlay and update docs"
   git push origin main
   ```
4. Проверить статус сборки в GitHub Actions через PowerShell:
   ```powershell
   $runs = Invoke-RestMethod -Uri "https://api.github.com/repos/Heck43/TelegramCustom/actions/runs?per_page=1" -Headers @{ 'User-Agent' = 'Dev' }
   $runs.workflow_runs | Select-Object id, head_sha, status, conclusion
   ```

---

## DO NOT BREAK
- **Не возвращайте старый способ добавления сообщений:** `_messagesLayout->addWidget(bubble, 0, Qt::AlignLeft)` гарантированно сломает перенос строк и обрежет текст сообщений.
- **Не удаляйте `loadHistorySlice`:** Без него закрытые чаты будут открываться с 1 сообщением.
- **Не удаляйте `docMedia->goodThumbnailWanted()`:** Приведет к падению приложения при рендере гифок.

---

## Verification needed
После успешной сборки протестировать собранный клиент:
- Открыть чат «Марат» в оверлее — проверить, что подгрузились все старые сообщения.
- Открыть чат «Astartes» — проверить, что размытые гифки/картинки становятся резкими после фоновой загрузки.
