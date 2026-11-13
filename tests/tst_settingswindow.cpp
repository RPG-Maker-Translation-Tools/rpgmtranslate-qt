// The settings window shows the settings, and writes them back when it is accepted. What it refuses to write is as
// much part of it as what it writes.
#include "Permissions.hpp"
#include "HttpStub.hpp"
#include "MarkingTranslator.hpp"
#include "ModalCatcher.hpp"
#ifdef HAVE_GMOCK
#include "MockCredentialStore.hpp"
#endif
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "SettingsWindow.hpp"
#include "TestFiles.hpp"

#include <QSslSocket>
#include <QCheckBox>
#include <QFontComboBox>
#include <QStackedWidget>
#include <QComboBox>
#include <QLabel>
#include <QListView>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QPushButton>
#include <QTemporaryFile>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTableWidget>
#include <QtTest>

namespace {
template <typename Widget>
[[nodiscard]] auto field(const SettingsWindow& window, const char* const name) -> Widget* {
    return window.findChild<Widget*>(QString::fromLatin1(name));
}
}  // namespace

class TestSettingsWindow final : public QObject {
    Q_OBJECT

    Settings settings;
    ProjectSettings project;
    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void init() {
        settings = Settings();
        project = ProjectSettings();
        catcher = make_unique<ModalCatcher>();
    }

    void cleanup() {
        QVERIFY2(catcher->text.isEmpty(), qPrintable(catcher->text));
        catcher.reset();
    }

    void showsTheSettingsItHolds() {
        settings.core.backup.enabled = true;
        settings.core.backup.period = 120;
        settings.core.backup.max = 7;
        settings.core.git.username = u"savann"_s;
        settings.core.checkForAppUpdates = false;
        settings.appearance.displayPercents = true;
        settings.controls.goToRow = u"Ctrl+J"_s;
        settings.translation.displayWordsAndCharacters = true;
        project.lineLengthHint = 42;

        const SettingsWindow window(&settings, &project, nullptr);

        QVERIFY(field<QCheckBox>(window, "backupCheckbox")->isChecked());
        QCOMPARE(field<QLineEdit>(window, "backupPeriodInput")->text(), u"120"_s);
        QCOMPARE(field<QLineEdit>(window, "maxBackupsInput")->text(), u"7"_s);
        QCOMPARE(field<QLineEdit>(window, "gitUsernameInput")->text(), u"savann"_s);
        QVERIFY(!field<QCheckBox>(window, "updatesCheckbox")->isChecked());
        QVERIFY(field<QCheckBox>(window, "displayPercentsCheckbox")->isChecked());
        QVERIFY(field<QCheckBox>(window, "displayWordsCheckbox")->isChecked());
        QCOMPARE(field<QKeySequenceEdit>(window, "goToRowInput")->keySequence(), QKeySequence(u"Ctrl+J"_s));
        QCOMPARE(field<QLineEdit>(window, "lineLengthHintInput")->text(), u"42"_s);
    }

    void aBackupPeriodOutsideTheLimitsIsPulledIntoThem() {
        settings.core.backup.period = 1;
        settings.core.backup.max = 250;

        const SettingsWindow window(&settings, &project, nullptr);

        QCOMPARE(settings.core.backup.period, static_cast<u16>(MIN_BACKUP_PERIOD));
        QCOMPARE(settings.core.backup.max, static_cast<u8>(MAX_BACKUPS));
    }

    void acceptingWritesTheEditsBack() {
        SettingsWindow window(&settings, &project, nullptr);

        field<QCheckBox>(window, "backupCheckbox")->setChecked(true);
        field<QLineEdit>(window, "gitUsernameInput")->setText(u"someone"_s);
        field<QLineEdit>(window, "backupPeriodInput")->setText(u"300"_s);
        field<QLineEdit>(window, "lineLengthHintInput")->setText(u"55"_s);
        field<QKeySequenceEdit>(window, "goToRowInput")->setKeySequence(QKeySequence(u"Ctrl+J"_s));

        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 1);
        QVERIFY(settings.core.backup.enabled);
        QCOMPARE(settings.core.git.username, u"someone"_s);
        QCOMPARE(settings.core.backup.period, u16{ 300 });
        QCOMPARE(project.lineLengthHint, u16{ 55 });
        QCOMPARE(settings.controls.goToRow, u"Ctrl+J"_s);
    }

    // A language the list does not know (written by hand, or by another version) is not saved over with nothing
    void aLanguageTheListDoesNotKnowIsKeptThroughASave() {
        project.sourceLang = u"tlh"_s;
        project.translationLang = u"tlh-x"_s;

        SettingsWindow window(&settings, &project, nullptr);
        window.accept();

        QCOMPARE(project.sourceLang, u"tlh"_s);
        QCOMPARE(project.translationLang, u"tlh-x"_s);
    }

    void rejectingLeavesTheSettingsAsTheyWere() {
        settings.core.git.username = u"before"_s;
        settings.core.checkForAppUpdates = true;

        SettingsWindow window(&settings, &project, nullptr);
        field<QLineEdit>(window, "gitUsernameInput")->setText(u"after"_s);
        field<QCheckBox>(window, "updatesCheckbox")->setChecked(false);

        window.reject();

        QCOMPARE(settings.core.git.username, u"before"_s);
        QVERIFY(settings.core.checkForAppUpdates);
    }

    void aValueOutsideItsLimitsKeepsTheWindowOpen_data() {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QString>("value");
        QTest::addColumn<QString>("message");

        QTest::newRow("backup period below the limit") << "backupPeriodInput"_ba << u"0"_s << u"Backup period must be between"_s;
        QTest::newRow("no backups at all") << "maxBackupsInput"_ba << u"0"_s << u"Max backups must be between"_s;
        QTest::newRow("line length hint") << "lineLengthHintInput"_ba << u"99999"_s << u"Line length hint must be between"_s;
        QTest::newRow("hint font size") << "hintFontSizeInput"_ba << u"99999"_s << u"Hint font size must be between"_s;
    }

    void aValueOutsideItsLimitsKeepsTheWindowOpen() {
        QFETCH(const QByteArray, input);
        QFETCH(const QString, value);
        QFETCH(const QString, message);

        SettingsWindow window(&settings, &project, nullptr);
        field<QLineEdit>(window, input.constData())->setText(value);

        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 0);
        QVERIFY(catcher->text.contains(message));
        catcher->text.clear();
    }

    void withoutAProjectItsSettingsAreNotChecked() {
        SettingsWindow window(&settings, nullptr, nullptr);

        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 1);
    }

    void aCustomLintThatIsNotARegularExpressionKeepsTheWindowOpen() {
        settings.translation.customLints = { CustomLint{ .sequence = u"fine"_s }, CustomLint{ .sequence = u"("_s } };

        SettingsWindow window(&settings, &project, nullptr);
        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 0);
        QVERIFY(catcher->text.contains(u"is not a valid regular expression"_qsv));
        catcher->text.clear();
        // Nothing was written: the pattern that would silence every lint did not get in
        QCOMPARE(settings.translation.customLints.size(), usize{ 2 });
    }

    void customLintsKeepTheirDisabledStateAndLoseTheEmptyOnes() {
        settings.translation.customLints = { CustomLint{ .sequence = u"kept"_s, .tooltip = u"hint"_s, .disabled = true },
                                             CustomLint{ .sequence = u"dropped"_s } };

        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");
        QCOMPARE(table->rowCount(), 2);
        table->item(1, 0)->setText(QString());
        table->item(0, 1)->setText(u"new hint"_s);

        window.accept();

        QCOMPARE(settings.translation.customLints.size(), usize{ 1 });
        QCOMPARE(settings.translation.customLints[0].sequence, u"kept"_s);
        QCOMPARE(settings.translation.customLints[0].tooltip, u"new hint"_s);
        // The state lives in the lint selection, not in this window
        QVERIFY(settings.translation.customLints[0].disabled);
    }

    void replacementsWithoutASourceAreDropped() {
        settings.translation.replacements = { Replacement(u"<<"_s, u"«"_s, true),
                                              Replacement(u"--"_s, u"—"_s, false) };

        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "replacementsTable");
        QCOMPARE(table->rowCount(), 2);
        table->item(1, 0)->setText(QString());
        table->item(0, 2)->setCheckState(Qt::Unchecked);

        window.accept();

        QCOMPARE(settings.translation.replacements.size(), usize{ 1 });
        QCOMPARE(settings.translation.replacements[0].source, u"<<"_s);
        QCOMPARE(settings.translation.replacements[0].replacement, u"«"_s);
        QVERIFY(!settings.translation.replacements[0].enabled);
    }

    void replacementsAreAddedAndRemovedWithTheirButtons() {
        settings.translation.replacements.clear();

        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "replacementsTable");
        auto* const add = field<QPushButton>(window, "addReplacementButton");
        auto* const remove = field<QPushButton>(window, "removeReplacementButton");
        QCOMPARE(table->rowCount(), 0);

        add->click();
        add->click();
        QCOMPARE(table->rowCount(), 2);
        // A replacement that was just added does not act until it is switched on
        QCOMPARE(table->item(1, 2)->checkState(), Qt::Unchecked);

        table->item(0, 0)->setText(u"a"_s);
        table->item(0, 1)->setText(u"b"_s);
        table->item(1, 0)->setText(u"c"_s);
        table->item(1, 1)->setText(u"d"_s);

        // Nothing is selected, so nothing is removed
        table->setCurrentCell(-1, -1);
        remove->click();
        QCOMPARE(table->rowCount(), 2);

        table->setCurrentCell(0, 0);
        remove->click();
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text(), u"c"_s);

        window.accept();

        QCOMPARE(settings.translation.replacements.size(), usize{ 1 });
        QCOMPARE(settings.translation.replacements[0].source, u"c"_s);
        QCOMPARE(settings.translation.replacements[0].replacement, u"d"_s);
    }

    void customLintsAreAddedAndRemovedWithTheirButtons() {
        settings.translation.customLints.clear();

        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");
        auto* const add = field<QPushButton>(window, "addLintButton");
        auto* const remove = field<QPushButton>(window, "removeLintButton");
        QCOMPARE(table->rowCount(), 0);

        add->click();
        add->click();
        QCOMPARE(table->rowCount(), 2);

        table->setCurrentCell(-1, -1);
        remove->click();
        QCOMPARE(table->rowCount(), 2);

        table->setCurrentCell(1, 0);
        remove->click();
        QCOMPARE(table->rowCount(), 1);
    }

    void whitespaceCharactersAreReadAsCodePoints_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QList<uint>>("expected");

        QTest::newRow("with and without a prefix") << u"U+3000, 0x00a0, 2003"_s << QList<uint>{ 0x3000, 0xA0, 0x2003 };
        QTest::newRow("the prefix in any case") << u"u+41,0X42"_s << QList<uint>{ 0x41, 0x42 };
        QTest::newRow("what is not a number is skipped") << u"zz, U+, , 41, 12g"_s << QList<uint>{ 0x41 };
        QTest::newRow("nothing") << QString() << QList<uint>{};
    }

    void whitespaceCharactersAreReadAsCodePoints() {
        QFETCH(const QString, text);
        QFETCH(const QList<uint>, expected);

        settings.translation.whitespaceCharacters = { 0x20 };

        {
            SettingsWindow window(&settings, &project, nullptr);
            field<QLineEdit>(window, "whitespaceCharactersInput")->setText(text);
            window.accept();
        }

        QCOMPARE(QList<uint>(settings.translation.whitespaceCharacters.begin(), settings.translation.whitespaceCharacters.end()), expected);

        // They come back as the same code points, written the way the window writes them
        SettingsWindow reopened(&settings, &project, nullptr);
        const QString shown = field<QLineEdit>(reopened, "whitespaceCharactersInput")->text();
        QCOMPARE(shown.isEmpty(), expected.isEmpty());

        if (!expected.isEmpty()) {
            QVERIFY2(shown.startsWith(u"U+"_qsv), qPrintable(shown));
        }
    }

    void aReservedHotkeyIsRefused() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QKeySequenceEdit>(window, "goToRowInput")->setKeySequence(QKeySequence(QKeySequence::Save));

        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 0);
        QVERIFY(catcher->text.contains(u"is reserved for"_qsv));
        catcher->text.clear();
    }

    void aHotkeyGivenToTwoThingsIsRefused() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QKeySequenceEdit>(window, "goToRowInput")->setKeySequence(QKeySequence(u"Ctrl+K"_s));
        field<QKeySequenceEdit>(window, "batchMenuInput")->setKeySequence(QKeySequence(u"Ctrl+K"_s));

        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 0);
        QVERIFY(catcher->text.contains(u"is already assigned to both"_qsv));
        catcher->text.clear();
    }

    void aScriptEndpointWithoutAScriptIsRefused() {
        settings.translation.endpoints = { EndpointSettings{ .id = u"id1"_s,
                                                              .name = u"My script"_s,
                                                              .endpoint = TranslationEndpoint::Script } };

        SettingsWindow window(&settings, &project, nullptr);
        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 0);
        QVERIFY(catcher->text.contains(u"needs a script file"_qsv));
        catcher->text.clear();
    }

    void anAddedEndpointIsNamedAndSelected() {
        SettingsWindow window(&settings, &project, nullptr);

        field<QPushButton>(window, "addEndpointButton")->click();

        QCOMPARE(settings.translation.endpoints.size(), usize{ 1 });
        QVERIFY(!settings.translation.endpoints[0].id.isEmpty());
        QVERIFY(!settings.translation.endpoints[0].name.isEmpty());
        QVERIFY(field<QListView>(window, "endpointList")->currentIndex().isValid());
        QVERIFY(field<QLineEdit>(window, "apiKeyInput")->isEnabled());
    }

    void theFormOfAnEndpointGoesToItOnAccept() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();

        field<QComboBox>(window, "typeSelect")->setCurrentIndex(static_cast<i32>(TranslationEndpoint::Yandex));
        field<QLineEdit>(window, "apiKeyInput")->setText(u"secret"_s);
        field<QLineEdit>(window, "yandexFolderIdInput")->setText(u"folder"_s);
        window.accept();

        QCOMPARE(settings.translation.endpoints.size(), usize{ 1 });
        const EndpointSettings& endpoint = settings.translation.endpoints[0];
        QCOMPARE(endpoint.endpoint, TranslationEndpoint::Yandex);
        QCOMPARE(endpoint.apiKey, u"secret"_s);
        QCOMPARE(endpoint.yandexFolderId, u"folder"_s);
    }

    void switchingBetweenEndpointsKeepsTheEditsOfEach() {
        SettingsWindow window(&settings, &project, nullptr);
        auto* const list = field<QListView>(window, "endpointList");

        field<QPushButton>(window, "addEndpointButton")->click();
        field<QLineEdit>(window, "apiKeyInput")->setText(u"first key"_s);

        field<QPushButton>(window, "addEndpointButton")->click();
        QCOMPARE(field<QLineEdit>(window, "apiKeyInput")->text(), QString());
        field<QLineEdit>(window, "apiKeyInput")->setText(u"second key"_s);

        list->setCurrentIndex(list->model()->index(0, 0));
        QCOMPARE(field<QLineEdit>(window, "apiKeyInput")->text(), u"first key"_s);

        window.accept();
        QCOMPARE(settings.translation.endpoints[0].apiKey, u"first key"_s);
        QCOMPARE(settings.translation.endpoints[1].apiKey, u"second key"_s);
    }

    void aRemovedEndpointIsGoneOnAccept() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();
        field<QPushButton>(window, "addEndpointButton")->click();
        const QString kept = settings.translation.endpoints[1].id;

        field<QListView>(window, "endpointList")->setCurrentIndex(field<QListView>(window, "endpointList")->model()->index(0, 0));
        field<QPushButton>(window, "removeEndpointButton")->click();
        QVERIFY(!field<QLineEdit>(window, "apiKeyInput")->isEnabled());
        window.accept();

        QCOMPARE(settings.translation.endpoints.size(), usize{ 1 });
        QCOMPARE(settings.translation.endpoints[0].id, kept);
    }

#ifdef HAVE_GMOCK
    // The key of a removed endpoint is deleted when the changes are saved; a store that refuses one does not keep the
    // others from being deleted
    void theKeysOfRemovedEndpointsAreDeletedAndARefusalDoesNotStopTheRest() {
        using testing::NiceMock;
        using testing::Return;

        SettingsWindow window(&settings, &project, nullptr);
        GMockFailures* const failures = GMockFailures::install();

        field<QPushButton>(window, "addEndpointButton")->click();
        field<QPushButton>(window, "addEndpointButton")->click();
        const QByteArray first = settings.translation.endpoints[0].id.toUtf8();
        const QByteArray second = settings.translation.endpoints[1].id.toUtf8();

        auto owned = make_unique<NiceMock<MockCredentialStore>>();
        auto* const credentials = owned.get();
        EXPECT_CALL(*credentials, remove(QByteArrayView(first))).WillOnce(Return(false));
        EXPECT_CALL(*credentials, remove(QByteArrayView(second))).WillOnce(Return(true));
        EXPECT_CALL(*credentials, error()).WillRepeatedly(Return(u"no keyring service"_s));
        window.setCredentialStore(std::move(owned));

        auto* const list = field<QListView>(window, "endpointList");

        for (i32 removed = 0; removed < 2; removed++) {
            list->setCurrentIndex(list->model()->index(0, 0));
            field<QPushButton>(window, "removeEndpointButton")->click();
        }

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to delete the API key of a removed endpoint: no keyring service"_s));
        window.accept();

        QVERIFY(testing::Mock::VerifyAndClearExpectations(credentials));
        QVERIFY2(failures->messages.isEmpty(), qPrintable(failures->messages.join(u'\n')));
        QVERIFY(settings.translation.endpoints.empty());
    }
#endif

    void aScriptEndpointAsksToApproveItsScriptOnAccept() {
        QTemporaryFile script;
        QVERIFY(script.open());
        script.write("function rpgmTranslateCallback(text) { return text; }");
        script.flush();

        settings.translation.endpoints = { EndpointSettings{ .id = u"id1"_s,
                                                              .name = u"My script"_s,
                                                              .scriptPath = script.fileName(),
                                                              .endpoint = TranslationEndpoint::Script } };
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        SettingsWindow window(&settings, &project, nullptr);
        window.accept();

        QVERIFY(catcher->text.contains(u"can run programs"_qsv));
        QCOMPARE(settings.core.approvedScriptsSha.size(), qsizetype{ 1 });
        catcher->text.clear();
    }

    void aScriptThatCannotBeReadIsReportedOnAccept() {
        settings.translation.endpoints = { EndpointSettings{ .id = u"id1"_s,
                                                              .name = u"My script"_s,
                                                              .scriptPath = u"/no/such/script.js"_s,
                                                              .endpoint = TranslationEndpoint::Script } };

        SettingsWindow window(&settings, &project, nullptr);
        window.accept();

        QVERIFY(catcher->text.contains(u"Cannot read script"_qsv));
        QCOMPARE(settings.core.approvedScriptsSha.size(), qsizetype{ 0 });
        catcher->text.clear();
    }

    void theScriptIsChosenInAFileDialog() {
        const QTemporaryDir dir;
        const QString script = dir.path() + u"/translate.js"_s;
        writeFile(script, "function rpgmTranslateCallback(text) { return text; }");

        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();
        catcher->file = script;

        field<QPushButton>(window, "browseScriptButton")->click();

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QCOMPARE(field<QLineEdit>(window, "scriptPathInput")->text(), script);
    }

    void aCancelledFileDialogKeepsTheScriptThatWasThere() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();
        field<QLineEdit>(window, "scriptPathInput")->setText(u"/old/script.js"_s);

        field<QPushButton>(window, "browseScriptButton")->click();

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QCOMPARE(field<QLineEdit>(window, "scriptPathInput")->text(), u"/old/script.js"_s);
    }

    void aColorIsChosenForACustomLintByDoubleClickingItsCell() {
        settings.translation.customLints = { CustomLint{ .sequence = u"fine"_s } };
        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");
        catcher->color = QColor(255, 0, 0);

        // Any other cell of the row is edited, not colored
        emit table->itemDoubleClicked(table->item(0, 0));
        QVERIFY(catcher->dialogs.isEmpty());

        emit table->itemDoubleClicked(table->item(0, 4));

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QCOMPARE(table->item(0, 4)->background().color(), QColor(255, 0, 0));

        window.accept();
        QCOMPARE(QColor::fromRgba(settings.translation.customLints[0].customColor), QColor(255, 0, 0));
    }

    void aCancelledColorDialogKeepsTheColor() {
        settings.translation.customLints = { CustomLint{ .sequence = u"fine"_s } };
        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");
        const QColor before = table->item(0, 4)->background().color();

        emit table->itemDoubleClicked(table->item(0, 4));

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QCOMPARE(table->item(0, 4)->background().color(), before);
    }

    void aColorIsChosenForASequenceByDoubleClickingItsCell() {
        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "textCodesTable");
        QVERIFY(table->rowCount() > 0);
        catcher->color = QColor(0, 128, 255);

        emit table->itemDoubleClicked(table->item(0, 0));
        QVERIFY(catcher->dialogs.isEmpty());

        emit table->itemDoubleClicked(table->item(0, 3));

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QCOMPARE(table->item(0, 3)->background().color(), QColor(0, 128, 255));
    }

    void anInvalidPatternTypedIntoACustomLintIsReported() {
        settings.translation.customLints = { CustomLint{ .sequence = u"fine"_s } };
        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");

        table->item(0, 0)->setText(u"("_s);

        QVERIFY(catcher->text.contains(u"Regular expression is invalid"_qsv));
        catcher->text.clear();

        // Another column is not a pattern
        table->item(0, 1)->setText(u"("_s);
        QVERIFY(catcher->text.isEmpty());
    }

    void lintsAreExportedToAndImportedFromAFile() {
        const QTemporaryDir dir;
        const QString path = dir.path() + u"/lints.json"_s;
        settings.translation.customLints = { CustomLint{ .sequence = u"first"_s, .tooltip = u"one"_s },
                                             CustomLint{ .sequence = u"second\\d"_s, .tooltip = u"two"_s } };

        {
            SettingsWindow window(&settings, &project, nullptr);
            catcher->file = path;
            field<QPushButton>(window, "exportLintsButton")->click();
        }

        const QByteArray exported = readFile(path);
        QVERIFY(exported.contains("first"));
        QVERIFY(exported.contains("two"));

        settings.translation.customLints.clear();
        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");
        QCOMPARE(table->rowCount(), 0);

        field<QPushButton>(window, "importLintsButton")->click();

        QCOMPARE(table->rowCount(), 2);
        QCOMPARE(table->item(0, 0)->text(), u"first"_s);
        QCOMPARE(table->item(1, 1)->text(), u"two"_s);
    }

    void cancellingTheExportOrImportDoesNothing() {
        SettingsWindow window(&settings, &project, nullptr);
        auto* const table = field<QTableWidget>(window, "customLintsTable");

        field<QPushButton>(window, "exportLintsButton")->click();
        field<QPushButton>(window, "importLintsButton")->click();

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 2 });
        QCOMPARE(table->rowCount(), 0);
    }

    void anImportOfLintsWithBadPatternsSkipsThemAndSaysSo() {
        const QTemporaryDir dir;
        const QString path = dir.path() + u"/lints.json"_s;
        // moc cannot read a raw string literal that holds quotes, so this one is escaped
        writeFile(path, "[{\"sequence\":\"good\"},{\"sequence\":\"(\"},{\"sequence\":\"also\\\\d\"}]");

        SettingsWindow window(&settings, &project, nullptr);
        catcher->file = path;

        field<QPushButton>(window, "importLintsButton")->click();

        auto* const table = field<QTableWidget>(window, "customLintsTable");
        QCOMPARE(table->rowCount(), 2);
        QVERIFY(catcher->text.contains(u"1 imported custom lint(s) were skipped"_qsv));
        catcher->text.clear();
    }

    void anImportOfSomethingThatIsNotLintsIsReported() {
        const QTemporaryDir dir;
        const QString path = dir.path() + u"/lints.json"_s;
        writeFile(path, "this is not json");

        SettingsWindow window(&settings, &project, nullptr);
        catcher->file = path;

        field<QPushButton>(window, "importLintsButton")->click();

        QCOMPARE(field<QTableWidget>(window, "customLintsTable")->rowCount(), 0);
        QVERIFY(catcher->text.contains(u"Parsing"_qsv));
        catcher->text.clear();
    }

    void anExportThatCannotBeWrittenIsReported() {
        const QTemporaryDir dir;

        SettingsWindow window(&settings, &project, nullptr);
        // A folder where the file should go
        QVERIFY(QDir(dir.path()).mkdir(u"taken.json"_s));
        catcher->file = dir.path() + u"/taken.json"_s;

        field<QPushButton>(window, "exportLintsButton")->click();

        QVERIFY(!catcher->text.isEmpty());
        QVERIFY(!QFileInfo(dir.path() + u"/taken.json"_s).isFile());
        catcher->text.clear();
    }

    void aScriptEndpointFailsTheKeyCheckAndTheButtonComesBack() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();
        field<QComboBox>(window, "typeSelect")->setCurrentIndex(static_cast<i32>(TranslationEndpoint::Script));

        auto* const button = field<QPushButton>(window, "checkKeyButton");
        button->click();
        QVERIFY(!button->isEnabled());

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"API key check failed"_qsv), 10000);
        QVERIFY(button->isEnabled());
        catcher->text.clear();
    }

    void aKeyThatTheServerAcceptsWorks() {
        const HttpStub server([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("200 OK", R"({"translations":[{"text":"Hallo"}]})");
        });
        settings.translation.endpoints = { EndpointSettings{ .id = u"id"_s,
                                                             .name = u"yandex"_s,
                                                             .apiKey = u"key"_s,
                                                             .yandexFolderId = u"folder"_s,
                                                             .yandexBaseUrl = server.url(),
                                                             .endpoint = TranslationEndpoint::Yandex } };

        SettingsWindow window(&settings, &project, nullptr);
        auto* const list = field<QListView>(window, "endpointList");
        list->setCurrentIndex(list->model()->index(0, 0));

        auto* const button = field<QPushButton>(window, "checkKeyButton");
        button->click();
        QVERIFY(!button->isEnabled());

        // The notice is a status one, which a dialog without a status bar shows as a box
        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"The API key works"_qsv), 10000);
        QVERIFY(button->isEnabled());
        QVERIFY(!server.requests.isEmpty());
        catcher->text.clear();
    }

    void aKeyThatTheServerRefusesIsReported() {
        const HttpStub server([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("403 Forbidden", R"({"message":"bad key"})");
        });
        settings.translation.endpoints = { EndpointSettings{ .id = u"id"_s,
                                                             .name = u"yandex"_s,
                                                             .apiKey = u"key"_s,
                                                             .yandexFolderId = u"folder"_s,
                                                             .yandexBaseUrl = server.url(),
                                                             .endpoint = TranslationEndpoint::Yandex } };

        SettingsWindow window(&settings, &project, nullptr);
        auto* const list = field<QListView>(window, "endpointList");
        list->setCurrentIndex(list->model()->index(0, 0));

        field<QPushButton>(window, "checkKeyButton")->click();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"API key check failed"_qsv), 10000);
        catcher->text.clear();
    }

    void savingThroughTheWindowKeepsWhatTheFormDoesNotShow() {
        settings.translation.endpoints = { EndpointSettings{ .id = u"id"_s,
                                                             .name = u"yandex"_s,
                                                             .apiKey = u"key"_s,
                                                             .yandexFolderId = u"folder"_s,
                                                             .yandexBaseUrl = u"http://127.0.0.1:1"_s,
                                                             .endpoint = TranslationEndpoint::Yandex } };

        SettingsWindow window(&settings, &project, nullptr);
        auto* const list = field<QListView>(window, "endpointList");
        list->setCurrentIndex(list->model()->index(0, 0));
        field<QLineEdit>(window, "apiKeyInput")->setText(u"changed"_s);
        window.accept();

        QCOMPARE(settings.translation.endpoints[0].apiKey, u"changed"_s);
        QCOMPARE(settings.translation.endpoints[0].yandexBaseUrl, u"http://127.0.0.1:1"_s);
    }

    void aLanguageToolServerThatAnswersIsConnectedTo() {
        HttpStub server([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("200 OK", "{}");
        });

        SettingsWindow window(&settings, &project, nullptr);
        // Trailing slashes are not part of the address
        field<QLineEdit>(window, "ltBaseUrlInput")->setText(server.url() + u"//"_s);

        field<QPushButton>(window, "ltCheckConnectionButton")->click();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Successfully connected"_qsv), 10000);
        QVERIFY(server.requests.first().startsWith(u"GET /v2"_qsv));
        catcher->text.clear();
    }

    void aLanguageToolServerThatIsNotThereIsReported() {
        QString dead;
        {
            const HttpStub gone([](const QString&, const QString&, const QByteArray&) -> QByteArray { return {}; });
            dead = gone.url();
        }

        SettingsWindow window(&settings, &project, nullptr);
        window.languageToolTimeoutMs = 2000;
        field<QLineEdit>(window, "ltBaseUrlInput")->setText(dead);

        field<QPushButton>(window, "ltCheckConnectionButton")->click();

        // A refused connection is reported at once on Linux, and on Windows when the check gives up
        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"connection check failed"_qsv), 15000);
        QVERIFY(!catcher->text.contains(u"HTTPS"_qsv));
        catcher->text.clear();
    }

    void aLanguageToolServerThatNeverAnswersGetsTheHttpsHintOnlyForHttps_data() {
        QTest::addColumn<QString>("scheme");
        QTest::addColumn<bool>("hint");
        QTest::newRow("https") << u"https"_s << true;
        QTest::newRow("http") << u"http"_s << false;
    }

    void aLanguageToolServerThatNeverAnswersGetsTheHttpsHintOnlyForHttps() {
        QFETCH(QString, scheme);
        QFETCH(bool, hint);

        if (hint && !QSslSocket::supportsSsl()) {
            QSKIP("TLS is not available");
        }

        const HttpStub silent([](const QString&, const QString&, const QByteArray&) -> QByteArray { return {}; });
        SettingsWindow window(&settings, &project, nullptr);
        window.languageToolTimeoutMs = 200;
        field<QLineEdit>(window, "ltBaseUrlInput")
            ->setText(scheme + u"://127.0.0.1:"_s + silent.url().section(u':', -1));

        field<QPushButton>(window, "ltCheckConnectionButton")->click();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"connection check failed"_qsv), 10000);
        QCOMPARE(catcher->text.contains(u"HTTPS"_qsv), hint);
        catcher->text.clear();
    }

    void everyEndpointTypeDescribesItselfAndShowsItsFields() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();
        auto* const type = field<QComboBox>(window, "typeSelect");
        const auto* const description = field<QLabel>(window, "typeDescriptionLabel");

        type->setCurrentIndex(static_cast<i32>(TranslationEndpoint::Yandex));
        QVERIFY(description->text().contains(u"Yandex"_qsv));
        QVERIFY(!field<QWidget>(window, "yandexFolderWidget")->isHidden());
        QVERIFY(field<QWidget>(window, "scriptPathWidget")->isHidden());

        type->setCurrentIndex(static_cast<i32>(TranslationEndpoint::DeepL));
        QVERIFY(description->text().contains(u"DeepL"_qsv));
        QVERIFY(field<QWidget>(window, "yandexFolderWidget")->isHidden());

        type->setCurrentIndex(static_cast<i32>(TranslationEndpoint::Script));
        QVERIFY(description->text().contains(u"Script"_qsv));
        QVERIFY(!field<QWidget>(window, "scriptPathWidget")->isHidden());
        QVERIFY(field<QWidget>(window, "apiKeyWidget")->isHidden());

        type->setCurrentIndex(static_cast<i32>(TranslationEndpoint::Google));
        QVERIFY(description->text().contains(u"Google"_qsv));
        QVERIFY(field<QWidget>(window, "scriptPathWidget")->isHidden());
    }

    // The theme is applied the same way, but the offscreen platform has no color scheme to change
    void theStyleIsAppliedAtOnce() {
        const QString styleBefore = qApp->style()->name();

        {
            SettingsWindow window(&settings, &project, nullptr);

            // One that is not the style already, or the selection changes nothing and applies nothing
            auto* const select = field<QComboBox>(window, "styleSelect");
            const qsizetype other = [&]() -> qsizetype {
                for (qsizetype index = 0; index < select->count(); index++) {
                    if (select->itemText(index).compare(styleBefore, Qt::CaseInsensitive) != 0) {
                        return index;
                    }
                }

                return -1;
            }();
            QVERIFY(other >= 0);

            select->setCurrentIndex(static_cast<i32>(other));
            QCOMPARE(qApp->style()->name(), select->itemText(static_cast<i32>(other)).toLower());
        }

        qApp->setStyle(QStyleFactory::create(styleBefore));
    }

    void aDictionaryWithoutItsDicFileIsRefused() {
        const QTemporaryDir data;
        QVERIFY(QDir().mkpath(data.path() + u"/dictionaries"_s));
        writeFile(data.path() + u"/dictionaries/de_DE.aff"_s, "SET UTF-8");
        writeFile(data.path() + u"/dictionaries/de_DE.dic"_s, "0");
        writeFile(data.path() + u"/dictionaries/en_US.aff"_s, "SET UTF-8");
        const QVariant dataBefore = qApp->property("data-location");
        qApp->setProperty("data-location", data.path());

        {
            SettingsWindow window(&settings, &project, nullptr);
            auto* const select = field<QComboBox>(window, "spellcheckDictionarySelect");
            QCOMPARE(select->count(), 3);

            select->setCurrentText(u"/de_DE.aff"_s);
            QVERIFY(catcher->text.isEmpty());
            QCOMPARE(select->currentText(), u"/de_DE.aff"_s);

            select->setCurrentText(u"/en_US.aff"_s);
            QVERIFY(catcher->text.contains(u".dic"_qsv));
            QCOMPARE(select->currentIndex(), 0);
            catcher->text.clear();

            // Opening the list looks at the folder again
            writeFile(data.path() + u"/dictionaries/fr_FR.aff"_s, "SET UTF-8");
            QKeyEvent f4(QEvent::KeyPress, Qt::Key_F4, Qt::NoModifier);
            QCoreApplication::sendEvent(select, &f4);
            select->hidePopup();
            QCOMPARE(select->count(), 4);

            // So does pressing the box with the mouse
            writeFile(data.path() + u"/dictionaries/es_ES.aff"_s, "SET UTF-8");
            QTest::mousePress(select, Qt::LeftButton);
            QTest::mouseRelease(select, Qt::LeftButton);
            select->hidePopup();
            QCOMPARE(select->count(), 5);

            // Alt+Down opens it too; Down alone is the box moving to the next entry
            writeFile(data.path() + u"/dictionaries/it_IT.aff"_s, "SET UTF-8");
            QKeyEvent down(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
            QCoreApplication::sendEvent(select, &down);
            QCOMPARE(select->count(), 5);
            QKeyEvent altDown(QEvent::KeyPress, Qt::Key_Down, Qt::AltModifier);
            QCoreApplication::sendEvent(select, &altDown);
            select->hidePopup();
            QCOMPARE(select->count(), 6);
        }

        qApp->setProperty("data-location", dataBefore);
    }
    void theStyleAndThemeFollowTheBoxesAndComeBackWhenTheWindowIsRejected() {
        const QString styleBefore = qApp->style()->name();
        settings.appearance.style = styleBefore;

        {
            SettingsWindow window(&settings, &project, nullptr);
            auto* const select = field<QComboBox>(window, "styleSelect");

            QString other;

            for (i32 index = 0; index < select->count() && other.isEmpty(); index++) {
                if (select->itemText(index).compare(styleBefore, Qt::CaseInsensitive) != 0) {
                    other = select->itemText(index);
                }
            }

            QVERIFY(!other.isEmpty());
            select->setCurrentText(other);

            field<QComboBox>(window, "themeSelect")->setCurrentIndex(1);

            window.reject();
            QCOMPARE(qApp->style()->name(), styleBefore);
        }

        qApp->styleHints()->unsetColorScheme();
    }

    void pickingASectionShowsItsPage() {
        SettingsWindow window(&settings, &project, nullptr);
        auto* const sections = field<QListView>(window, "sectionsList");
        auto* const pages = field<QStackedWidget>(window, "stackedWidget");

        sections->setCurrentIndex(sections->model()->index(2, 0));
        QCOMPARE(pages->currentIndex(), 2);
        sections->setCurrentIndex(sections->model()->index(4, 0));
        QCOMPARE(pages->currentIndex(), 4);
    }

    void removingNothingKeepsTheEndpoints() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();

        auto* const list = field<QListView>(window, "endpointList");
        list->setCurrentIndex(QModelIndex());
        field<QPushButton>(window, "removeEndpointButton")->click();
        window.accept();

        QCOMPARE(settings.translation.endpoints.size(), usize{ 1 });
    }

    void aHotkeyThatIsClearedIsSavedEmptyAndNotCheckedAgainstTheReservedOnes() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QKeySequenceEdit>(window, "goToRowInput")->clear();

        QSignalSpy finished(&window, &QDialog::finished);
        window.accept();

        QCOMPARE(finished.count(), 1);
        QVERIFY(settings.controls.goToRow.isEmpty());
    }

    void aChosenHintFontIsSavedToTheProject() {
        SettingsWindow window(&settings, &project, nullptr);
        auto* const select = field<QFontComboBox>(window, "hintFontSelect");

        QString other;

        for (i32 index = 0; index < select->count() && other.isEmpty(); index++) {
            if (select->itemText(index) != select->currentFont().family()) {
                other = select->itemText(index);
            }
        }

        if (other.isEmpty()) {
            QSKIP("only one font is installed");
        }

        select->setCurrentFont(QFont(other));
        window.accept();

        QCOMPARE(project.hintFont, other);
    }

    void retranslatingWithAnEndpointSelectedRetranslatesItsDescription() {
        SettingsWindow window(&settings, &project, nullptr);
        field<QPushButton>(window, "addEndpointButton")->click();

        const MarkingTranslator translator;
        translator.retranslate(&window);

        QVERIFY(field<QLabel>(window, "typeDescriptionLabel")->text().startsWith(MarkingTranslator::MARK));
    }

    void anExportIntoAFolderThatCannotBeWrittenIsReported() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const QTemporaryDir dir;
        QVERIFY(QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));

        {
            SettingsWindow window(&settings, &project, nullptr);
            catcher->file = dir.path() + u"/lints.json"_s;
            field<QPushButton>(window, "exportLintsButton")->click();
        }

        QVERIFY(catcher->text.contains(u"Writing"_qsv));
        QVERIFY(!QFile::exists(dir.path() + u"/lints.json"_s));
        catcher->text.clear();

        QVERIFY(QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    }

    void anImportOfAFileThatCannotBeReadIsReported() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        const QTemporaryDir dir;
        const QString path = dir.path() + u"/lints.json"_s;
        writeFile(path, "[]");
        QVERIFY(QFile::setPermissions(path, QFileDevice::WriteOwner));

        SettingsWindow window(&settings, &project, nullptr);
        catcher->file = path;
        field<QPushButton>(window, "importLintsButton")->click();

        QVERIFY(catcher->text.contains(u"Reading"_qsv));
        catcher->text.clear();

        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }
};

QTEST_MAIN(TestSettingsWindow)
#include "tst_settingswindow.moc"
