#pragma once

#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>
#include <functional>

// A server on the loopback that answers each request with whatever `handler` returns: a complete response, head and
// body, or nothing to leave the request hanging. `requests` keeps the request lines it was asked for.
class HttpStub final : public QObject {
   public:
    using Handler = std::function<QByteArray(const QString& method, const QString& path, const QByteArray& body)>;

    explicit HttpStub(Handler handler) : handler(std::move(handler)) {
        QVERIFY2(server.listen(QHostAddress::LocalHost), "stub server did not start");

        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket* const socket = server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] { answer(socket); });
            }
        });
    }

    [[nodiscard]] auto url() const -> QString {
        return QStringLiteral("http://127.0.0.1:") + QString::number(server.serverPort());
    }

    // A response with `status` ("200 OK") and `body`
    [[nodiscard]] static auto response(const QByteArray& status, const QByteArray& body) -> QByteArray {
        return "HTTP/1.1 " + status + "\r\nContent-Type: application/json\r\nContent-Length: " +
               QByteArray::number(body.size()) + "\r\n\r\n" + body;
    }

    QStringList requests;

   private:
    void answer(QTcpSocket* const socket) {
        QByteArray& buffer = pending[socket];
        buffer += socket->readAll();

        const qsizetype split = buffer.indexOf("\r\n\r\n");

        if (split < 0) {
            return;
        }

        const QByteArray head = buffer.left(split);
        qsizetype length = 0;

        for (const QByteArray& line : head.split('\n')) {
            if (line.toLower().startsWith("content-length:")) {
                length = line.mid(15).trimmed().toLongLong();
            }
        }

        // A request whose body is still on its way is answered once the rest is here
        if (buffer.size() < split + 4 + length) {
            return;
        }

        const QList<QByteArray> line = head.left(head.indexOf("\r\n")).split(' ');

        if (line.size() < 2) {
            return;
        }

        const QString method = QString::fromLatin1(line.at(0));
        const QString path = QString::fromLatin1(line.at(1));
        const QByteArray body = buffer.mid(split + 4, length);
        buffer.clear();
        requests.append(method + u' ' + path);

        const QByteArray reply = handler(method, path, body);

        if (!reply.isEmpty()) {
            socket->write(reply);
            socket->flush();

            if (closeAfterReply) {
                socket->disconnectFromHost();
            }
        }
    }

   public:
    // Hang up once the reply is written, so a reply that promises more than it holds ends in a dropped connection
    bool closeAfterReply = false;

   private:
    QTcpServer server;
    Handler handler;
    QHash<QTcpSocket*, QByteArray> pending;
};
