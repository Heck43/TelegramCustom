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

        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || 
            ext == "webp" || ext == "bmp" || ext == "svg" || ext == "heic") {
            return FileCategory::Images;
        }
        if (ext == "mp3" || ext == "flac" || ext == "wav" || ext == "ogg" || 
            ext == "m4a" || ext == "aac" || ext == "opus") {
            return FileCategory::Audio;
        }
        if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov" || 
            ext == "webm" || ext == "wmv" || ext == "flv") {
            return FileCategory::Video;
        }
        if (ext == "pdf" || ext == "docx" || ext == "doc" || ext == "xlsx" || 
            ext == "pptx" || ext == "txt" || ext == "csv" || ext == "epub") {
            return FileCategory::Documents;
        }
        if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || 
            ext == "gz" || ext == "bz2" || ext == "xz") {
            return FileCategory::Archives;
        }
        if (ext == "exe" || ext == "msi" || ext == "bat" || ext == "cmd" || 
            ext == "ps1" || ext == "apk") {
            return FileCategory::Programs;
        }
        return FileCategory::Other;
    }

    static QString GetSuggestedDownloadPath(const QString &fileName, const QString &defaultPath = QString()) {
        if (!GetConfig().enableDownloadsRouter) {
            return defaultPath.isEmpty()
                ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                : defaultPath;
        }

        const auto category = CategorizeFile(fileName);
        QString targetDir;

        switch (category) {
        case FileCategory::Images:
            targetDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/Telegram Pictures";
            break;
        case FileCategory::Audio:
            targetDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/Telegram Music";
            break;
        case FileCategory::Video:
            targetDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/Telegram Video";
            break;
        case FileCategory::Documents:
            targetDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Telegram Documents";
            break;
        case FileCategory::Archives:
        case FileCategory::Programs:
        case FileCategory::Other:
        default:
            targetDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/Telegram Desktop";
            break;
        }

        QDir().mkpath(targetDir);
        return targetDir;
    }
};

} // namespace CustomFeatures
