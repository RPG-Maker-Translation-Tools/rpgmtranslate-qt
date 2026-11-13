// Opening, closing and keeping a project: the ways a folder becomes a project (or fails to), what is remembered of it, and
// what the window does when tasks are running while the project is closed or the application quits.
#include "Permissions.hpp"
#include "BackupSelectDialog.hpp"
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"
#include "SearchMenu.hpp"
#include "SettingsWindow.hpp"
#include "TaskWorker.hpp"
#include "glazemeta.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStatusBar>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QtTest>

namespace {
constexpr auto ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])";

void writeGame(const Env& env) {
    writeFile(env.project.path() + u"/data/Actors.json"_s, ACTORS_DATA);
    writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
}

// A VX Ace archive (.rgss3a) holding the given files, written the way the engine's format describes: every number
// of the table is XORed with a key derived from the seed, and every file with a key of its own
[[nodiscard]] auto vxAceArchive(const QList<QPair<QByteArray, QByteArray>>& files) -> QByteArray {
    constexpr u32 SEED = 0x12345678;
    constexpr u32 KEY = (SEED * 9) + 3;

    const auto put = [](QByteArray& out, const u32 value) -> void {
        for (i32 shift = 0; shift < 32; shift += 8) {
            out.append(static_cast<char>((value >> shift) & 0xFF));
        }
    };

    usize tableSize = 4;

    for (const auto& [name, data] : files) {
        tableSize += 16 + static_cast<usize>(name.size());
    }

    QByteArray table;
    QByteArray body;
    u32 offset = 12 + static_cast<u32>(tableSize);

    for (const auto& [name, data] : files) {
        const u32 fileKey = 0xA5A5A5A5;

        put(table, offset ^ KEY);
        put(table, static_cast<u32>(data.size()) ^ KEY);
        put(table, fileKey ^ KEY);
        put(table, static_cast<u32>(name.size()) ^ KEY);

        for (qsizetype idx = 0; idx < name.size(); idx++) {
            table.append(static_cast<char>(static_cast<u8>(name[idx]) ^ ((KEY >> (8 * (idx % 4))) & 0xFF)));
        }

        u32 key = fileKey;

        for (qsizetype idx = 0; idx < data.size(); idx++) {
            body.append(static_cast<char>(static_cast<u8>(data[idx]) ^ ((key >> (8 * (idx % 4))) & 0xFF)));

            if (idx % 4 == 3) {
                key = (key * 7) + 3;
            }
        }

        offset += static_cast<u32>(data.size());
    }

    // The table ends with an offset of 0
    put(table, KEY);

    QByteArray result("RGSSAD\0\3", 8);
    put(result, SEED);
    return result + table + body;
}

[[nodiscard]] auto recentProjectsOf(const Env& env) -> QStringList {
    QStringList recent;

    const QJsonObject settings = QJsonDocument::fromJson(readFile(env.settingsPath())).object();

    for (const QJsonValue project : settings[u"core"_s][u"recentProjects"_s].toArray()) {
        recent.append(project.toString());
    }

    return recent;
}

[[nodiscard]] auto workerOf(const MainWindow& window) -> TaskWorker* {
    return window.findChild<TaskWorker*>();
}

// Reads the game through the first-read question and closes the project, which leaves its settings, hashes and baseline
// behind. The window is the one to open the project again with.
void readAndClose(const MainWindow& window, const Env& env) {
    answerFirstRead(window, true);
    openFromRecent(window, env.project.path());
    QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Harold"), 15000);
    QTRY_VERIFY_WITH_TIMEOUT(tabsOf(window)->tabCount() >= 1, 15000);
    closeProject(window);
    QCOMPARE(tabsOf(window)->tabCount(), 0);
}

void writeSourceCheckingSettings(const Env& env) {
    env.writeSettings([&env](Settings& settings) -> void {
        settings.core.projectPath.clear();
        settings.core.recentProjects = { env.project.path() };
        settings.core.checkForSourceUpdates = true;
    });
}
}  // namespace

class TestMainWindowProject final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void init() { catcher = make_unique<ModalCatcher>(); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();

        if (catcher != nullptr) {
            QVERIFY2(catcher->text.isEmpty(), qPrintable(catcher->text));
            catcher.reset();
        }
    }

    void aProjectWithoutSettingsKeepsWhatItsMetadataSays() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        writeFile(env.translationPath() + RVPACKER_METADATA_FILE.toString(), R"({"hashes":{"actors":5},"duplicateMode":1,"readEncoding":"windows-1252"})");
        env.writeSettings();

        {
            const MainWindow window;
            QCOMPARE(tabsOf(window)->tabCount(), 1);
            closeProject(window);
        }

        const auto saved = savedProjectSettings(env);
        QVERIFY(saved.has_value());
        QCOMPARE(saved->duplicateMode, DuplicateMode::Remove);
        QCOMPARE(saved->readEncoding, u"windows-1252"_s);
        QCOMPARE(saved->engineType, EngineType::MVMZ);
    }

    void settingsThatCannotBeUsedFailTheOpeningWhenTheGameCannotBeDetected() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        writeFile(programFile(env, PROJECT_SETTINGS_FILE.toString()), "this is not json");
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        env.writeSettings();

        const MainWindow window;

        QVERIFY(catcher->text.contains(u"project-settings.json is unusable"_qsv));
        QCOMPARE(tabsOf(window)->tabCount(), 0);
        catcher->text.clear();
    }

    void aTranslationThatWasDeletedIsReadAgainAndKeepsTheSettingsOfTheProject() {
        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);
        readAndClose(window, env);

        // A setting of the project that reading does not touch
        auto settings = savedProjectSettings(env);
        QVERIFY(settings.has_value());
        settings->lineLengthHint = 42;
        const auto json = glz::write_json(*settings);
        QVERIFY(json.has_value());
        writeFile(programFile(env, PROJECT_SETTINGS_FILE.toString()), QByteArray(json->data(), static_cast<qsizetype>(json->size())));

        QVERIFY(QDir(env.translationPath()).removeRecursively());
        QVERIFY(!QFile::exists(env.translationFile(u"actors"_s)));

        answerFirstRead(window, true);
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Harold"), 15000);
        QTRY_VERIFY_WITH_TIMEOUT(tabsOf(window)->tabCount() >= 1, 15000);

        closeProject(window);
        const auto kept = savedProjectSettings(env);
        QVERIFY(kept.has_value());
        QCOMPARE(kept->lineLengthHint, u16{ 42 });
    }

    void aLineWithoutTheDelimiterIsReportedWhenTheProjectOpens() {
        const Env env;
        env.writeTranslation(u"actors"_s, "no delimiter in this line\nHarold<#>\n");
        env.writeSettings();

        const MainWindow window;

        QVERIFY(catcher->text.contains(u"failed to split some lines"_qsv));
        QCOMPARE(tabsOf(window)->tabCount(), 1);
        catcher->text.clear();
    }

    void translationFilesWhoseNamesCannotBeTabsAreSkipped() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeTranslation(u"имена"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(tabsOf(window)->tabCount(), 1);
        QCOMPARE(tabsOf(window)->tabName(0), u"actors"_s);
    }

    void theProjectOpenedIsMovedToTheEndOfTheRecentOnes() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings([&env](Settings& settings) -> void {
            settings.core.recentProjects = { env.project.path(), u"/other/one"_s, u"/other/two"_s };
        });

        {
            const MainWindow window;
            closeProject(window);
        }

        QCOMPARE(recentProjectsOf(env), (QStringList{ u"/other/one"_s, u"/other/two"_s, env.project.path() }));
    }

    void onlyTheMostRecentProjectsAreRemembered() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");

        QStringList others;

        for (i32 index = 0; index < 10; index++) {
            others.append(u"/other/%1"_s.arg(index));
        }

        env.writeSettings([&](Settings& settings) -> void { settings.core.recentProjects = others; });

        {
            const MainWindow window;
            closeProject(window);
        }

        const QStringList recent = recentProjectsOf(env);
        QCOMPARE(recent.size(), 10);
        QCOMPARE(recent.last(), env.project.path());
        QCOMPARE(recent.first(), u"/other/1"_s);
    }

    void aBaselineWithoutADataFolderIsMovedIntoOne() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        const QString baseline = programFile(env, BASELINE_DATA_DIRECTORY.toString());
        QVERIFY(QDir().mkpath(baseline));
        writeFile(baseline + u"/Actors.json"_s, "[null]");
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(readFile(baseline + u"/data/Actors.json"_s), "[null]"_ba);
        QVERIFY(!QFile::exists(baseline + u"/Actors.json"_s));
    }

    void aBaselineThatCannotBeMovedIsLoggedAndTheProjectStillOpens() {
        if (permissionsAreIgnored()) {
            QSKIP("the permissions the test takes away are ignored here");
        }

        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        const QString baseline = programFile(env, BASELINE_DATA_DIRECTORY.toString());
        QVERIFY(QDir().mkpath(baseline));
        writeFile(baseline + u"/Actors.json"_s, "[null]");
        env.writeSettings();
        // No `data` folder can be made in a folder that cannot be written
        QVERIFY(QFile::setPermissions(baseline, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to move the baseline files"_s));
        {
            const MainWindow window;
            QVERIFY(QFile::exists(baseline + u"/Actors.json"_s));
            QVERIFY(!QFile::exists(baseline + u"/data"_s));
        }

        QVERIFY(QFile::setPermissions(baseline, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    }

    void aTranslationFolderThatCannotBeCopiedIsLoggedAndNotReused() {
        if (permissionsAreIgnored()) {
            QSKIP("the permissions the test takes away are ignored here");
        }

        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        const QString hidden = env.project.path() + u"/translation/hidden"_s;
        QVERIFY(QDir().mkpath(hidden));
        writeFile(env.project.path() + u"/translation/actors.txt"_s, "Harold<#>Гарольд\n");
        QVERIFY(QFile::setPermissions(hidden, QFileDevice::Permissions()));
        writeRecentSettings(env);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to copy directory"_s));
        {
            const MainWindow window;
            const WindowGuard guard(window);

            catcher.reset();
            const ModalCatcher question(QMessageBox::Yes);
            // The folder was not reused, so the project is read from the game, which the answer declines
            answerFirstRead(window, false);
            openFromRecent(window, env.project.path());
            QCOMPARE(tabsOf(window)->tabCount(), 0);
        }

        QVERIFY(QFile::setPermissions(hidden, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    }

    void aMetadataFileThatCannotBeOpenedIsNamedAndTheFolderNotReused() {
        if (permissionsAreIgnored()) {
            QSKIP("the permissions the test takes away are ignored here");
        }

        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        QVERIFY(QDir().mkpath(env.project.path() + u"/translation"_s));
        writeFile(env.project.path() + u"/translation/actors.txt"_s, "Harold<#>Гарольд\n");
        const QString metadata = env.project.path() + u"/translation/.rvpacker-metadata"_s;
        writeFile(metadata, "{}");
        QVERIFY(QFile::setPermissions(metadata, QFileDevice::Permissions()));
        writeRecentSettings(env);

        {
            const MainWindow window;
            const WindowGuard guard(window);

            catcher.reset();
            const ModalCatcher question(QMessageBox::Yes);
            openFromRecent(window, env.project.path());

            QVERIFY2(
                question.seen.last().contains(u"Failed to open the .rvpacker-metadata file"_qsv),
                qPrintable(question.seen.join(u'|'))
            );
        }

        QVERIFY(QFile::setPermissions(metadata, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }

    void theGameDataIsCopiedAsTheBaselineWhenThereIsNone() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;

        const QString baseline = programFile(env, BASELINE_DATA_DIRECTORY.toString());
        QVERIFY(QFile::exists(baseline + u"/data/System.json"_s));
        QVERIFY(QFile::exists(baseline + u"/js/plugins.js"_s));
    }

    void aGameWithoutItsPluginListGetsNoBaselineAndSaysWhy() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        QVERIFY(QFile::remove(env.project.path() + u"/js/plugins.js"_s));
        env.writeSettings();

        const MainWindow window;

        QVERIFY(catcher->text.contains(u"as a baseline data"_qsv));
        QCOMPARE(tabsOf(window)->tabCount(), 1);
        catcher->text.clear();
    }

    void theDatabaseOfARm2kGameIsCopiedAsTheBaseline() {
        const Env env;
        // An RPG Maker 2000/2003 game keeps its data files at the root, without a `data` folder
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        writeFile(env.project.path() + u"/RPG_RT.ldb"_s, "database");
        writeFile(env.project.path() + u"/Map0001.lmu"_s, "map");
        writeFile(env.project.path() + u"/RPG_RT.exe"_s, "not copied");
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;

        const QString baseline = programFile(env, BASELINE_DATA_DIRECTORY.toString());
        QCOMPARE(readFile(baseline + u"/data/RPG_RT.ldb"_s), "database"_ba);
        QCOMPARE(readFile(baseline + u"/data/Map0001.lmu"_s), "map"_ba);
        QVERIFY(!QFile::exists(baseline + u"/data/RPG_RT.exe"_s));
    }

    void sourceFilesThatWereNeverReadAreOfferedForAppending() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        // A baseline that is there is the sign that the game was copied before, so now it is compared
        QVERIFY(QDir().mkpath(programFile(env, BASELINE_DATA_DIRECTORY.toString()) + u"/data"_s));
        env.writeSettings([](Settings& settings) -> void { settings.core.checkForSourceUpdates = true; });

        const MainWindow window;

        QCOMPARE(catcher->dialogs, QStringList{ u"Source files have been updated"_s });
        QVERIFY(catcher->text.contains(u"New files:"_qsv));
        QVERIFY(catcher->text.contains(u"actors"_qsv));
        catcher->text.clear();
    }

    void aGameThatDidNotChangeIsSaidToBeUpToDate() {
        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        writeSourceCheckingSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);
        readAndClose(window, env);

        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().contains(u"up-to-date"_qsv), 10000);
        QVERIFY(catcher->dialogs.isEmpty());
    }

    void aSourceFileThatChangedIsListedAsChanged() {
        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        writeSourceCheckingSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);
        readAndClose(window, env);

        writeFile(env.project.path() + u"/data/Actors.json"_s, QByteArray(ACTORS_DATA).replace("Harold", "Haroldo"));
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(!catcher->dialogs.isEmpty(), 10000);
        QVERIFY(catcher->text.contains(u"Changed files:\nactors"_qsv));
        QVERIFY(!catcher->text.contains(u"New files:"_qsv));
        catcher->text.clear();
    }

    void aSourceFolderWithoutMatchingFilesIsReported() {
        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        writeSourceCheckingSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);
        readAndClose(window, env);

        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        QVERIFY(QDir().mkpath(env.project.path() + u"/data"_s));
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"No matching source files"_qsv), 10000);
        catcher->text.clear();
    }

    // A tab and the worker address a file by a name of fifteen characters at most: a longer one is not tracked at all,
    // however much it looks like a map
    void aSourceFileWithANameTooLongToTrackIsLeftOut() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        // One character more than the name of a tracked file can hold
        const QString longName = u"Map"_s + QString(static_cast<qsizetype>(sizeof(FilenameArray)) - 3, u'1');
        writeFile(env.project.path() + u"/data/"_s + longName + u".json"_s, "{}");
        QVERIFY(QDir().mkpath(programFile(env, BASELINE_DATA_DIRECTORY.toString()) + u"/data"_s));
        env.writeSettings([](Settings& settings) -> void { settings.core.checkForSourceUpdates = true; });

        const MainWindow window;

        QCOMPARE(catcher->dialogs, QStringList{ u"Source files have been updated"_s });
        QVERIFY(catcher->text.contains(u"actors"_qsv));
        QVERIFY2(!catcher->text.contains(longName, Qt::CaseInsensitive), qPrintable(catcher->text));
        catcher->text.clear();
    }

    void aSourceFileThatCannotBeReadIsListedAmongTheNewOnesAsSkipped() {
        if (permissionsAreIgnored()) {
            QSKIP("the file system reads what a file refuses");
        }

        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        const QString items = env.project.path() + u"/data/Items.json"_s;
        writeFile(items, "[null]");
        QVERIFY(QFile::setPermissions(items, QFileDevice::WriteOwner));
        const auto restore = qScopeGuard([&items] -> void { QFile::setPermissions(items, QFileDevice::ReadOwner | QFileDevice::WriteOwner); });
        QVERIFY(QDir().mkpath(programFile(env, BASELINE_DATA_DIRECTORY.toString()) + u"/data"_s));
        env.writeSettings([](Settings& settings) -> void { settings.core.checkForSourceUpdates = true; });

        const MainWindow window;

        QCOMPARE(catcher->dialogs, QStringList{ u"Source files have been updated"_s });
        QVERIFY2(catcher->text.contains(u"Skipped files:\nItems.json"_qsv), qPrintable(catcher->text));
        catcher->text.clear();
    }

    void aGameThatDidNotChangeSaysWhichSourceFilesCouldNotBeRead() {
        if (permissionsAreIgnored()) {
            QSKIP("the file system reads what a file refuses");
        }

        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        writeSourceCheckingSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);
        readAndClose(window, env);

        const QString actors = env.project.path() + u"/data/Actors.json"_s;
        QVERIFY(QFile::setPermissions(actors, QFileDevice::WriteOwner));
        const auto restore = qScopeGuard([&actors] -> void { QFile::setPermissions(actors, QFileDevice::ReadOwner | QFileDevice::WriteOwner); });

        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().contains(u"Skipped files:"_qsv), 10000);
        QVERIFY(window.statusBar()->currentMessage().contains(u"Actors.json"_qsv));
        QVERIFY(catcher->dialogs.isEmpty());
    }

    // The settings of a project that were saved without its source folder: it is found again, and kept from then on
    void aProjectWhoseSettingsLackTheSourceDirectoryFindsIt() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        writeProjectSettings(env, [](ProjectSettings& project) -> void { project.sourceDirectory = SourceDirectory::None; });
        env.writeSettings();

        {
            const MainWindow window;
            QCOMPARE(tabsOf(window)->tabCount(), 1);
            closeProject(window);
        }

        const auto saved = savedProjectSettings(env);
        QVERIFY(saved.has_value());
        QCOMPARE(saved->sourceDirectory, SourceDirectory::LowercaseData);
    }

    // The first read of a game whose data cannot be read fails, and says so
    void aNewProjectWhoseDataCannotBeReadFailsToLoad() {
        const Env env;
        writeFile(env.project.path() + u"/data/Actors.json"_s, "this is not json");
        forgetProgramFolder(env);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        answerFirstRead(window, true);
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Failed to load project"_qsv) || catcher->text.contains(u"Read failed"_qsv), 15000);
        catcher->text.clear();
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void aGameInAnArchiveThatCannotBeExtractedFailsTheOpening() {
        const Env env;
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        writeFile(env.project.path() + u"/Game.rgss3a"_s, "not an archive");
        forgetProgramFolder(env);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        answerFirstRead(window, true);
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Failed to load project"_qsv), 15000);
        QCOMPARE(tabsOf(window)->tabCount(), 0);
        catcher->text.clear();
    }

    // The extracted files are read as a game's own data, which is what opens the project
    void aGameInAnArchiveIsExtractedAndReadWhenTheProjectOpens() {
        const Env env;
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());

        // Marshal for `[nil]` and for `{}`: the smallest data a VX Ace game can have
        writeFile(
            env.project.path() + u"/Game.rgss3a"_s,
            vxAceArchive({
                { "Data\\Actors.rvdata2"_ba, "\x04\x08[\x06" "0"_ba },
                { "Data\\MapInfos.rvdata2"_ba, "\x04\x08{\x00"_ba },
                { "Data\\System.rvdata2"_ba, "\x04\x08o:\x10RPG::System\x00"_ba },
                { "Data\\Scripts.rvdata2"_ba, "\x04\x08[\x00"_ba },
            })
        );
        forgetProgramFolder(env);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        answerFirstRead(window, true);
        openFromRecent(window, env.project.path());

        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(env.project.path() + u"/Data/Actors.rvdata2"_s), 15000);
        // Reading it makes the translation files, even when a game has next to no text
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(env.translationFile(u"system"_s)), 15000);
        QVERIFY(!catcher->text.contains(u"Failed to load project"_qsv));
        catcher->text.clear();
    }

    void aTranslationFolderWithUnusableMetadataIsNotReused_data() {
        QTest::addColumn<QByteArray>("metadata");
        QTest::addColumn<QString>("message");

        QTest::newRow("not json") << "{ this is not json"_ba << u"Failed to parse the .rvpacker-metadata file"_s;
        QTest::newRow("custom separator") << R"({"hashes":{},"lineSeparator":"@@"})"_ba << u"custom line separator"_s;
        QTest::newRow("custom line break") << R"({"hashes":{},"lineBreak":"<br>"})"_ba << u"custom line separator"_s;
        QTest::newRow("unknown duplicate mode") << R"({"hashes":{},"duplicateMode":9})"_ba
                                                << u"unknown duplicate mode"_s;
    }

    void aTranslationFolderWithUnusableMetadataIsNotReused() {
        QFETCH(const QByteArray, metadata);
        QFETCH(const QString, message);

        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        QVERIFY(QDir().mkpath(env.project.path() + u"/translation"_s));
        writeFile(env.project.path() + u"/translation/actors.txt"_s, "Harold<#>Гарольд\n");
        writeFile(env.project.path() + u"/translation/.rvpacker-metadata"_s, metadata);
        writeRecentSettings(env);

        const MainWindow window;
        const WindowGuard guard(window);

        catcher.reset();
        const ModalCatcher question(QMessageBox::Yes);
        openFromRecent(window, env.project.path());

        QVERIFY2(question.seen.size() >= 2, qPrintable(question.seen.join(u'|')));
        QVERIFY2(question.seen.last().contains(message), qPrintable(question.seen.last()));
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void theMetadataOfAReusedTranslationFolderIsKept() {
        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        QVERIFY(QDir().mkpath(env.project.path() + u"/translation"_s));
        writeFile(env.project.path() + u"/translation/actors.txt"_s, "Harold<#>Гарольд\n");
        writeFile(env.project.path() + u"/translation/.rvpacker-metadata"_s, R"({"hashes":{"actors":5},"duplicateMode":1})");
        writeRecentSettings(env);

        {
            const MainWindow window;
            catcher.reset();
            const ModalCatcher question(QMessageBox::Yes);
            openFromRecent(window, env.project.path());
            QCOMPARE(tabsOf(window)->tabCount(), 1);
            closeProject(window);
        }

        const auto saved = savedProjectSettings(env);
        QVERIFY(saved.has_value());
        QCOMPARE(saved->duplicateMode, DuplicateMode::Remove);
        QVERIFY(!saved->hashes.empty());
    }

    void aProjectCannotBeClosedWhileATaskThatCannotBeInterruptedRuns() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const auto task = workerOf(window)->startTask(TaskKind::Write);
        QVERIFY(task.has_value());

        closeProject(window);

        QVERIFY(catcher->text.contains(u"Cannot close the project while"_qsv));
        QCOMPARE(tabsOf(window)->tabCount(), 1);
        catcher->text.clear();

        workerOf(window)->finishTask(*task);
    }

    void closingTheProjectInterruptsTheTasksThatCanBeAndWaitsForThem() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const auto task = workerOf(window)->startTask(TaskKind::Replace);
        QVERIFY(task.has_value());

        // The task ends once it notices the request
        QTimer::singleShot(100, &window, [&window, token = *task] -> void {
            if (token->interrupted()) {
                workerOf(window)->finishTask(token);
            }
        });

        closeProject(window);

        QVERIFY((*task)->interrupted());
        QVERIFY(workerOf(window)->idle());
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void aTabCannotBeOpenedWhileTheFilesAreBeingRewritten() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const auto task = workerOf(window)->startTask(TaskKind::Replace);
        QVERIFY(task.has_value());

        tabsOf(window)->changeTab(u"actors"_s);

        QVERIFY(window.statusBar()->currentMessage().contains(u"Cannot open a tab while"_qsv));
        QVERIFY(tabsOf(window)->currentTabName().isEmpty());

        workerOf(window)->finishTask(*task);
    }

    void aTaskThatConflictsWithARunningOneIsRefusedByName() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        const auto task = workerOf(window)->startTask(TaskKind::Replace);
        QVERIFY(task.has_value());

        Selected selected;
        selected.flags = FileFlags_Actors;
        emit window.findChild<SearchMenu*>()->actionRequested(
            selected,
            SearchAction::Replace,
            u"Harold"_s,
            u"Harry"_s,
            SearchLocation::Translation,
            -1,
            static_cast<SearchFlags>(0)
        );

        QVERIFY(catcher->text.contains(u"Cannot start"_qsv));
        catcher->text.clear();

        workerOf(window)->finishTask(*task);
    }

    void aTabWhoseFileIsGoneCannotBeOpened() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        QVERIFY(QFile::remove(env.translationFile(u"actors"_s)));

        tabsOf(window)->changeTab(u"actors"_s);

        QVERIFY(catcher->text.contains(u"Failed to open tab"_qsv));
        catcher->text.clear();
        QVERIFY(tabsOf(window)->currentTabName().isEmpty());
    }

    void aTabWhoseFileATaskHasLockedCannotBeOpened() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        emit workerOf(window)->lockFile(u"actors"_s);

        tabsOf(window)->changeTab(u"actors"_s);

        QVERIFY(catcher->text.contains(u"is being locked"_qsv));
        catcher->text.clear();
        QVERIFY(tabsOf(window)->currentTabName().isEmpty());

        // Once the task is done with it the tab opens
        emit workerOf(window)->lockFile(QString());
        tabsOf(window)->changeTab(u"actors"_s);
        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
    }

    void closingTheProjectClosesTheWindowsThatWereOpenedForIt() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        window.findChild<QAction*>(u"actionSettings"_s)->trigger();
        window.findChild<QAction*>(u"actionLoadBackup"_s)->trigger();
        QVERIFY(window.findChild<SettingsWindow*>() != nullptr);
        QVERIFY(window.findChild<BackupSelectDialog*>() != nullptr);

        closeProject(window);

        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<SettingsWindow*>() == nullptr, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<BackupSelectDialog*>() == nullptr, 5000);
    }

    void theSourceFilesOfAnRm2kGameAreCheckedToo() {
        const Env env;
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        writeFile(env.project.path() + u"/RPG_RT.ldb"_s, "database");
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        QVERIFY(QDir().mkpath(programFile(env, BASELINE_DATA_DIRECTORY.toString()) + u"/data"_s));
        env.writeSettings([](Settings& settings) -> void { settings.core.checkForSourceUpdates = true; });

        const MainWindow window;

        QCOMPARE(catcher->dialogs, QStringList{ u"Source files have been updated"_s });
        QVERIFY(catcher->text.contains(u"New files:"_qsv));
        QVERIFY(catcher->text.contains(u"rpg_rt"_qsv));
        catcher->text.clear();
    }

    void appendingTheNewTextOfChangedSourcesKeepsWhatWasTranslated() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\n");
        QVERIFY(QDir().mkpath(programFile(env, BASELINE_DATA_DIRECTORY.toString()) + u"/data"_s));
        env.writeSettings([](Settings& settings) -> void { settings.core.checkForSourceUpdates = true; });
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);

        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("The Brave"), 15000);
        QVERIFY(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд"));

        // The file that cannot be read has no hash, so it is offered again whenever the project is opened. The reopen
        // that follows the read is not to start another one.
        catcher->answer = QMessageBox::No;
        QThreadPool::globalInstance()->waitForDone();
        QTest::qWait(300);
        catcher->text.clear();
    }

    void theSettingsCannotBeOpenedWhileATaskRuns() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const auto task = workerOf(window)->startTask(TaskKind::Replace);
        QVERIFY(task.has_value());

        window.findChild<QAction*>(u"actionSettings"_s)->trigger();

        QVERIFY(window.statusBar()->currentMessage().contains(u"Settings cannot be opened"_qsv));
        QVERIFY(window.findChild<SettingsWindow*>() == nullptr);

        workerOf(window)->finishTask(*task);
    }

    void theSpellcheckDictionaryIsLoadedAgainWhenTheSettingsWindowCloses_data() {
        QTest::addColumn<QByteArray>("dictionary");
        QTest::addColumn<QString>("message");

        QTest::newRow("a working dictionary") << "1\nhello\n"_ba << QString();
        QTest::newRow("a damaged dictionary") << "this is not a dictionary"_ba
                                              << u"Failed to load the spellcheck dictionary"_s;
    }

    void theSpellcheckDictionaryIsLoadedAgainWhenTheSettingsWindowCloses() {
        QFETCH(const QByteArray, dictionary);
        QFETCH(const QString, message);

        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");

        ProjectSettings project;
        project.projectPath = env.project.path();
        project.sourceDirectory = SourceDirectory::LowercaseData;
        project.spellcheckDictionary = u"/en.aff"_s;
        const auto json = glz::write_json(project);
        QVERIFY(json.has_value());
        writeFile(programFile(env, PROJECT_SETTINGS_FILE.toString()), QByteArray(json->data(), static_cast<qsizetype>(json->size())));

        QVERIFY(QDir().mkpath(env.data.path() + u"/dictionaries"_s));
        writeFile(env.data.path() + u"/dictionaries/en.aff"_s, "SET UTF-8\n");
        writeFile(env.data.path() + u"/dictionaries/en.dic"_s, dictionary);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QCOMPARE(tabsOf(window)->tabCount(), 1);

        window.findChild<QAction*>(u"actionSettings"_s)->trigger();
        auto* const settingsWindow = window.findChild<SettingsWindow*>();
        QVERIFY(settingsWindow != nullptr);
        settingsWindow->reject();

        if (message.isEmpty()) {
            QTRY_VERIFY_WITH_TIMEOUT(window.findChild<SettingsWindow*>() == nullptr, 5000);
            QVERIFY(catcher->text.isEmpty());
        } else {
            QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(message), 5000);
            catcher->text.clear();
        }
    }

    // The project names a dictionary whose file is gone: there is none to load, and nothing to say about it
    void aSpellcheckDictionaryThatIsNotThereIsLeftOutWhenTheSettingsWindowCloses() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        writeProjectSettings(env, [](ProjectSettings& project) -> void { project.spellcheckDictionary = u"/ghost.aff"_s; });
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QCOMPARE(tabsOf(window)->tabCount(), 1);

        window.findChild<QAction*>(u"actionSettings"_s)->trigger();
        auto* const settingsWindow = window.findChild<SettingsWindow*>();
        QVERIFY(settingsWindow != nullptr);
        settingsWindow->reject();

        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<SettingsWindow*>() == nullptr, 5000);
        QVERIFY(catcher->text.isEmpty());
    }

    void quittingWhileATaskRunsAsksFirstAndStaysWhenRefused() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const auto task = workerOf(window)->startTask(TaskKind::Write);
        QVERIFY(task.has_value());

        catcher.reset();
        const ModalCatcher question(QMessageBox::No);
        window.findChild<QAction*>(u"actionExit"_s)->trigger();

        QVERIFY(question.text.contains(u"Quit anyway?"_qsv));
        QCOMPARE(tabsOf(window)->tabCount(), 1);
        QVERIFY(!(*task)->interrupted());

        workerOf(window)->finishTask(*task);
    }

    void quittingWhileATaskRunsInterruptsItOnceConfirmed() {
        const Env env;
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        const MainWindow window;
        const auto task = workerOf(window)->startTask(TaskKind::Write);
        QVERIFY(task.has_value());

        catcher.reset();
        const ModalCatcher question(QMessageBox::Yes);
        window.findChild<QAction*>(u"actionExit"_s)->trigger();

        QVERIFY(question.text.contains(u"Quit anyway?"_qsv));
        QVERIFY((*task)->interrupted());

        workerOf(window)->finishTask(*task);
    }
};

QTEST_MAIN(TestMainWindowProject)
#include "tst_mainwindow_project.moc"
