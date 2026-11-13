#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Notice.hpp"

#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

class AutoUpdater final : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;

    void checkForUpdates();
    void downloadUpdate();
    void abortDownload();

    // Path of the running AppImage file, empty when not running from one. An AppImage is updated by replacing that
    // file (its contents are a read-only mount), and has its own release asset.
    [[nodiscard]] static auto appImagePath() -> QString;

    // Replaces the releases page the version is read from and the update is downloaded from. Only the tests use it, to
    // point the updater at a local server.
    static void setReleasesUrl(const QString& url);

    // Replaces `target` with the executable in `archive` (and adds the notice and licenses beside it). Whatever fails
    // leaves the old executable in place, and the error is the notice to show.
    [[nodiscard]] static auto install(const QByteArray& archive, const QFileInfo& target, bool appImage)
        -> expected<void, Notice>;

   signals:
    void versionFetched(const QString& version);
    void updateDownloaded(const QByteArray& data);
    void updateDownloadProgress(u64 received, u64 total);
    // download: the failure is that of the download, not of the version check
    void updateFailed(const QString& error, bool download);

   private:
    QNetworkAccessManager networkManager;
    QNetworkReply* downloadReply = nullptr;
    bool aborted = false;
};
