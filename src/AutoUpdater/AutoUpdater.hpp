#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

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
