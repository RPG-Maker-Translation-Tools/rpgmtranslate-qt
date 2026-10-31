#pragma once

#include <QFile>
#include <QtTest>

// Small file helpers for tests that work in a `QTemporaryDir`
inline void writeFile(const QString& path, const QByteArray& content) {
    QFile file(path);

    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
    QCOMPARE(file.write(content), static_cast<qint64>(content.size()));
}

[[nodiscard]] inline auto readFile(const QString& path) -> QByteArray {
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return file.readAll();
}
