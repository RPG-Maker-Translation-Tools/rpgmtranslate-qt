// What the main window writes to disk, driven the way a user does it: a project that was open at the last exit is
// reopened from the settings file, its tabs are opened and edited through the widgets, and the files are read back.
#include "Permissions.hpp"
#include "BackupArchive.hpp"
#include "BackupSelectDialog.hpp"
#include "Constants.hpp"
#include "MainWindow.hpp"
#include "ModalCatcher.hpp"
#ifdef HAVE_GMOCK
#include "MockCredentialStore.hpp"
#endif
#include "Settings.hpp"
#include "TabPanel.hpp"
#include "MainWindowFixture.hpp"
#include "TestFiles.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "Utils.hpp"

#include <QAction>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMap>
#include <QPushButton>
#include <QStatusBar>
#include <archive.h>
#include <archive_entry.h>
#include <functional>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QToolButton>
#include <QtTest>

namespace {
using ArchiveFiles = vector<pair<QString, QByteArray>>;

// The writer calls that refuse: any header, the data of the second entry, or the close
i32 entriesSeen = 0;

auto refusingHeader(archive*, archive_entry*) -> int {
    return ARCHIVE_FATAL;
}

auto countingHeader(archive* const arch, archive_entry* const entry) -> int {
    ++entriesSeen;
    return archive_write_header(arch, entry);
}

auto refusingSecondData(archive* const arch, const void* const data, const size_t size) -> la_ssize_t {
    return entriesSeen == 2 ? -1 : archive_write_data(arch, data, size);
}

auto refusingClose(archive* const arch) -> int {
    archive_write_close(arch);
    return ARCHIVE_FATAL;
}

// A `.tar.xz` like the ones that `saveBackup` makes
void writeArchive(const QString& path, const ArchiveFiles& files, const bool asDirectory = false) {
    archive* const arch = archive_write_new();
    archive_write_add_filter_xz(arch);
    archive_write_set_format_pax_restricted(arch);
    QVERIFY(archive_write_open_filename(arch, QFile::encodeName(path).constData()) == ARCHIVE_OK);

    for (const auto& [name, data] : files) {
        archive_entry* const entry = archive_entry_new();
        archive_entry_set_pathname_utf8(entry, name.toUtf8().constData());
        archive_entry_set_filetype(entry, asDirectory ? AE_IFDIR : AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, asDirectory ? 0 : data.size());
        QVERIFY(archive_write_header(arch, entry) == ARCHIVE_OK);

        if (!asDirectory) {
            QCOMPARE(archive_write_data(arch, data.constData(), data.size()), data.size());
        }

        archive_entry_free(entry);
    }

    QVERIFY(archive_write_close(arch) == ARCHIVE_OK);
    archive_write_free(arch);
}

[[nodiscard]] auto readArchive(const QString& path) -> QMap<QString, QByteArray> {
    QMap<QString, QByteArray> files;
    const QByteArray content = readFile(path);

    archive* const arch = archive_read_new();
    archive_read_support_format_tar(arch);
    archive_read_support_filter_xz(arch);

    if (archive_read_open_memory(arch, content.constData(), content.size()) != ARCHIVE_OK) {
        archive_read_free(arch);
        return files;
    }

    archive_entry* entry = nullptr;

    while (archive_read_next_header(arch, &entry) == ARCHIVE_OK) {
        QByteArray data(static_cast<qsizetype>(archive_entry_size(entry)), Qt::Uninitialized);
        archive_read_data(arch, data.data(), data.size());
        files.insert(QString::fromUtf8(archive_entry_pathname(entry)), data);
    }

    archive_read_free(arch);
    return files;
}

[[nodiscard]] auto backupsOf(const Env& env) -> QStringList {
    return QDir(env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + BACKUP_DIRECTORY.toString())
        .entryList({ u"*.tar.xz"_s }, QDir::Files, QDir::Name);
}

[[nodiscard]] auto backupPathOf(const Env& env) -> QString {
    return env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + BACKUP_DIRECTORY.toString();
}

// Opens the dialog of "Load Backup", picks the first backup of the list and presses "Load"
void loadFirstBackup(const MainWindow& window) {
    window.findChild<QAction*>(u"actionLoadBackup"_s)->trigger();

    auto* const dialog = window.findChild<BackupSelectDialog*>();
    QVERIFY(dialog != nullptr);

    auto* const list = dialog->findChild<QListWidget*>();
    QVERIFY(list->count() > 0);
    list->setCurrentRow(0);

    // The dialog deletes itself when it is accepted
    dialog->findChild<QPushButton*>()->click();
}
// The saved settings of the application
[[nodiscard]] auto savedSettings(const Env& env) -> QJsonObject {
    return QJsonDocument::fromJson(readFile(env.settingsPath())).object();
}

[[nodiscard]] auto savedEndpointIds(const Env& env) -> QStringList {
    QStringList ids;

    for (const QJsonValue endpoint : savedSettings(env)[u"translation"_s][u"endpoints"_s].toArray()) {
        ids.append(endpoint[u"id"_s].toString());
    }

    return ids;
}

[[nodiscard]] auto storedCredential(const char* const account) -> optional<QString> {
    FFIString stored;

    if (!rpgm_get_credential(strtoffi(account), &stored)) {
        return nullopt;
    }

    auto key = QString::fromUtf8(stored.ptr, stored.len);
    rpgm_string_free(stored);
    return key;
}

void makeReadOnly(const QString& path) {
    QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner));
}

void makeWritable(const QString& path) {
    QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
}
}  // namespace

class TestMainWindowSaveLoad final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() {
        // Saving settings stores and deletes API keys: they must not reach the credential store of the user
        qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1");
    }

    void init() {
        // Nothing here asks the user anything: a box that opens is a failure, and would block the test otherwise
        catcher = make_unique<ModalCatcher>();
    }

    void cleanup() {
        // A backup that is still compressing owns files of the project
        QThreadPool::globalInstance()->waitForDone();

        // The credential store of the process outlives each test, and the next one must not find these keys in it
        for (const char* const account : { "languagetool", "endpoint-1", "endpoint-2" }) {
            (void)rpgm_delete_credential(strtoffi(account));
        }

        if (catcher != nullptr) {
            QVERIFY2(catcher->text.isEmpty(), qPrintable(catcher->text));
            catcher.reset();
        }
    }

    void reopensTheProjectOfTheLastSession() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Меч\nShield<#>\n");
        env.writeTranslation(u"armors"_s, "Cap<#>\n");
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(tabsOf(window)->tabCount(), 2);

        QStringList names;

        for (i32 index = 0; index < tabsOf(window)->tabCount(); index++) {
            names.append(tabsOf(window)->tabName(index));
        }

        names.sort();
        QCOMPARE(names, (QStringList{ u"armors"_s, u"items"_s }));

        const i32 items = tabsOf(window)->tabName(0) == u"items"_s ? 0 : 1;
        QCOMPARE(tabsOf(window)->tabTotal(items), 2);
        QCOMPARE(tabsOf(window)->tabTranslated(items), 1);
    }

    void aTabThatWasNotEditedIsWrittenBackByteForByte() {
        const Env env;
        // Comments are kept whole, and a cell may hold an escaped line break
        const QByteArray content = "<!>NAME<#>Hero\nSword<#>Меч\\#острый\nShield<#>Щит\n<!>BOOKMARK<#>note\n";
        env.writeTranslation(u"items"_s, content);
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        closeTab(window);

        QCOMPARE(env.readTranslation(u"items"_s), content);
    }

    void aRowWithFewerColumnsIsPaddedToTheWidthOfTheTable() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Меч\nShield<#>Щит<#>Shield\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        closeTab(window);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч<#>\nShield<#>Щит<#>Shield\n"));
    }

    void anEditedCellIsSavedWhenTheTabIsClosed() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\nShield<#>Щит\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);
        closeTab(window);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\nShield<#>Щит\n"));
    }

    void aLineBreakInACellIsWrittenEscaped() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"one\ntwo"_s);
        closeTab(window);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>one\\#two\n"));
    }

    void theSaveButtonWritesTheOpenTabAndKeepsItOpen() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);
        save(window);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
        QCOMPARE(tabsOf(window)->currentTabName(), u"items"_s);
    }

    void switchingTabsSavesTheOneLeftAndTouchesNoOther() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeTranslation(u"armors"_s, "Cap<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);
        tabsOf(window)->changeTab(u"armors"_s);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
        QCOMPARE(env.readTranslation(u"armors"_s), QByteArray("Cap<#>\n"));
    }

    void theGameTitleIsKeptThroughASave() {
        const Env env;
        const QByteArray content = "Gold<#>Золото\n<!>NAME<#>Game Title\nMy Game<#>";
        env.writeTranslation(u"system"_s, content);
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(titleInputOf(window)->placeholderText(), u"My Game"_s);

        tabsOf(window)->changeTab(u"system"_s);
        closeTab(window);

        QCOMPARE(env.readTranslation(u"system"_s), content);
    }

    // The title is the line after its marker; one that cannot be split is not a title, and the file still opens
    void aGameTitleLineThatCannotBeSplitIsNotATitle() {
        const Env env;
        env.writeTranslation(u"system"_s, "Gold<#>Золото\n<!>NAME<#>Game Title\nMy Game");
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(tabsOf(window)->tabCount(), 1);
        QVERIFY(titleInputOf(window)->placeholderText().isEmpty());
    }

    void aTranslatedGameTitleFillsTheTitleInput() {
        const Env env;
        env.writeTranslation(u"system"_s, "Gold<#>Золото\n<!>NAME<#>Game Title\nMy Game<#>Моя игра");
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(titleInputOf(window)->placeholderText(), u"My Game"_s);
        QCOMPARE(titleInputOf(window)->text(), u"Моя игра"_s);
    }

    void blankLinesAndExtraPartsOfAMapHeaderAreNotRows() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n   \nShield<#>\n");
        env.writeTranslation(u"maps"_s, "<!>ID<#>3<#>extra\nHello<#>\n");
        env.writeSettings();

        const MainWindow window;

        // The header names the map by its number alone
        QStringList names;
        i32 itemsTotal = 0;

        for (i32 index = 0; index < tabsOf(window)->tabCount(); index++) {
            names.append(tabsOf(window)->tabName(index));

            if (names.last() == u"items"_s) {
                itemsTotal = tabsOf(window)->tabTotal(index);
            }
        }

        names.sort();
        QCOMPARE(names, (QStringList{ u"items"_s, u"map3"_s }));
        QCOMPARE(itemsTotal, 2);
    }

    void aTranslationFileThatCannotBeReadIsLeftOutWhenTheProjectOpens() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeTranslation(u"armors"_s, "Cap<#>\n");
        env.writeSettings();
        QVERIFY(QFile::setPermissions(env.translationFile(u"armors"_s), QFileDevice::WriteOwner));

        const MainWindow window;

        QCOMPARE(tabsOf(window)->tabCount(), 1);
        QCOMPARE(tabsOf(window)->tabName(0), u"items"_s);

        makeWritable(env.translationFile(u"armors"_s));
    }

    void anEditedGameTitleIsSavedAfterTheMarker() {
        const Env env;
        env.writeTranslation(u"system"_s, "Gold<#>Золото\n<!>NAME<#>Game Title\nMy Game<#>");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"system"_s);
        titleInputOf(window)->setText(u"Моя игра"_s);
        closeTab(window);

        QCOMPARE(env.readTranslation(u"system"_s), QByteArray("Gold<#>Золото\n<!>NAME<#>Game Title\nMy Game<#>Моя игра"));
    }

    void aMapIsHeldInMemoryAndWrittenToMapsTxtOnSave() {
        const Env env;
        env.writeTranslation(u"maps"_s, "<!>ID<#>1\nHello<#>\n<!>ID<#>2\nWorld<#>Мир\n");
        env.writeSettings();

        const MainWindow window;

        QCOMPARE(tabsOf(window)->tabCount(), 2);

        tabsOf(window)->changeTab(u"map1"_s);
        // The first row is the `<!>ID` comment of the section
        edit(window, 1, 1, u"Привет"_s);
        save(window);

        QCOMPARE(
            env.readTranslation(u"maps"_s),
            QByteArray("<!>ID<#>1\nHello<#>Привет\n<!>ID<#>2\nWorld<#>Мир\n")
        );
    }

    void closingTheProjectSavesTheTabAndTheProjectFiles() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);
        closeProject(window);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
        QCOMPARE(tabsOf(window)->tabCount(), 0);
        QVERIFY(QFile::exists(env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + PROJECT_SETTINGS_FILE.toString()));
        QVERIFY(QFile::exists(env.translationPath() + RVPACKER_METADATA_FILE.toString()));
    }

    void anApiKeyLeavesTheSettingsFileForTheCredentialStore() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            EndpointSettings endpoint;
            endpoint.id = u"test-endpoint"_s;
            endpoint.name = u"Test"_s;
            endpoint.apiKey = u"top-secret-key"_s;
            endpoint.endpoint = TranslationEndpoint::Google;
            settings.translation.endpoints.push_back(endpoint);
        });

        const MainWindow window;
        closeProject(window);

        const QByteArray saved = readFile(env.settingsPath());
        QVERIFY(saved.contains("test-endpoint"));
        QVERIFY(!saved.contains("top-secret-key"));

        FFIString stored;
        QVERIFY(rpgm_get_credential(strtoffi("test-endpoint"), &stored));
        QCOMPARE(QString::fromUtf8(stored.ptr, stored.len), u"top-secret-key"_s);
        rpgm_string_free(stored);

        // Not left in the credential store of the process for the next test
        QVERIFY(rpgm_delete_credential(strtoffi("test-endpoint")));
    }

    void aKeyThatIsOnlyInTheCredentialStoreIsLoadedAndStoredAgain() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        QVERIFY(rpgm_store_credential(strtoffi("cleared-endpoint"), strtoffi("old-key")));
        env.writeSettings([](Settings& settings) -> void {
            EndpointSettings endpoint;
            endpoint.id = u"cleared-endpoint"_s;
            endpoint.name = u"Test"_s;
            endpoint.endpoint = TranslationEndpoint::Google;
            settings.translation.endpoints.push_back(endpoint);
        });

        // The settings file holds no key: the one of the store comes into the endpoint at the start, and the save
        // puts it back
        const MainWindow window;
        closeProject(window);

        FFIString stored;
        QVERIFY(rpgm_get_credential(strtoffi("cleared-endpoint"), &stored));
        QCOMPARE(QString::fromUtf8(stored.ptr, stored.len), u"old-key"_s);
        rpgm_string_free(stored);
        QVERIFY(!readFile(env.settingsPath()).contains("old-key"));

        QVERIFY(rpgm_delete_credential(strtoffi("cleared-endpoint")));
    }

    void theBackupTimerArchivesTheTranslationFiles() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Меч\n");
        env.writeTranslation(u"armors"_s, "Cap<#>Шапка\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        const MainWindow window;

        QTRY_VERIFY_WITH_TIMEOUT(!backupsOf(env).isEmpty(), 5000);

        const auto files = readArchive(backupPathOf(env) + u'/' + backupsOf(env).first());
        QCOMPARE(files.value(u"items.txt"_s), QByteArray("Sword<#>Меч\n"));
        QCOMPARE(files.value(u"armors.txt"_s), QByteArray("Cap<#>Шапка\n"));
        // Only the translation: no settings of the application, nor the archives themselves
        QVERIFY(!files.contains(u"settings.json"_s));
    }

    void aBackupIncludesTheEditsOfTheOpenTab() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);

        QTRY_VERIFY_WITH_TIMEOUT(!backupsOf(env).isEmpty(), 5000);

        QCOMPARE(readArchive(backupPathOf(env) + u'/' + backupsOf(env).first()).value(u"items.txt"_s), QByteArray("Sword<#>Меч\n"));
    }

    void theOldestBackupsAreDroppedBeyondTheLimit() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
            settings.core.backup.max = 2;
        });

        QDir().mkpath(backupPathOf(env));

        for (i32 age = 1; age <= 3; age++) {
            const QString path = backupPathOf(env) + u"/old%1.tar.xz"_s.arg(age);
            writeArchive(path, { { u"items.txt"_s, "old" } });

            QFile file(path);
            QVERIFY(file.open(QIODevice::ReadWrite));
            QVERIFY(file.setFileTime(QDateTime::currentDateTime().addDays(-age), QFileDevice::FileModificationTime));
        }

        const MainWindow window;

        // The new archive and the newest of the old ones
        QTRY_VERIFY_WITH_TIMEOUT(backupsOf(env).size() == 2 && backupsOf(env).contains(u"old1.tar.xz"_s), 5000);
        QVERIFY(!backupsOf(env).contains(u"old2.tar.xz"_s));
        QVERIFY(!backupsOf(env).contains(u"old3.tar.xz"_s));
    }

    void anInterruptedBackupLeftNoPartFileBehind() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        QDir().mkpath(backupPathOf(env));
        writeFile(backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz.part"_s, "half a backup");

        const MainWindow window;

        QTRY_VERIFY_WITH_TIMEOUT(!backupsOf(env).isEmpty(), 5000);
        QVERIFY(!QFile::exists(backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz.part"_s));
    }

    void aBackupThatCannotBeWrittenIsReportedAndLeavesTheProjectAlone() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Меч\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        // A file where the folder of the backups should be: no archive can be opened under it
        QDir().mkpath(QFileInfo(backupPathOf(env)).path());
        writeFile(backupPathOf(env), "not a folder");

        const MainWindow window;

        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().contains(u"Failed to create the backup"_qsv), 5000);
        QCOMPARE(readFile(backupPathOf(env)), QByteArray("not a folder"));
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
    }

    void aWriterThatFailsLeavesTheOldArchiveAndNoPartFile_data() {
        QTest::addColumn<int>("stage");
        QTest::newRow("header of the first entry") << 0;
        QTest::newRow("data of the second entry") << 1;
        QTest::newRow("close") << 2;
    }

    void aWriterThatFailsLeavesTheOldArchiveAndNoPartFile() {
        QFETCH(int, stage);
        const QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString archivePath = dir.filePath(u"backup.tar.xz"_s);
        writeFile(archivePath, "the old backup");
        const ArchiveFiles files{ { u"a.txt"_s, "first" }, { u"b.txt"_s, "second" } };

        ArchiveWriter writer;
        entriesSeen = 0;

        if (stage == 0) {
            writer.writeHeader = refusingHeader;
        } else if (stage == 1) {
            writer.writeHeader = countingHeader;
            writer.writeData = refusingSecondData;
        } else {
            writer.close = refusingClose;
        }

        QVERIFY(!writeBackupArchive(archivePath, files, writer));
        QCOMPARE(readFile(archivePath), QByteArray("the old backup"));
        QVERIFY(!QFile::exists(archivePath + u".part"_s));
    }

    void anArchiveThatCannotBeRenamedIntoPlaceLeavesNoPartFile() {
        const QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // A folder with something in it stands where the archive goes: it can be neither removed nor replaced
        const QString archivePath = dir.filePath(u"backup.tar.xz"_s);
        QVERIFY(QDir().mkpath(archivePath));
        writeFile(archivePath + u"/keep"_s, "kept");

        QVERIFY(!writeBackupArchive(archivePath, { { u"a.txt"_s, "first" } }));
        QCOMPARE(readFile(archivePath + u"/keep"_s), QByteArray("kept"));
        QVERIFY(!QFile::exists(archivePath + u".part"_s));
    }

    void loadingABackupRestoresItsFilesAndReopensTheProject() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>BROKEN\n");
        env.writeSettings();

        QDir().mkpath(backupPathOf(env));
        writeArchive(
            backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s,
            { { u"items.txt"_s, "Sword<#>Меч\n" }, { u"armors.txt"_s, "Cap<#>Шапка\n" } }
        );

        const MainWindow window;

        QCOMPARE(tabsOf(window)->tabCount(), 1);

        loadFirstBackup(window);

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
        QCOMPARE(env.readTranslation(u"armors"_s), QByteArray("Cap<#>Шапка\n"));
        // The project was opened again from the restored files
        QCOMPARE(tabsOf(window)->tabCount(), 2);
    }

    void loadingABackupSavesTheOpenTabFirstAndThenReplacesIt() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        QDir().mkpath(backupPathOf(env));
        writeArchive(backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s, { { u"items.txt"_s, "Sword<#>Backup\n" } });

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Edit"_s);

        loadFirstBackup(window);

        // What the editor held did not come back over the restored file
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Backup\n"));
        QVERIFY(tabsOf(window)->currentTabName().isEmpty());
    }

    void entriesThatCouldEscapeTheTranslationFolderAreNotRestored() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        QDir().mkpath(backupPathOf(env));
        writeArchive(
            backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s,
            { { u"../escaped.txt"_s, "x" }, { u"sub/nested.txt"_s, "x" }, { u"items.txt"_s, "Sword<#>Меч\n" } }
        );

        const MainWindow window;
        loadFirstBackup(window);

        QVERIFY(!QFile::exists(env.translationPath() + u"/../escaped.txt"_s));
        QVERIFY(!QFile::exists(env.translationPath() + u"/sub/nested.txt"_s));
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
    }

    void anEntryThatIsNotAFileIsNotRestored() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        QDir().mkpath(backupPathOf(env));
        writeArchive(backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s, { { u"items.txt"_s, QByteArray() } }, true);

        const MainWindow window;
        loadFirstBackup(window);

        // A directory entry named like a translation file must not blank the file
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>\n"));
    }

    // A backup that cannot be restored is reported, and the translation it was meant to replace stays as it is
    void anUnusableBackupIsRefusedAndNothingIsTouched_data() {
        QTest::addColumn<i32>("kind");
        QTest::addColumn<QString>("message");
        QTest::addColumn<i32>("tabs");

        QTest::newRow("aBackupThatCannotBeReadIsReportedAndTouchesNothing") << 0 << u"Opening backup"_s << -1;
        QTest::newRow("aFileThatIsNotAnArchiveIsRefusedAndNothingIsTouched") << 1 << QString() << 1;
        QTest::newRow("aDamagedBackupNeverDestroysTheTranslationItWasMeantToRestore") << 2 << QString() << -1;
    }

    void anUnusableBackupIsRefusedAndNothingIsTouched() {
        QFETCH(const i32, kind);
        QFETCH(const QString, message);
        QFETCH(const i32, tabs);

        if (kind == 0 && runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Original\n");
        env.writeSettings();

        const QString backup = backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s;
        QDir().mkpath(backupPathOf(env));

        if (kind == 0) {
            writeArchive(backup, { { u"items.txt"_s, "Sword<#>Меч\n" } });
            QVERIFY(QFile::setPermissions(backup, QFileDevice::WriteOwner));
        } else if (kind == 1) {
            writeFile(backup, "this is not an archive");
        } else {
            // Incompressible, so that cutting the archive cuts into the data of the entry
            QByteArray noise(300000, Qt::Uninitialized);
            QRandomGenerator generator(7);

            for (char& byte : noise) {
                byte = static_cast<char>(generator.bounded(256));
            }

            writeArchive(backup, { { u"items.txt"_s, noise } });

            const QByteArray whole = readFile(backup);
            QVERIFY(whole.size() > 100000);
            writeFile(backup, whole.first(whole.size() / 2));
        }

        // The catcher keeps answering, so it takes both messages of a damaged archive. Two of them would race for each box.
        catcher.reset();
        ModalCatcher refusal;

        const MainWindow window;
        loadFirstBackup(window);

        QVERIFY(!refusal.text.isEmpty());
        QVERIFY2(refusal.text.contains(message), qPrintable(refusal.text));
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Original\n"));

        if (tabs >= 0) {
            QCOMPARE(tabsOf(window)->tabCount(), tabs);
        }

        if (kind == 0) {
            makeWritable(backup);
        }
    }

    void aSettingsFileThatCannotBeParsedFallsBackToTheDefaults() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        writeFile(env.settingsPath(), "{ not json");

        const MainWindow window;

        // The last project is part of the lost settings, so nothing is opened
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void aTabThatCannotBeWrittenStaysOpenWithItsEdits() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeTranslation(u"armors"_s, "Cap<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);

        QVERIFY(QFile::setPermissions(env.translationFile(u"items"_s), QFileDevice::ReadOwner));

        catcher.reset();
        ModalCatcher refusal;
        tabsOf(window)->changeTab(u"armors"_s);

        QVERIFY(!refusal.text.isEmpty());
        // The switch was undone: the edits are still in the table, and nothing was lost on disk
        QCOMPARE(tabsOf(window)->currentTabName(), u"items"_s);
        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Меч"_s);
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>\n"));

        QVERIFY(QFile::setPermissions(env.translationFile(u"items"_s), QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }

    // The box that comes up when a file cannot be opened for writing: Continue anyway goes on without that file (Abort,
    // which stops what was being done, is aTabThatCannotBeWrittenStaysOpenWithItsEdits)
    void aTabThatCannotBeOpenedForWritingIsSkipped() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeTranslation(u"armors"_s, "Cap<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);
        makeReadOnly(env.translationFile(u"items"_s));

        catcher.reset();
        ModalCatcher refusal;
        refusal.press = u"Continue anyway"_s;
        tabsOf(window)->changeTab(u"armors"_s);

        QVERIFY(!refusal.text.isEmpty());
        QCOMPARE(tabsOf(window)->currentTabName(), u"armors"_s);
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>\n"));

        makeWritable(env.translationFile(u"items"_s));
    }

    void closingTheProjectIsAbortedOrSkippedPerFile_data() {
        QTest::addColumn<bool>("maps");
        QTest::addColumn<QString>("button");
        QTest::addColumn<i32>("tabsLeft");

        QTest::newRow("maps, abort") << true << u"Abort"_s << 2;
        QTest::newRow("maps, continue anyway") << true << u"Continue anyway"_s << 0;
        QTest::newRow("project settings, abort") << false << u"Abort"_s << 2;
        QTest::newRow("project settings, continue anyway") << false << u"Continue anyway"_s << 0;
    }

    void closingTheProjectIsAbortedOrSkippedPerFile() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        QFETCH(const bool, maps);
        QFETCH(const QString, button);
        QFETCH(const i32, tabsLeft);

        const Env env;
        env.writeTranslation(u"maps"_s, "<!>ID<#>1\nHello<#>\n");
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        const MainWindow window;
        QCOMPARE(tabsOf(window)->tabCount(), 2);

        const QString target = maps ? env.translationFile(u"maps"_s)
                                    : env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + PROJECT_SETTINGS_FILE.toString();
        const QByteArray before = readFile(target);
        makeReadOnly(target);

        catcher.reset();
        ModalCatcher refusal;
        refusal.press = button;
        closeProject(window);

        QVERIFY(!refusal.text.isEmpty());
        QCOMPARE(tabsOf(window)->tabCount(), tabsLeft);
        // The file that could not be opened is as it was, whichever way the box was answered
        QCOMPARE(readFile(target), before);

        makeWritable(target);
    }

    // saveEverything runs settings, tab, maps, project settings, glossary, metadata in that order and stops at the first
    // refusal: what comes before a refused file is saved, what comes after is not
    void closingTheProjectStopsAtTheFirstRefusedFile_data() {
        QTest::addColumn<i32>("target");
        QTest::addColumn<QString>("button");
        QTest::addColumn<i32>("tabsLeft");
        QTest::addColumn<bool>("tabSaved");

        // 0 settings, 1 glossary, 2 metadata
        QTest::newRow("settings, abort") << 0 << u"Abort"_s << 1 << false;
        QTest::newRow("settings, continue anyway") << 0 << u"Continue anyway"_s << 0 << true;
        QTest::newRow("glossary, abort") << 1 << u"Abort"_s << 1 << true;
        QTest::newRow("glossary, continue anyway") << 1 << u"Continue anyway"_s << 0 << true;
        QTest::newRow("metadata, abort") << 2 << u"Abort"_s << 1 << true;
        QTest::newRow("metadata, continue anyway") << 2 << u"Continue anyway"_s << 0 << true;
    }

    void closingTheProjectStopsAtTheFirstRefusedFile() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        QFETCH(const i32, target);
        QFETCH(const QString, button);
        QFETCH(const i32, tabsLeft);
        QFETCH(const bool, tabSaved);

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);

        const QString dataDirectory = env.project.path() + PROGRAM_DATA_DIRECTORY.toString();
        const QString path = target == 0   ? env.settingsPath()
                             : target == 1 ? dataDirectory + GLOSSARY_FILE.toString()
                                           : env.translationFile(u"items"_s).section(u'/', 0, -2) + RVPACKER_METADATA_FILE.toString();

        // The file may not exist yet; a read-only file is what makes the box come up
        if (!QFile::exists(path)) {
            writeFile(path, "{}");
        }

        const QByteArray before = readFile(path);
        makeReadOnly(path);

        catcher.reset();
        ModalCatcher refusal;
        refusal.press = button;
        closeProject(window);

        QVERIFY(!refusal.text.isEmpty());
        QCOMPARE(tabsOf(window)->tabCount(), tabsLeft);
        QCOMPARE(readFile(path), before);
        QCOMPARE(env.readTranslation(u"items"_s), tabSaved ? QByteArray("Sword<#>Меч\n") : QByteArray("Sword<#>\n"));

        makeWritable(path);
    }

    void aScriptThatCannotBeSavedAbortsClosingTheProject() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        // The scripts of the batch menu are loaded from this folder at start
        const QString script = env.data.path() + u"/scripts/mine.js"_s;
        QVERIFY(QDir().mkpath(env.data.path() + u"/scripts"_s));
        writeFile(script, "// mine");
        makeReadOnly(script);

        const MainWindow window;
        QCOMPARE(tabsOf(window)->tabCount(), 1);

        catcher.reset();
        ModalCatcher refusal;
        refusal.press = u"Abort"_s;
        closeProject(window);

        QVERIFY(!refusal.text.isEmpty());
        QCOMPARE(tabsOf(window)->tabCount(), 1);
        QCOMPARE(readFile(script), QByteArray("// mine"));

        makeWritable(script);
    }

    void aFileThatCannotBeReadIsLeftOutOfTheBackup() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Меч\n");
        env.writeTranslation(u"armors"_s, "Cap<#>Шапка\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        const MainWindow window;
        QVERIFY(QFile::setPermissions(env.translationFile(u"armors"_s), QFileDevice::WriteOwner));

        QTRY_VERIFY_WITH_TIMEOUT(!backupsOf(env).isEmpty(), 5000);

        const auto files = readArchive(backupPathOf(env) + u'/' + backupsOf(env).first());
        QCOMPARE(files.value(u"items.txt"_s), QByteArray("Sword<#>Меч\n"));
        QVERIFY(!files.contains(u"armors.txt"_s));

        makeWritable(env.translationFile(u"armors"_s));
    }

    // 615-617: the new content is complete but cannot replace the file; the other entries are restored regardless
    void anEntryThatCannotReplaceItsFileIsReportedAndTheOthersAreRestored() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Original\n");
        env.writeSettings();

        QDir().mkpath(backupPathOf(env));
        writeArchive(
            backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s,
            { { u"items.txt"_s, "Sword<#>Меч\n" }, { u"armors.txt"_s, "Cap<#>Шапка\n" } }
        );

        const MainWindow window;

        // A folder that holds something cannot be replaced by a file
        QVERIFY(QFile::remove(env.translationFile(u"items"_s)));
        QVERIFY(QDir().mkpath(env.translationFile(u"items"_s)));
        writeFile(env.translationFile(u"items"_s) + u"/inside"_s, "x");

        catcher.reset();
        ModalCatcher refusal;
        loadFirstBackup(window);

        QVERIFY(refusal.text.contains(u"Writing entry"_qsv));
        QCOMPARE(env.readTranslation(u"armors"_s), QByteArray("Cap<#>Шапка\n"));
    }

    void aLanguageTheProgramIsNotTranslatedIntoFallsBackToEnglish() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void { settings.appearance.language = QLocale::Zulu; });

        const MainWindow window;
        closeProject(window);

        const auto saved = readJsonFile<Settings>(env.settingsPath());
        QVERIFY(saved.has_value());
        QCOMPARE(saved->appearance.language, QLocale::English);
    }

    void exitingKeepsTheProjectForTheNextStart() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        {
            const MainWindow window;
            tabsOf(window)->changeTab(u"items"_s);
            edit(window, 0, 1, u"Меч"_s);
            window.findChild<QAction*>(u"actionExit"_s)->trigger();
        }

        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>Меч\n"));
        // The settings that were there are kept aside by the save
        QVERIFY(QFile::exists(env.settingsPath() + BACKUP_SUFFIX.toString()));

        const MainWindow next;
        QCOMPARE(tabsOf(next)->tabCount(), 1);
    }

    void theHotkeyThatClashedWithSaveIsReset() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void { settings.controls.translationsMenu = u"Ctrl+S"_s; });

        const MainWindow window;
        closeProject(window);

        QCOMPARE(savedSettings(env)[u"controls"_s][u"translationsMenu"_s].toString(), ControlSettings{}.translationsMenu);
    }

    void endpointsOfRemovedBackendsAreDroppedWithTheirKeys() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");

        QVERIFY(rpgm_store_credential(strtoffi("gone-llm"), strtoffi("key")));
        QVERIFY(rpgm_store_credential(strtoffi("gone-script"), strtoffi("key")));

        env.writeSettings([](Settings& settings) -> void {
            const auto add = [&settings](const char* const id, const TranslationEndpoint kind, const QString& script) -> void {
                EndpointSettings endpoint;
                endpoint.id = QString::fromLatin1(id);
                endpoint.endpoint = kind;
                endpoint.scriptPath = script;
                settings.translation.endpoints.push_back(endpoint);
            };

            // An LLM backend of v1.1.x, and a leftover that shares its number with `Script` but has no script
            add("gone-llm", static_cast<TranslationEndpoint>(7), QString());
            add("gone-script", TranslationEndpoint::Script, QString());
            add("kept-script", TranslationEndpoint::Script, u"translate.js"_s);
            add("kept-google", TranslationEndpoint::Google, QString());
        });

        const MainWindow window;
        closeProject(window);

        QCOMPARE(savedEndpointIds(env), (QStringList{ u"kept-script"_s, u"kept-google"_s }));
        QVERIFY(!storedCredential("gone-llm").has_value());
        QVERIFY(!storedCredential("gone-script").has_value());
    }

    void anEndpointWithoutAnIdGetsOne() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            EndpointSettings endpoint;
            endpoint.endpoint = TranslationEndpoint::Google;
            settings.translation.endpoints.push_back(endpoint);
        });

        const MainWindow window;
        closeProject(window);

        const QStringList ids = savedEndpointIds(env);
        QCOMPARE(ids.size(), 1);
        // The account of its API key in the credential store: it must not be empty, nor change between starts
        QVERIFY(!ids.first().isEmpty());
    }

#ifdef HAVE_GMOCK
    // The system credential store refuses: the keys must stay where they can still be found, and the store must be
    // asked in the right terms. A mock of the store is how it is made to refuse.
    void theKeysTheCredentialStoreRefusesStayInTheSettingsFile() {
        using testing::NiceMock;
        using testing::Return;

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            EndpointSettings endpoint;
            endpoint.id = u"endpoint-1"_s;
            endpoint.endpoint = TranslationEndpoint::Google;
            endpoint.apiKey = u"google-key"_s;
            settings.translation.endpoints.push_back(endpoint);
            settings.translation.languageTool.apiKey = u"lt-key"_s;
        });

        MainWindow window;
        GMockFailures* const failures = GMockFailures::install();

        auto owned = make_unique<NiceMock<MockCredentialStore>>();
        auto* const credentials = owned.get();
        ON_CALL(*credentials, remove).WillByDefault(Return(true));
        EXPECT_CALL(*credentials, store(QByteArrayView("endpoint-1"), QByteArrayView("google-key"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*credentials, store(QByteArrayView("languagetool"), QByteArrayView("lt-key"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*credentials, error()).WillRepeatedly(Return(u"no keyring service"_s));
        window.setCredentialStore(std::move(owned));

        closeProject(window);

        // Both the store and what it was told are as expected
        QVERIFY(testing::Mock::VerifyAndClearExpectations(credentials));
        QVERIFY2(failures->messages.isEmpty(), qPrintable(failures->messages.join(u'\n')));

        // Losing a key silently is worse than leaving it in the file
        const QByteArray saved = readFile(env.settingsPath());
        QVERIFY(saved.contains("google-key"));
        QVERIFY(saved.contains("lt-key"));
    }

    void theKeysTheCredentialStoreTakesAreLeftOutOfTheSettingsFile() {
        using testing::NiceMock;
        using testing::Return;

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            EndpointSettings endpoint;
            endpoint.id = u"endpoint-1"_s;
            endpoint.endpoint = TranslationEndpoint::Google;
            endpoint.apiKey = u"google-key"_s;
            settings.translation.endpoints.push_back(endpoint);
            settings.translation.languageTool.apiKey = u"lt-key"_s;
        });

        MainWindow window;
        GMockFailures* const failures = GMockFailures::install();

        auto owned = make_unique<NiceMock<MockCredentialStore>>();
        auto* const credentials = owned.get();
        ON_CALL(*credentials, store).WillByDefault(Return(true));
        ON_CALL(*credentials, remove).WillByDefault(Return(true));
        EXPECT_CALL(*credentials, store(QByteArrayView("endpoint-1"), QByteArrayView("google-key"))).WillRepeatedly(Return(true));
        EXPECT_CALL(*credentials, store(QByteArrayView("languagetool"), QByteArrayView("lt-key"))).WillRepeatedly(Return(true));
        window.setCredentialStore(std::move(owned));

        closeProject(window);

        QVERIFY(testing::Mock::VerifyAndClearExpectations(credentials));
        QVERIFY2(failures->messages.isEmpty(), qPrintable(failures->messages.join(u'\n')));

        const QByteArray saved = readFile(env.settingsPath());
        QVERIFY(!saved.contains("google-key"));
        QVERIFY(!saved.contains("lt-key"));
        // The endpoint itself stays
        QVERIFY(saved.contains("endpoint-1"));
    }

    void anUnsupportedEndpointIsDroppedEvenWhenItsKeyCannotBeDeleted() {
        using testing::NiceMock;
        using testing::Return;

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            // A `Script` endpoint without a path is a leftover of the removed LLM backends
            EndpointSettings endpoint;
            endpoint.id = u"leftover-1"_s;
            endpoint.endpoint = TranslationEndpoint::Script;
            settings.translation.endpoints.push_back(endpoint);
        });

        GMockFailures* const failures = GMockFailures::install();

        auto owned = make_unique<NiceMock<MockCredentialStore>>();
        auto* const credentials = owned.get();
        // Any other account (the window also drops a LanguageTool key it no longer has) is not what is checked
        EXPECT_CALL(*credentials, remove).WillRepeatedly(Return(true));
        EXPECT_CALL(*credentials, remove(QByteArrayView("leftover-1"))).WillOnce(Return(false));
        EXPECT_CALL(*credentials, error()).WillOnce(Return(u"no keyring service"_s));

        // The migration runs in the constructor, so the store has to be given to it
        const MainWindow window(nullptr, std::move(owned));
        closeProject(window);

        QVERIFY(testing::Mock::VerifyAndClearExpectations(credentials));
        QVERIFY2(failures->messages.isEmpty(), qPrintable(failures->messages.join(u'\n')));
        // The endpoint goes even though its key stays in the store
        QVERIFY(savedEndpointIds(env).isEmpty());
    }

    // A key the user cleared is deleted from the store, or it would come back at the next start
    void aClearedKeyIsDeletedFromTheCredentialStoreAndARefusalIsSurvived() {
        using testing::NiceMock;
        using testing::Return;

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            EndpointSettings endpoint;
            endpoint.id = u"endpoint-2"_s;
            endpoint.endpoint = TranslationEndpoint::Google;
            settings.translation.endpoints.push_back(endpoint);
        });

        MainWindow window;
        GMockFailures* const failures = GMockFailures::install();

        auto owned = make_unique<NiceMock<MockCredentialStore>>();
        auto* const credentials = owned.get();
        EXPECT_CALL(*credentials, remove(QByteArrayView("endpoint-2"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*credentials, remove(QByteArrayView("languagetool"))).WillRepeatedly(Return(false));
        EXPECT_CALL(*credentials, error()).WillRepeatedly(Return(u"no keyring service"_s));
        EXPECT_CALL(*credentials, store).Times(0);
        window.setCredentialStore(std::move(owned));

        closeProject(window);

        QVERIFY(testing::Mock::VerifyAndClearExpectations(credentials));
        QVERIFY2(failures->messages.isEmpty(), qPrintable(failures->messages.join(u'\n')));
        // The settings were written all the same
        QVERIFY(readFile(env.settingsPath()).contains("endpoint-2"));
    }
#endif

    void theLanguageToolKeyIsKeptInTheCredentialStore() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void { settings.translation.languageTool.apiKey = u"json-key"_s; });

        {
            const MainWindow window;
            closeProject(window);
        }

        QVERIFY(!readFile(env.settingsPath()).contains("json-key"));
        QCOMPARE(storedCredential("languagetool").value_or(QString()), u"json-key"_s);

        // Not in the file any more, and loaded back from the store: saved into it again by the next start
        {
            const MainWindow window;
            closeProject(window);
        }

        QCOMPARE(storedCredential("languagetool").value_or(QString()), u"json-key"_s);

        QVERIFY(rpgm_delete_credential(strtoffi("languagetool")));
    }

    void theSettingsOfTheProjectSurviveAReopen() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Меч<#>\n");

        ProjectSettings project;
        project.columns = { { u"Source"_s, 210 }, { u"Translation"_s, 320 }, { u"Notes"_s, 150 } };
        project.completedFiles = { u"items"_s };
        project.readEncoding = u"Shift_JIS"_s;
        project.sourceDirectory = SourceDirectory::LowercaseData;
        project.engineType = EngineType::MVMZ;

        const auto json = glz::write_json(project);
        QVERIFY(json.has_value());
        writeFile(
            env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + PROJECT_SETTINGS_FILE.toString(),
            QByteArray(json->data(), static_cast<qsizetype>(json->size()))
        );
        env.writeSettings();

        {
            const MainWindow window;
            closeProject(window);
        }

        const QByteArray saved =
            readFile(env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + PROJECT_SETTINGS_FILE.toString());
        const auto reread = glz::read_json<ProjectSettings>(string_view(saved.constData(), static_cast<usize>(saved.size())));
        QVERIFY(reread.has_value());

        QCOMPARE(reread->columns.size(), usize{ 3 });
        QCOMPARE(reread->columns[2].name, u"Notes"_s);
        QCOMPARE(reread->columns[1].width, u16{ 320 });
        QCOMPARE(reread->completedFiles, QStringList{ u"items"_s });
        QCOMPARE(reread->readEncoding, u"Shift_JIS"_s);

        // What the last read used, for the project that loses its settings file
        QVERIFY(readFile(env.translationPath() + RVPACKER_METADATA_FILE.toString()).contains("Shift_JIS"));
    }

    void theFontOfTheTableComesFromTheSettings() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void { settings.appearance.translationTableFontSize = 17; });

        const MainWindow window;

        QCOMPARE(tableOf(window)->font().pointSize(), 17);
    }

    // A file that takes what is written to it and fails when it is flushed: the save opens, and only the commit refuses.
    // Linux only; a project is never opened with such a file in it, as reading it would not end.
    void theProjectStaysOpenWhileTheMapsCannotBeCommitted() {
        if (!QFileInfo::exists(u"/dev/full"_s)) {
            QSKIP("needs /dev/full");
        }

        const Env env;
        env.writeTranslation(u"maps"_s, "<!>ID<#>1\nHello<#>\n");
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings();

        const MainWindow window;
        QCOMPARE(tabsOf(window)->tabCount(), 2);

        const QString maps = env.translationFile(u"maps"_s);
        const auto restore = qScopeGuard([&env, &maps] -> void {
            QFile::remove(maps);
            env.writeTranslation(u"maps"_s, "<!>ID<#>1\nHello<#>\n");
        });
        QVERIFY(QFile::remove(maps));
        QVERIFY(QFile::link(u"/dev/full"_s, maps));

        catcher.reset();
        ModalCatcher refusal;
        closeProject(window);

        QVERIFY2(refusal.text.contains(u"Failed to save file"_qsv), qPrintable(refusal.text));
        QCOMPARE(tabsOf(window)->tabCount(), 2);
    }

    // The entry is complete in memory and fails only when it is committed: it is reported and the others are restored
    void anEntryThatFailsWhenItIsCommittedIsReportedAndTheOthersAreRestored() {
        if (!QFileInfo::exists(u"/dev/full"_s)) {
            QSKIP("needs /dev/full");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>Original\n");
        env.writeSettings();

        QDir().mkpath(backupPathOf(env));
        // `.dat` is not read when the project is opened again, which would never end on this file
        writeArchive(
            backupPathOf(env) + u"/01-01-2020_00-00-00.tar.xz"_s,
            { { u"notes.dat"_s, "note" }, { u"armors.txt"_s, "Cap<#>Шапка\n" } }
        );

        const MainWindow window;

        const QString notes = env.translationPath() + u"/notes.dat"_s;
        const auto restore = qScopeGuard([&notes] -> void { QFile::remove(notes); });
        QVERIFY(QFile::link(u"/dev/full"_s, notes));

        catcher.reset();
        ModalCatcher refusal;
        loadFirstBackup(window);

        QVERIFY2(refusal.text.contains(u"Writing entry"_qsv), qPrintable(refusal.text));
        QCOMPARE(env.readTranslation(u"armors"_s), QByteArray("Cap<#>Шапка\n"));
    }

    // Compressing takes longer than the period of the timer: the ticks in between start no second backup, which would
    // remove the half-written file of the first
    void aBackupIsNotStartedWhileThePreviousOneIsCompressing() {
        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        // Random text does not compress well: a few seconds of work for the compressor. One row, so no table to fill.
        QByteArray noise(12 * 1024 * 1024, Qt::Uninitialized);
        QRandomGenerator generator(7);
        generator.fillRange(reinterpret_cast<quint32*>(noise.data()), noise.size() / static_cast<qsizetype>(sizeof(quint32)));
        noise = "k<#>" + noise.toHex();
        env.writeTranslation(u"noise"_s, noise);

        const MainWindow window;
        QSignalSpy messages(window.statusBar(), &QStatusBar::messageChanged);

        QTRY_VERIFY_WITH_TIMEOUT(!backupsOf(env).isEmpty(), 120000);

        QCOMPARE(readArchive(backupPathOf(env) + u'/' + backupsOf(env).first()).value(u"noise.txt"_s), noise);

        for (const QList<QVariant>& message : messages) {
            QVERIFY2(!message.first().toString().contains(u"Failed to create the backup"_qsv), qPrintable(message.first().toString()));
        }
    }

    void noBackupIsMadeWhileTheOpenTabCannotBeSaved() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const Env env;
        env.writeTranslation(u"items"_s, "Sword<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            settings.core.backup.enabled = true;
            settings.core.backup.period = 1;
        });

        const MainWindow window;
        tabsOf(window)->changeTab(u"items"_s);
        edit(window, 0, 1, u"Меч"_s);
        makeReadOnly(env.translationFile(u"items"_s));
        const auto restore = qScopeGuard([&env] -> void { makeWritable(env.translationFile(u"items"_s)); });

        catcher.reset();
        ModalCatcher refusal;
        refusal.press = u"Abort"_s;

        // Two ticks of the timer, each of which is refused by the tab
        QTRY_VERIFY_WITH_TIMEOUT(refusal.seen.size() >= 2, 10000);

        QVERIFY(backupsOf(env).isEmpty());
        QCOMPARE(*tableOf(window)->model()->item(0, 1), u"Меч"_s);
        QCOMPARE(env.readTranslation(u"items"_s), QByteArray("Sword<#>\n"));
    }

   private:
    unique_ptr<ModalCatcher> catcher;
};

QTEST_MAIN(TestMainWindowSaveLoad)
#include "tst_mainwindow_saveload.moc"
