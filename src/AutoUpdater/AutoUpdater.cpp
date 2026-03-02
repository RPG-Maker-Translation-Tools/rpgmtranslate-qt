#include "AutoUpdater.hpp"

#include <QByteArray>
#include <QNetworkRequest>

void AutoUpdater::checkForUpdates() {
    connect(
        &networkManager,
        &QNetworkAccessManager::finished,
        this,
        [this](QNetworkReply* const reply) -> void {
        if (reply->error() != QNetworkReply::NoError) {
            emit updateFailed(reply->error());
        } else {
            const QString url = reply->url().toString();
            emit versionFetched(url.sliced(url.lastIndexOf('/') + 2));
        }

        reply->deleteLater();
    },
        Qt::SingleShotConnection
    );

    networkManager.get(QNetworkRequest(QUrl(
        u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases/latest"_s
    )));
};

void AutoUpdater::downloadUpdate() {
    connect(
        &networkManager,
        &QNetworkAccessManager::finished,
        this,
        [this](QNetworkReply* const reply) -> void {
        if (!aborted) {
            if (reply->error() != QNetworkReply::NoError) {
                emit updateFailed(reply->error());
            } else {
                emit updateDownloaded(reply->readAll());
            }
        }

        reply->deleteLater();
        downloadReply = nullptr;
    },
        Qt::SingleShotConnection
    );

    downloadReply = networkManager.get(QNetworkRequest(QUrl(
#ifdef Q_OS_WINDOWS
        u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases/latest/download/rpgmtranslate.7z"_s
#elifdef Q_OS_LINUX
        u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases/latest/download/rpgmtranslate.tar.xz"_s
#else
        // TODO
        u""_s
#endif
    )));

    connect(
        downloadReply,
        &QNetworkReply::downloadProgress,
        this,
        [=, this](const u64 received, const u64 total) -> void {
        emit updateDownloadProgress(received, total);
    }
    );
}

void AutoUpdater::abortDownload() {
    aborted = true;
    downloadReply->abort();
}
