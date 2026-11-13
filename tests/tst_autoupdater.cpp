#include "TestArchive.hpp"
#include "AutoUpdater.hpp"
#include "ReleaseServer.hpp"
#include "TestFiles.hpp"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include <archive.h>
#include <archive_entry.h>

namespace {
#ifdef Q_OS_WINDOWS
constexpr auto EXECUTABLE = "rpgmtranslate.exe";
#else
constexpr auto EXECUTABLE = "rpgmtranslate";
#endif

// QVERIFY2 evaluates its message even when the check passes, and error() of a success is undefined
auto errorOf(const expected<void, Notice>& result) -> QString {
    return result.has_value() ? QString() : result.error().english();
}
}  // namespace

class TestAutoUpdater final : public QObject {
    Q_OBJECT

    QTemporaryDir dir;

    [[nodiscard]] auto targetPath() const -> QString { return dir.path() + u'/' + QLatin1StringView(EXECUTABLE); }

    void installOldExecutable() const { writeFile(targetPath(), "old"); }

   private slots:
    void init() {
        for (const QString& name : QDir(dir.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
            const QString path = dir.path() + u'/' + name;
            QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
        }
    }

    void theExecutableIsReplacedAndTheDocumentsAreAdded() {
        installOldExecutable();

        const auto archive = makeArchive({
            { EXECUTABLE, "new" },
            { "THIRD-PARTY-NOTICE.md", "notice" },
            { "licenses/zlib/LICENSE", "license" },
        });

        const auto result = AutoUpdater::install(archive, QFileInfo(targetPath()), false);

        QVERIFY2(result.has_value(), qPrintable(errorOf(result)));
        QCOMPARE(readFile(targetPath()), "new"_ba);
        QCOMPARE(readFile(dir.path() + u"/THIRD-PARTY-NOTICE.md"_qsv), "notice"_ba);
        QCOMPARE(readFile(dir.path() + u"/licenses/zlib/LICENSE"_qsv), "license"_ba);
#ifdef Q_OS_WINDOWS
        // The running executable cannot be overwritten, so the old one is moved aside
        QCOMPARE(readFile(dir.path() + u"/rpgmtranslate-old.exe"_qsv), "old"_ba);
#else
        QVERIFY(!QFile::exists(targetPath() + u".new"_qsv));
#endif
    }

// Only Windows moves the old executable aside, so only there can a leftover of it block the next update
#ifdef Q_OS_WINDOWS
    void anOldLeftoverFromAPreviousUpdateDoesNotBlockTheNextOne() {
        installOldExecutable();
        writeFile(dir.path() + u"/rpgmtranslate-old.exe"_qsv, "ancient");

        const auto result = AutoUpdater::install(makeArchive({ { EXECUTABLE, "new" } }), QFileInfo(targetPath()), false);

        QVERIFY2(result.has_value(), qPrintable(errorOf(result)));
        QCOMPARE(readFile(targetPath()), "new"_ba);
    }
#endif

    void onlyTheExecutableTheNoticeAndTheLicensesAreInstalled() {
        installOldExecutable();

        const auto archive = makeArchive({
            { EXECUTABLE, "new" },
            { "other.txt", "stray" },
            { "licenses/../escaped.txt", "climbs" },
            { "docs/THIRD-PARTY-NOTICE.md", "nested" },
        });

        const auto result = AutoUpdater::install(archive, QFileInfo(targetPath()), false);

        QVERIFY2(result.has_value(), qPrintable(errorOf(result)));
        QVERIFY(!QFile::exists(dir.path() + u"/other.txt"_qsv));
        QVERIFY(!QFile::exists(dir.path() + u"/escaped.txt"_qsv));
        QVERIFY(!QFile::exists(dir.path() + u"/../escaped.txt"_qsv));
        QVERIFY(!QFile::exists(dir.path() + u"/docs"_qsv));
    }

    void anArchiveWithoutTheExecutableIsRefusedAndTheOldOneStays() {
        installOldExecutable();

        const auto result =
            AutoUpdater::install(makeArchive({ { "THIRD-PARTY-NOTICE.md", "notice" } }), QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QVERIFY2(result.error().english().startsWith(u"Failed to extract update archive"_qsv), qPrintable(result.error().english()));
        QCOMPARE(readFile(targetPath()), "old"_ba);
        QVERIFY(!QFile::exists(targetPath() + u".new"_qsv));
    }

    void somethingThatIsNotAnArchiveIsRefusedAndTheOldExecutableStays() {
        installOldExecutable();

        const auto result = AutoUpdater::install("definitely not an archive"_ba, QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QVERIFY2(result.error().english().startsWith(u"Failed to open update archive"_qsv), qPrintable(result.error().english()));
        QCOMPARE(readFile(targetPath()), "old"_ba);
        QVERIFY(!QFile::exists(dir.path() + u"/rpgmtranslate-old.exe"_qsv));
    }

    void aTruncatedArchiveNeverLeavesABrokenExecutable() {
        installOldExecutable();

        QByteArray archive = makeArchive({ { EXECUTABLE, QByteArray(100'000, 'x').toStdString() } });
        archive.truncate(archive.size() / 2);

        const auto result = AutoUpdater::install(archive, QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QCOMPARE(readFile(targetPath()), "old"_ba);
    }

    void aCorruptedArchiveNeverLeavesABrokenExecutable() {
        installOldExecutable();

        // Incompressible, so the damaged bytes are payload and not padding
        string payload(100'000, '\0');
        quint32 state = 12345;

        for (char& byte : payload) {
            state = state * 1664525U + 1013904223U;
            byte = static_cast<char>(state >> 24);
        }

        QByteArray archive = makeArchive({ { EXECUTABLE, payload } });
        const qsizetype middle = archive.size() / 2;

        for (qsizetype idx = middle; idx < middle + 64; idx++) {
            archive[idx] = static_cast<char>(archive[idx] ^ 0xFF);
        }

        const auto result = AutoUpdater::install(archive, QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QCOMPARE(readFile(targetPath()), "old"_ba);
        QVERIFY(!QFile::exists(targetPath() + u".new"_qsv));
    }

#ifdef Q_OS_WINDOWS
    void anExecutableThatCannotBeMovedAsideIsReported() {
        // Nothing to rename: the target is not there
        const auto result = AutoUpdater::install(makeArchive({ { EXECUTABLE, "new" } }), QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QVERIFY(result.error().english().startsWith(u"Failed to prepare the update"_qsv));
    }
#endif

#ifndef Q_OS_WINDOWS
    void aTargetThatCannotBeReplacedIsReportedAndLeavesNoNewFile() {
        // A folder that holds something cannot be replaced by a file
        QVERIFY(QDir().mkpath(targetPath()));
        writeFile(targetPath() + u"/inside"_qsv, "x");

        const auto result = AutoUpdater::install(makeArchive({ { EXECUTABLE, "new" } }), QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QVERIFY2(result.error().english().startsWith(u"Failed to install the update"_qsv), qPrintable(result.error().english()));
        QVERIFY(!QFile::exists(targetPath() + u".new"_qsv));
    }

    void anExecutableThatCannotBeWrittenIsReportedAndTheOldOneStays() {
        installOldExecutable();
        // Where the new executable is written stands a folder that holds something: the write is refused at its header
        QVERIFY(QDir().mkpath(targetPath() + u".new"_qsv));
        writeFile(targetPath() + u".new/inside"_qsv, "x");

        const auto result = AutoUpdater::install(makeArchive({ { EXECUTABLE, "new" } }), QFileInfo(targetPath()), false);

        QVERIFY(!result.has_value());
        QVERIFY2(result.error().english().startsWith(u"Failed to extract update archive"_qsv), qPrintable(result.error().english()));
        QCOMPARE(readFile(targetPath()), "old"_ba);
    }
#endif

    void theVersionIsReadFromWhereTheLatestReleaseRedirects() {
        ReleaseServer server;
        AutoUpdater updater;
        QSignalSpy fetched(&updater, &AutoUpdater::versionFetched);

        updater.checkForUpdates();

        QTRY_COMPARE(fetched.size(), 1);
        QCOMPARE(fetched.takeFirst().at(0).toString(), u"9.8.7"_s);
    }

    void aServerThatIsNotThereFailsTheVersionCheck() {
        // Nothing listens on a port that was just released
        QString dead;
        {
            ReleaseServer server;
            dead = server.url();
        }
        AutoUpdater::setReleasesUrl(dead);

        AutoUpdater updater;
        QSignalSpy failed(&updater, &AutoUpdater::updateFailed);

        updater.checkForUpdates();

        QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, DEAD_SERVER_MS);
        QVERIFY(!failed.first().at(0).toString().isEmpty());
        QVERIFY(!failed.first().at(1).toBool());
    }

    void theArchiveIsDownloadedWithItsProgress() {
        ReleaseServer server;
        server.asset = QByteArray(300'000, 'z');
        AutoUpdater updater;
        QSignalSpy downloaded(&updater, &AutoUpdater::updateDownloaded);
        QSignalSpy progress(&updater, &AutoUpdater::updateDownloadProgress);

        updater.downloadUpdate();

        QTRY_COMPARE(downloaded.size(), 1);
        QCOMPARE(downloaded.first().at(0).toByteArray(), server.asset);
        QVERIFY(!progress.isEmpty());
        QVERIFY(server.requests().last().contains(u"/latest/download/rpgmtranslate"_qsv));
    }

    void aMissingAssetFailsTheDownload() {
        ReleaseServer server;
        server.status = 404;
        AutoUpdater updater;
        QSignalSpy failed(&updater, &AutoUpdater::updateFailed);
        QSignalSpy downloaded(&updater, &AutoUpdater::updateDownloaded);

        updater.downloadUpdate();

        QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, DEAD_SERVER_MS);
        QVERIFY(failed.first().at(1).toBool());
        QVERIFY(downloaded.isEmpty());
    }

    void anAbortedDownloadReportsNothing() {
        ReleaseServer server;
        server.asset = QByteArray(300'000, 'z');
        server.hold = true;
        AutoUpdater updater;
        QSignalSpy failed(&updater, &AutoUpdater::updateFailed);
        QSignalSpy downloaded(&updater, &AutoUpdater::updateDownloaded);

        updater.downloadUpdate();
        QTRY_COMPARE(server.downloads, 1);
        updater.abortDownload();
        QTest::qWait(200);

        QVERIFY(failed.isEmpty());
        QVERIFY(downloaded.isEmpty());
        // Once the download is over there is nothing left to abort
        updater.abortDownload();
    }

    void aDownloadCutShortFailsAndDeliversNothing() {
        ReleaseServer server;
        server.drop = true;
        AutoUpdater updater;
        QSignalSpy failed(&updater, &AutoUpdater::updateFailed);
        QSignalSpy downloaded(&updater, &AutoUpdater::updateDownloaded);

        updater.downloadUpdate();

        QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, DEAD_SERVER_MS);
        QVERIFY(failed.first().at(1).toBool());
        QVERIFY(downloaded.isEmpty());
    }

    // Without a reply there is nothing to abort; touching the missing reply would crash
    void abortingWithoutADownloadDoesNothing() {
        AutoUpdater updater;
        QSignalSpy failed(&updater, &AutoUpdater::updateFailed);

        updater.abortDownload();

        QVERIFY(failed.isEmpty());
    }

    void theAppImageIsTakenFromTheEnvironment() {
        qunsetenv("APPIMAGE");
        QVERIFY(AutoUpdater::appImagePath().isEmpty());

        qputenv("APPIMAGE", "/opt/rpgmtranslate.AppImage");
        QCOMPARE(AutoUpdater::appImagePath(), u"/opt/rpgmtranslate.AppImage"_s);
        qunsetenv("APPIMAGE");
    }
};

QTEST_MAIN(TestAutoUpdater)
#include "tst_autoupdater.moc"
