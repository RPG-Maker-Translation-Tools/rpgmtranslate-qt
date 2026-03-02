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

    [[nodiscard]] auto downloadedData() const -> QByteArray;

   signals:
    void versionFetched(const QString& version);
    void updateDownloaded(const QByteArray& data);
    void updateDownloadProgress(u64 received, u64 total);
    void updateFailed(QNetworkReply::NetworkError);

   private:
    void downloadFile(QNetworkReply* reply);
    void getVersion(QNetworkReply* reply);

    QNetworkAccessManager networkManager;
    QNetworkReply* downloadReply = nullptr;
    bool aborted = false;
};
