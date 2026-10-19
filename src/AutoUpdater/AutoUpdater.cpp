#include "AutoUpdater.hpp"

#include <QByteArray>
#include <QNetworkRequest>

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
    networkManager.head(
        QNetworkRequest(QUrl(u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases/latest"_s))
    );
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

    const QString url =
        u"https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases/latest/download/"_qsv % asset;
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
