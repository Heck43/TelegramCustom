# Setup & Build Guide: TelegramCustom

В этом документе описаны процесс сборки клиента, структура CI/CD пайплайна и способы диагностики ошибок.

---

## 1. Сборка через GitHub Actions (Рекомендуемый способ)

Поскольку сборка Telegram Desktop на Windows требует около 50 ГБ дискового пространства и десятков прекомпилированных C++ библиотек (Qt, OpenSSL, FFmpeg, Opus, WebP, Range-v3, rpl и др.), основным и наиболее быстрым методом сборки является **CI/CD в GitHub Actions**.

### Конфигурация пайплайна
- **Файл конфигурации:** `.github/workflows/build_custom_win.yml`
- **Триггеры сборки:**
  1. Любой `push` в ветку `main` или `master`.
  2. Ручной запуск через веб-интерфейс GitHub (`workflow_dispatch`).

### Шаги сборки в GitHub Actions:
1. **Очистка диска:** Удаление неиспользуемых сред (.NET, Ruby, Android SDK) на раннере `windows-latest` для освобождения ~30 ГБ.
2. **Восстановление кэша библиотек:** Восстановление скомпилированных third-party библиотек из кэша Actions (`actions/cache@v4`).
3. **Клонирование tdesktop:** Клонирование официального репозитория `https://github.com/telegramdesktop/tdesktop.git` ветки `v7.0.9`.
4. **Наложение модификаций:** Копирование наших файлов из репозитория поверх чистого tdesktop:
   - Каталог `custom_features/`
   - Настройки `settings_custom.*`
   - Измененные файлы `stickers_list_widget.cpp`, `file_utilities.cpp`, `main_window_win.cpp` и др.
5. **Сборка через MSVC 2022:** Инструментарий v14.44 (x64) с оптимизациями Release.
6. **Выгрузка артефакта:** Публикация готового архива `TelegramCustom-Windows-x64.zip` с бинарником `Telegram.exe`.

---

## 2. Мониторинг сборки и скачивание логов через PowerShell

Для проверки статуса сборки или чтения логов компиляции можно использовать PowerShell:

```powershell
# 1. Проверка последних запусков сборки
$headers = @{ 'User-Agent' = 'TelegramCustomDev' }
$runs = Invoke-RestMethod -Uri "https://api.github.com/repos/Heck43/TelegramCustom/actions/runs?per_page=5" -Headers $headers
$runs.workflow_runs | Select-Object id, name, head_sha, status, conclusion

# 2. Получение ID задач конкретной сборки
$run_id = "<RUN_ID>"
$jobs = Invoke-RestMethod -Uri "https://api.github.com/repos/Heck43/TelegramCustom/actions/runs/$run_id/jobs" -Headers $headers
$jobs.jobs | Select-Object id, name, status, conclusion

# 3. Скачивание логов задачи и поиск ошибок компилятора
$job_id = "<JOB_ID>"
$log = Invoke-RestMethod -Uri "https://api.github.com/repos/Heck43/TelegramCustom/actions/jobs/$job_id/logs" -Headers $headers
$log | Select-String -Pattern "error C|fatal error|: error" -Context 2,2
```

---

## 3. Локальная разработка

Для локального редактирования и проверки синтаксиса:
- **IDE:** Visual Studio 2022 или CLion / VS Code с расширением C/C++.
- **Стандарт C++:** C++20 (`/std:c++20`).
- **Синтаксическая проверка:** Большинство кастомных классов в `Telegram/SourceFiles/custom_features/` выполнены заголовочными (`.hpp`), что позволяет инспектировать их без полной пересборки всего монолита.
