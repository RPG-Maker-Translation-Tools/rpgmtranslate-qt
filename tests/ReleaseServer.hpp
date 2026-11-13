#pragma once

#include "AutoUpdater.hpp"
#include "HttpStub.hpp"

// Windows (and wine) gives up on a connection to a port nobody listens on only after about a minute
#ifdef Q_OS_WINDOWS
inline constexpr int DEAD_SERVER_MS = 90000;
#else
inline constexpr int DEAD_SERVER_MS = 10000;
#endif

// A local stand-in for the releases page, which the updater is pointed at for as long as it lives. `/latest` redirects
// to a tag page, anything below `/latest/download/` answers with `asset`.
class ReleaseServer final {
   public:
    ReleaseServer()
        : stub([this](const QString& /* method */, const QString& path, const QByteArray& /* body */) -> QByteArray {
              if (path == QStringLiteral("/latest")) {
                  return "HTTP/1.1 302 Found\r\nLocation: /tag/v" + version.toLatin1() +
                         "\r\nContent-Length: 0\r\n\r\n";
              }

              if (path.startsWith(QStringLiteral("/latest/download/"))) {
                  downloads++;

                  stub.closeAfterReply = drop;

                if (hold || drop) {
                      // The head promises more than is ever sent
                      return "HTTP/1.1 200 OK\r\nContent-Length: 1000000\r\n\r\npartial";
                  }

                  return status == 200 ? HttpStub::response("200 OK", asset)
                                       : HttpStub::response("404 Not Found", QByteArray());
              }

              return HttpStub::response("200 OK", QByteArray());
          }) {
        AutoUpdater::setReleasesUrl(url());
    }

    ~ReleaseServer() {
        AutoUpdater::setReleasesUrl(
            QStringLiteral("https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt/releases")
        );
    }

    ReleaseServer(const ReleaseServer&) = delete;
    auto operator=(const ReleaseServer&) -> ReleaseServer& = delete;

    [[nodiscard]] auto url() const -> QString { return stub.url(); }

    [[nodiscard]] auto requests() const -> QStringList { return stub.requests; }

    QByteArray asset = "archive";
    // The version the latest release is tagged with
    QString version = QStringLiteral("9.8.7");
    int status = 200;
    int downloads = 0;
    // Accepts the download request and never finishes it
    bool hold = false;
    // Like `hold`, but the connection is closed after what was sent: the download is cut short
    bool drop = false;

   private:
    HttpStub stub;
};
