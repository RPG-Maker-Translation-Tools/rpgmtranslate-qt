// The window's side of updating: when it asks the release page for the latest version, what it does with the answer,
// and what it tells the user when something goes wrong. A release server on the loopback stands in for GitHub.
#include "Permissions.hpp"
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"
#include "ReleaseServer.hpp"
#include "TestArchive.hpp"

#include <QProgressDialog>
#include <QStatusBar>
#include <QtTest>

namespace {
// The address of a server that is gone
[[nodiscard]] auto deadUrl() -> QString {
    const HttpStub stub([](const QString&, const QString&, const QByteArray&) -> QByteArray { return {}; });
    return stub.url();
}

[[nodiscard]] auto checkAction(const MainWindow& window) -> QAction* {
    return window.findChild<QAction*>(u"actionCheckForUpdates"_s);
}

// A slot of the window runs from the event loop of the application, and a test body does not. What such a slot asks to
// have deleted later is then deleted by the first message box it opens, under the very code that asked for it, so the
// bodies that show a box after a network answer run inside a loop of their own.
void inEventLoop(const std::function<void()>& body) {
    QEventLoop loop;
    QTimer::singleShot(0, &loop, [&body, &loop] -> void {
        body();
        loop.quit();
    });
    loop.exec();
}

// The check that starts with the window
void writeCheckingSettings(const Env& env) {
    env.writeSettings([](Settings& settings) -> void { settings.core.checkForAppUpdates = true; });
}
}  // namespace

class TestMainWindowUpdates final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();
        catcher.reset();
        qunsetenv("APPIMAGE");
    }

    void theStartupCheckIsSkippedWhenTheUserTurnedItOff() {
        ReleaseServer server;
        const Env env;
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QTest::qWait(300);

        QVERIFY(server.requests().isEmpty());
    }

    void theStartupCheckStaysSilentWhenTheProgramIsUpToDate() {
        ReleaseServer server;
        server.version = u"0.0.1"_s;
        const Env env;
        writeCheckingSettings(env);
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY(!server.requests().isEmpty());
        QTest::qWait(200);
        QVERIFY(catcher->seen.isEmpty());
        QVERIFY(!window.statusBar()->currentMessage().contains(u"up-to-date"_qsv));
    }

    void anExplicitCheckSaysTheProgramIsUpToDate() {
        ReleaseServer server;
        server.version = u"0.0.1"_s;
        const Env env;
        env.writeSettings();
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);
        checkAction(window)->trigger();

        QTRY_VERIFY(window.statusBar()->currentMessage().contains(u"up-to-date"_qsv));
        QVERIFY(catcher->seen.isEmpty());
    }

    void anExplicitCheckWorksWhenTheStartupCheckIsOff() {
        ReleaseServer server;
        server.version = u"0.0.1"_s;
        const Env env;
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        checkAction(window)->trigger();

        QTRY_VERIFY(!server.requests().isEmpty());
        QVERIFY(server.requests().first().startsWith(u"HEAD /latest"_qsv));
    }

    void aNewerVersionIsOfferedAndClosingTheOfferDownloadsNothing() {
        ReleaseServer server;
        const Env env;
        writeCheckingSettings(env);
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Version 9.8.7 is available"_qsv), 10000);
        QCOMPARE(catcher->title, u"New version is available"_s);
        QTest::qWait(200);
        QCOMPARE(server.downloads, 0);
    }

    void skippingAVersionWithTheBoxTickedStopsTheChecks() {
        ReleaseServer server;
        const Env env;
        writeCheckingSettings(env);
        catcher = make_unique<ModalCatcher>();
        catcher->press = u"Skip"_s;
        catcher->tickBox = true;

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"is available"_qsv), 10000);
        QTest::qWait(200);
        QCOMPARE(server.downloads, 0);

        // Saved with the settings when the project is closed
        closeProject(window);
        QVERIFY(readFile(env.settingsPath()).contains("\"checkForAppUpdates\":false"));
    }

    void skippingAVersionWithoutTheBoxKeepsChecking() {
        ReleaseServer server;
        const Env env;
        writeCheckingSettings(env);
        catcher = make_unique<ModalCatcher>();
        catcher->press = u"Skip"_s;

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"is available"_qsv), 10000);
        closeProject(window);
        QVERIFY(readFile(env.settingsPath()).contains("\"checkForAppUpdates\":true"));
    }

    void aMissingArchiveIsReportedAsAFailedDownload() {
        inEventLoop([this] -> void {
            ReleaseServer server;
            server.status = 404;
            const Env env;
            writeCheckingSettings(env);
            catcher = make_unique<ModalCatcher>();
            catcher->press = u"Install"_s;

            const MainWindow window;
            const WindowGuard guard(window);

            QTRY_VERIFY_WITH_TIMEOUT(catcher->seen.size() >= 2, 10000);
            QVERIFY(catcher->seen.last().contains(u"Update failed with error"_qsv));
            QCOMPARE(server.downloads, 1);
        });
    }

    void aCancelledDownloadIsNotReported() {
        ReleaseServer server;
        server.asset = QByteArray(300'000, 'z');
        server.hold = true;
        const Env env;
        writeCheckingSettings(env);
        catcher = make_unique<ModalCatcher>();
        catcher->press = u"Install"_s;

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY_WITH_TIMEOUT(server.downloads == 1, 10000);
        auto* const progress = window.findChild<QProgressDialog*>();
        QVERIFY(progress != nullptr);

        progress->cancel();
        QTest::qWait(300);

        // Only the offer was shown
        QCOMPARE(catcher->seen.size(), qsizetype{ 1 });
    }

    void anArchiveThatCannotBeInstalledIsReportedAndTheOldProgramStays() {
        inEventLoop([this] -> void {
            // The program updates the file it runs from, which the tests point at a stand-in
            const QTemporaryDir installDir;
            const QString target = installDir.path() + u"/RPGMTranslate.AppImage"_s;
            writeFile(target, "old program");
            qputenv("APPIMAGE", target.toUtf8());

            ReleaseServer server;
            server.asset = "not an archive";
            const Env env;
            writeCheckingSettings(env);
            catcher = make_unique<ModalCatcher>();
            catcher->press = u"Install"_s;

            const MainWindow window;
            const WindowGuard guard(window);

            QTRY_VERIFY_WITH_TIMEOUT(catcher->seen.size() >= 2, 15000);
            QVERIFY2(
                catcher->seen.last().contains(u"Failed to open update archive"_qsv),
                qPrintable(catcher->seen.join(u'|'))
            );
            QCOMPARE(readFile(target), "old program"_ba);
        });
    }

    // After the update is installed the application quits, and the new program is started only once it has: the old
    // instance has to give up its single-instance lock first
    void anInstalledUpdateIsStartedAfterTheApplicationQuits() {
        inEventLoop([this] -> void {
            const QTemporaryDir installDir;
            const QString target = installDir.path() + u"/RPGMTranslate.AppImage"_s;
            writeFile(target, "old program");
            qputenv("APPIMAGE", target.toUtf8());

            ReleaseServer server;
#ifdef Q_OS_WINDOWS
            server.asset = makeArchive({ { "rpgmtranslate.exe", "new program" } });
#else
            server.asset = makeArchive({ { "RPGMTranslate-x86_64.AppImage", "new program" } });
#endif
            const Env env;
            writeCheckingSettings(env);
            catcher = make_unique<ModalCatcher>();
            catcher->press = u"Install"_s;

            QStringList started;
            MainWindow window;
            const WindowGuard guard(window);
            window.restartLauncher = [&started](const QString& path) -> void {
                started.append(path);
            };

            QTRY_VERIFY_WITH_TIMEOUT(readFile(target) == "new program"_ba, 15000);
            QVERIFY(started.isEmpty());

            QMetaObject::invokeMethod(qApp, "aboutToQuit");

            QCOMPARE(started, QStringList{ QFileInfo(target).absoluteFilePath() });
        });
    }

    // The program quits to install the update, so what it would save on exit is saved first: a tab that cannot be
    // saved stops the update before anything is installed
    void anUpdateIsNotInstalledWhileTheOpenTabCannotBeSaved() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        inEventLoop([this] -> void {
            const QTemporaryDir installDir;
            const QString target = installDir.path() + u"/RPGMTranslate.AppImage"_s;
            writeFile(target, "old program");
            qputenv("APPIMAGE", target.toUtf8());

            ReleaseServer server;
            server.asset = "not an archive";
            const Env env;
            env.writeTranslation(u"items"_s, "Sword<#>\n");
            env.writeSettings();
            catcher = make_unique<ModalCatcher>();
            catcher->press = u"Install"_s;

            const MainWindow window;
            const WindowGuard guard(window);
            tabsOf(window)->changeTab(u"items"_s);
            edit(window, 0, 1, u"Меч"_s);

            const QString file = env.translationFile(u"items"_s);
            // Restored before the window goes: closing it saves the tab again
            const auto restore = qScopeGuard([&file] -> void {
                QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            });
            QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner));

            checkAction(window)->trigger();

            // The offer, then the refusal of the tab
            QTRY_VERIFY_WITH_TIMEOUT(catcher->seen.size() >= 2, 15000);
            QTest::qWait(300);

            for (const QString& text : catcher->seen) {
                QVERIFY2(!text.contains(u"Failed to open update archive"_qsv), qPrintable(text));
            }

            QCOMPARE(readFile(target), "old program"_ba);
            QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Меч"_s);
        });
    }

    void aFailedStartupCheckStaysSilent() {
        const Env env;
        writeCheckingSettings(env);
        catcher = make_unique<ModalCatcher>();
        AutoUpdater::setReleasesUrl(deadUrl());

        const MainWindow window;
        const WindowGuard guard(window);
        QTest::qWait(500);

        QVERIFY(catcher->seen.isEmpty());
    }

    void aFailedExplicitCheckTellsTheUser() {
        inEventLoop([this] -> void {
            const Env env;
            env.writeSettings();
            catcher = make_unique<ModalCatcher>();
            AutoUpdater::setReleasesUrl(deadUrl());

            const MainWindow window;
            const WindowGuard guard(window);
            checkAction(window)->trigger();

            QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Update failed with error"_qsv), DEAD_SERVER_MS);
        });
    }
};

QTEST_MAIN(TestMainWindowUpdates)
#include "tst_mainwindow_updates.moc"
