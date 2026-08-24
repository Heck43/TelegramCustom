#pragma once

#include <QString>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include "custom_features/custom_settings.hpp"

namespace CustomFeatures {

class DownloadsRouter {
public:
    enum class FileCategory {
        Images,
        Audio,
        Video,
        Documents,
        Archives,
        Programs,
        Other
    };

    static FileCategory CategorizeFile(const QString &fileName) {
        const auto ext = QFileInfo(fileName).suffix().toLower();

        // 1. Изображения
        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || 
            ext == "webp" || ext == "bmp" || ext == "svg" || ext == "heic" ||
            ext == "psd" || ext == "ico" || ext == "tiff" || ext == "ai") {
            return FileCategory::Images;
        }

        // 2. Аудио и музыка
        if (ext == "mp3" || ext == "flac" || ext == "wav" || ext == "ogg" || 
            ext == "m4a" || ext == "aac" || ext == "opus" || ext == "mid" ||
            ext == "wma" || ext == "alac") {
            return FileCategory::Audio;
        }

        // 3. Видео
        if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov" || 
            ext == "webm" || ext == "wmv" || ext == "flv" || ext == "m4v" ||
            ext == "3gp") {
            return FileCategory::Video;
        }

        // 4. Документы, книги и логи
        if (ext == "pdf" || ext == "docx" || ext == "doc" || ext == "xlsx" || 
            ext == "xls" || ext == "pptx" || ext == "ppt" || ext == "txt" || 
            ext == "log" || ext == "html" || ext == "htm" || ext == "csv" || 
            ext == "epub" || ext == "fb2" || ext == "json" || ext == "xml" || 
            ext == "rtf" || ext == "md" || ext == "odt" || ext == "ods") {
            return FileCategory::Documents;
        }

        // 5. Архивы
        if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || 
            ext == "gz" || ext == "bz2" || ext == "xz" || ext == "tgz" ||
            ext == "zst" || ext == "cab") {
            return FileCategory::Archives;
        }

        // 6. Программы и исполняемые файлы
        if (ext == "exe" || ext == "msi" || ext == "dll" || ext == "bat" || 
            ext == "cmd" || ext == "ps1" || ext == "apk" || ext == "vbs" || 
            ext == "jar" || ext == "msix" || ext == "appx") {
            return FileCategory::Programs;
        }

        return FileCategory::Other;
    }

    static QString GetSuggestedDownloadPath(const QString &fileName, const QString &defaultPath = QString()) {
        const auto downloadsBase = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/Telegram Desktop";

        if (!GetConfig().enableDownloadsRouter) {
            return defaultPath.isEmpty() ? downloadsBase : defaultPath;
        }

        const auto category = CategorizeFile(fileName);
        QString targetDir;

        switch (category) {
        case FileCategory::Images:
            targetDir = downloadsBase + "/Изображения";
            break;
        case FileCategory::Audio:
            targetDir = downloadsBase + "/Музыка";
            break;
        case FileCategory::Video:
            targetDir = downloadsBase + "/Видео";
            break;
        case FileCategory::Documents:
            targetDir = downloadsBase + "/Документы";
            break;
        case FileCategory::Archives:
            targetDir = downloadsBase + "/Архивы";
            break;
        case FileCategory::Programs:
            targetDir = downloadsBase + "/Программы";
            break;
        case FileCategory::Other:
        default:
            targetDir = downloadsBase + "/Разное";
            break;
        }

        QDir().mkpath(targetDir);
        return targetDir;
    }
};

} // namespace CustomFeatures
