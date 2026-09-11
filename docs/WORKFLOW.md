# Workflows: TelegramCustom

В этом документе пошагово описаны сценарии выполнения ключевых пользовательских и системных процессов в модификации.

---

## 1. Внутриигровой оверлей: вызов, загрузка истории и общение

```text
+---------------+       +---------------------+       +----------------------+
| Пользователь  | ----> | LL Keyboard Hook    | ----> | InGameOverlayManager |
| в игре нажал  |       | (WH_KEYBOARD_LL)    |       | updateState()        |
| Shift + ~     |       +---------------------+       +----------------------+
+---------------+                                                |
                                                                 v
                                                      [ Проверка активного окна ]
                                                      (Игнорировать desktop/dwm)
                                                                 |
                                                                 v
+-----------------------+     +-------------------+     +----------------------+
| Окно оверлея          | <-- | Вызов сессии TG:  | <-- | InGameOverlayWidget  |
| отображается на экране|     | requestDialogs()  |     | toggleVisibility()   |
+-----------------------+     +-------------------+     +----------------------+
```

### Пошаговый сценарий:
1. Пользователь находится в полноэкранной или оконной 3D-игре (например, `cs2.exe` или `javaw.exe`) и нажимает сочетание `Shift + ~`.
2. Функция-ловушка `InGameOverlayManager::HookProc` перехватывает нажатие до того, как игра обработает его.
3. Проверяется имя процесса переднего плана через `GetForegroundWindow` -> `GetWindowThreadProcessId` -> `QueryFullProcessImageNameW`.
4. Если процесс не входит в системный блэклист (`explorer.exe`, `dwm.exe` и др.), вызывается `InGameOverlayWidget::toggleVisibility()`.
5. Оверлей запрашивает актуальный список диалогов через `session->api().requestDialogs()`.
6. Вызывается `reloadRealData()`, очищающий и заново заполняющий левую колонку диалогов. Для каждого диалога загружается аватарка собеседника через `GeneratePeerAvatarPixmap()`.
7. Пользователь кликает на нужный диалог:
   - Шапка оверлея обновляется: отображается имя, круглая аватарка и реальный статус онлайна собеседника (`FormatPeerStatusText` -> `Data::OnlineText`).
   - Если в оперативной памяти уже есть сообщения, они мгновенно отрисовываются через `renderActiveMessages(true)`.
   - Параллельно вызывается `loadHistorySlice(history)`, отправляющий серверный запрос `MTPmessages_GetHistory` для подгрузки предыдущих 50 сообщений.
   - Сервер возвращает сообщения -> они парсятся и добавляются в `history->addOlderSlice(*histList)`.
   - Вызывается `renderActiveMessages(true)`, сообщения отрисовываются с автоматической прокруткой в самый низ.
8. Пользователь вводит текст в поле `_msgInput` и нажимает `Enter`.
9. `sendCurrentMessage()` отправляет сообщение через MTProto API: `session->api().sendMessage(Api::SendAction(_activeHistory), text)`.
10. Новое сообщение мгновенно отображается в виде синего облачка справа с галочками `✓✓` и текущим временем.

---

## 2. Загрузка и динамическое обновление медиа-миниатюр в оверлее

```text
[ Рендеринг сообщения с фото/GIF ]
                 |
                 v
   GenerateMediaThumbnailPixmap()
                 |
        (Вызов goodThumbnailWanted, thumbnailWanted, videoThumbnailWanted)
                 |
      +----------+----------+
      |                     |
      v                     v
[ HD-файл в кэше ]     [ HD-файл не скачан ]
      |                     |
      v                     v
Рендер четкого         Рендер сглаженного
QPixmap (DPR 2.0)      микро-эскиза (inline)
                            |
                     (Фоновый загрузчик MTProto скачал файл)
                            v
                     session->downloaderTaskFinished()
                            v
                     _mediaUpdateTimer.start(150)
                            v
                     renderActiveMessages(false)
                            v
                     Мгновенная плавная замена эскиза на четкий HD-кадр
```

### Пошаговый сценарий:
1. При переборе сообщений в `renderActiveMessages` для каждого элемента с медиа вызывается `GenerateMediaThumbnailPixmap(item, 260, 180)`.
2. Функция проверяет тип вложения (`PhotoData` или `DocumentData` / GIF / видео).
3. Создается медиа-вьюер (`createMediaView()`) и отправляются сигналы заинтересованности во всех размерах:
   - Для фото: `photoMedia->wanted(Thumbnail/Large/Small)`.
   - Для GIF/видео: `docMedia->goodThumbnailWanted()`, `thumbnailWanted()`, `videoThumbnailWanted()`.
4. Если файл еще не скачан Telegram, берётся встроенный в сообщение MTProto микро-эскиз (`thumbnailInline`). Чтобы он не выглядел пиксельным, он сглаживается билинейным фильтром.
5. Как только клиент Telegram завершает загрузку полноразмерного превью в фоне, срабатывает событие `session->downloaderTaskFinished()`.
6. Срабатывает таймер `_mediaUpdateTimer` (150 мс), который вызывает `renderActiveMessages(false)`.
7. `renderActiveMessages(false)` заново формирует облачка без сброса позиции скролла — размытые миниатюры бесшовно заменяются на четкие HD-изображения прямо на глазах у пользователя.

---

## 3. Умный роутер загрузок (Downloads Router)

```text
Пользователь кликает "Сохранить файл"
                 |
                 v
DownloadsRouter::GetSuggestedDownloadPath(fileName)
                 |
                 v
DownloadsRouter::CategorizeFile(extension)
                 |
                 +---> .png/.jpg  -> Downloads/Telegram Desktop/Изображения
                 +---> .mp3/.ogg  -> Downloads/Telegram Desktop/Музыка
                 +---> .mp4/.mkv  -> Downloads/Telegram Desktop/Видео
                 +---> .pdf/.docx -> Downloads/Telegram Desktop/Документы
                 +---> .zip/.rar  -> Downloads/Telegram Desktop/Архивы
                 +---> .exe/.msi  -> Downloads/Telegram Desktop/Программы
                 |
                 v
QDir().mkpath(targetDir) -> Создание папки, если не существует
                 |
                 v
Файл сохраняется в специализированную категорию
```

---

## 4. Динамическая смена акцента (Wallpaper Engine / Windows)

```text
Фоновый таймер темы (каждые 1500 мс)
                 |
                 v
Чтение цвета: DwmGetColorizationColor / Registry AccentColor
                 |
                 v
Цвет отличается от текущего цвета темы Telegram?
  ├── ДА  ---> Вызов ComputePaletteWithAccent(color)
  |            Генерация новой таблицы стилей Qt
  |            Мгновенная перерисовка всех окон без перезапуска клиента
  └── НЕТ ---> Ожидание следующего тика таймера
```

---

## 5. Автоблокировка по Win + L

```text
Пользователь нажимает Win + L на клавиатуре
                 |
                 v
Windows блокирует рабочую станцию и рассылает WM_WTSSESSION_CHANGE
                 |
                 v
main_window_win.cpp ловит wParam == WTS_SESSION_LOCK
                 |
                 v
Проверка: включена ли опция autoLockOnWindowsLock в ClientConfig
  ├── ДА  ---> Core::App().domain().local().lockByPasscode()
  |            Telegram блокируется (требуется ввод локального пароля)
  └── НЕТ ---> Игнорирование события
```
