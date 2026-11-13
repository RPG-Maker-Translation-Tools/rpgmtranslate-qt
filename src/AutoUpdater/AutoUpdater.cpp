#include "AutoUpdater.hpp"

#include "QtAliases.hpp"
#include "StringBuilder.hpp"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QNetworkRequest>

#include <archive.h>
#include <archive_entry.h>

namespace {
QString releasesUrl = u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases"_s;
}  // namespace

void AutoUpdater::setReleasesUrl(const QString& url) {
    releasesUrl = url;
}

void AutoUpdater::checkForUpdates() {
    connect(&networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply* const reply) -> void {
        if (reply->error() != QNetworkReply::NoError) {
            emit updateFailed(reply->errorString(), false);
        } else {
            const QString url = reply->url().toString();
            emit versionFetched(url.sliced(url.lastIndexOf(u'/') + 2));
        }

        reply->deleteLater();
    }, Qt::SingleShotConnection);

    // Only the address it redirects to is of interest, not the page
    const QString url = releasesUrl % u"/latest"_qsv;
    networkManager.head(QNetworkRequest(QUrl(url)));
};

void AutoUpdater::downloadUpdate() {
    connect(&networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply* const reply) -> void {
        if (!aborted) {
            if (reply->error() != QNetworkReply::NoError) {
                emit updateFailed(reply->errorString(), true);
            } else {
                emit updateDownloaded(reply->readAll());
            }
        }

        reply->deleteLater();
        downloadReply = nullptr;
    }, Qt::SingleShotConnection);

#ifdef Q_OS_WINDOWS
    const QString asset = u"rpgmtranslate.7z"_s;
#elifdef Q_OS_LINUX
    const QString asset = appImagePath().isEmpty() ? u"rpgmtranslate.tar.xz"_s : u"rpgmtranslate-appimage.tar.xz"_s;
#elifdef Q_OS_MACOS
    const QString asset = u"rpgmtranslate.zip"_s;
#endif

    const QString url = releasesUrl % u"/latest/download/"_qsv % asset;
    downloadReply = networkManager.get(QNetworkRequest(QUrl(url)));

    connect(
        downloadReply,
        &QNetworkReply::downloadProgress,
        this,
        [=, this](const u64 received, const u64 total) -> void { emit updateDownloadProgress(received, total); }
    );
}

void AutoUpdater::abortDownload() {
    // The finished handler nulls downloadReply, so there is nothing to abort once the download is over
    if (downloadReply == nullptr) {
        return;
    }

    aborted = true;
    downloadReply->abort();
}

auto AutoUpdater::appImagePath() -> QString {
    return qEnvironmentVariable("APPIMAGE");
}

namespace {
// libarchive gives no message for some failures
auto archiveError(archive* const handle) -> QString {
    const char* const message = archive_error_string(handle);
    return message == nullptr ? QString() : QString::fromUtf8(message);
}
}  // namespace

auto AutoUpdater::install(const QByteArray& archiveData, const QFileInfo& installTarget, [[maybe_unused]] const bool appImage)
    -> expected<void, Notice> {
    const QString appDir = installTarget.absolutePath();

#ifdef Q_OS_WINDOWS
    const QString exePath = installTarget.absoluteFilePath();
    const QString oldExePath = appDir + u"/rpgmtranslate-old.exe";

    // Leftover from a previous update; QFile::rename refuses to overwrite it
    QFile::remove(oldExePath);

    if (!QFile::rename(exePath, oldExePath)) {
        return Err(Notice(
            NOTICE_IN("MainWindow", "Failed to prepare the update: could not rename %1."),
            Severity::Warning,
            Delivery::Modal,
            { exePath }
        ));
    }

    const auto restoreExe = [&exePath, &oldExePath] -> void {
        // Drop whatever was partially extracted, rename can't overwrite
        QFile::remove(exePath);
        QFile::rename(oldExePath, exePath);
    };
#else
    // What was being extracted is the only thing to undo: the executable itself is replaced by a rename at the end
    const auto restoreExe = [&installTarget] -> void { QFile::remove(installTarget.absoluteFilePath() + u".new"); };
#endif

    archive* const archive_ = archive_read_new();
    archive* const disk = archive_write_disk_new();

    const auto cleanup = [archive_, disk] -> void {
        archive_read_close(archive_);
        archive_read_free(archive_);
        archive_write_close(disk);
        archive_write_free(disk);
    };

#ifdef Q_OS_WINDOWS
    archive_read_support_format_7zip(archive_);
    archive_read_support_filter_none(archive_);
#elifdef Q_OS_LINUX
    archive_read_support_format_tar(archive_);
    archive_read_support_filter_xz(archive_);
    archive_read_support_filter_lzma(archive_);
#elifdef Q_OS_MACOS
    archive_read_support_format_zip(archive_);
#endif

    // UNLINK: an existing license file is replaced instead of being written into
    archive_write_disk_set_options(
        disk,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_UNLINK
    );

    if (archive_read_open_memory(archive_, archiveData.constData(), static_cast<usize>(archiveData.size())) !=
        ARCHIVE_OK) {
        qWarning().noquote() << u"libarchive failed to open archive: %1"_qsv.arg(
            archiveError(archive_)
        );

        cleanup();
        restoreExe();
        return Err(Notice(
            NOTICE_IN("MainWindow", "Failed to open update archive."),
            Severity::Warning,
            Delivery::Modal
        ));
    }

#ifdef Q_OS_WINDOWS
    static constexpr string_view targetEntry = "rpgmtranslate.exe";
#else
    const string_view targetEntry = appImage ? "RPGMTranslate-x86_64.AppImage" : "rpgmtranslate";
#endif
#ifdef Q_OS_WINDOWS
    // The running executable was renamed away above, so its path is free
    const QByteArray outputPath = installTarget.absoluteFilePath().toUtf8();
#else
    // Written next to the target and renamed over it afterwards, so a failed extraction never leaves a broken
    // executable behind
    const QByteArray outputPath = installTarget.absoluteFilePath().toUtf8() + ".new";
#endif
    const QByteArray appDirUtf8 = appDir.toUtf8();

    // Writes the entry the reader is positioned on to the path already set on it
    const auto extractEntry = [archive_, disk](archive_entry* const entry) -> bool {
        if (archive_write_header(disk, entry) != ARCHIVE_OK) {
            qWarning().noquote() << u"libarchive write_header failed: %1"_qsv.arg(
                archiveError(disk)
            );
            return false;
        }

        const void* buffer;
        usize size;
        i64 offset;
        bool success = true;

        while (true) {
            const i32 read = archive_read_data_block(archive_, &buffer, &size, &offset);

            if (read == ARCHIVE_EOF) {
                break;
            }

            if (read != ARCHIVE_OK) {
                qWarning().noquote() << u"libarchive read_data_block failed: %1"_qsv.arg(
                    archiveError(archive_)
                );
                success = false;
                break;
            }

            if (archive_write_data_block(disk, buffer, size, offset) != ARCHIVE_OK) {
                qWarning().noquote() << u"libarchive write_data_block failed: %1"_qsv.arg(
                    archiveError(disk)
                );
                success = false;
                break;
            }
        }

        archive_write_finish_entry(disk);
        return success;
    };

    bool extracted = false;
    archive_entry* entry;

    while (archive_read_next_header(archive_, &entry) == ARCHIVE_OK) {
        const char* const rawName = archive_entry_pathname_utf8(entry);
        const u32 type = archive_entry_filetype(entry);

        // Only the executable, notice and licenses are installed. Names are joined to the install directory by
        // hand, so an entry that could climb out of it, or is not a plain file or directory, is never written.
        // A link entry would be installed as a link to anywhere
        const bool isPlain = archive_entry_symlink(entry) == nullptr && archive_entry_hardlink(entry) == nullptr;
        const bool isExecutable =
            rawName != nullptr && isPlain && type == AE_IFREG && string_view(rawName) == targetEntry;
        const bool isDocument =
            rawName != nullptr && isPlain && !string_view(rawName).contains("..") &&
            (type == AE_IFREG || type == AE_IFDIR) &&
            (string_view(rawName) == "THIRD-PARTY-NOTICE.md" || string_view(rawName).starts_with("licenses/"));

        if (!isExecutable && !isDocument) {
            archive_read_data_skip(archive_);
            continue;
        }

        if (isExecutable) {
            archive_entry_set_pathname_utf8(entry, outputPath.constData());
            extracted = extractEntry(entry);

            if (!extracted) {
                break;
            }
        } else {
            // The licenses are not worth failing an update that already installed the executable
            const QByteArray documentPath = appDirUtf8 + '/' + rawName;
            archive_entry_set_pathname_utf8(entry, documentPath.constData());
            extractEntry(entry);
        }
    }

    cleanup();

    if (!extracted) {
        restoreExe();
        return Err(Notice(
            NOTICE_IN("MainWindow", "Failed to extract update archive."),
            Severity::Warning,
            Delivery::Modal
        ));
    }

#ifndef _WIN32
    const QString extractedPath = QString::fromUtf8(outputPath);

    QFile::setPermissions(
        extractedPath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup |
            QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther
    );

    // rename(2) replaces the target atomically, also while it is running
    error_code renameError;
    fs::rename(extractedPath.toStdString(), installTarget.absoluteFilePath().toStdString(), renameError);

    if (renameError) {
        QFile::remove(extractedPath);
        return Err(Notice(
            NOTICE_IN("MainWindow", "Failed to install the update: %1"),
            Severity::Warning,
            Delivery::Modal,
            { QString::fromStdString(renameError.message()) }
        ));
    }
#endif

    return {};
}
