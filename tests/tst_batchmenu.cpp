#include "Permissions.hpp"
#include "BatchMenu.hpp"
#include "CodeViewer.hpp"
#include "Constants.hpp"
#include "FileSelectMenu.hpp"
#include "MarkingTranslator.hpp"
#include "ModalCatcher.hpp"
#include "ProjectSettings.hpp"
#include "ScriptEditor.hpp"
#include <QTextBlock>
#include <QTextLayout>
#include "TabListModel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QtTest>

namespace {
template <typename T>
auto part(const QWidget& root, const QString& name) -> T* {
    return root.findChild<T*>(name);
}

void writeFile(const QString& path, const QString& text) {
    QFile file(path);
    QVERIFY(file.open(QFile::WriteOnly));
    file.write(text.toUtf8());
}

auto readFile(const QString& path) -> QString {
    QFile file(path);
    return file.open(QFile::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

// Answers the script editor dialog that a call is about to open
class EditorDriver {
   public:
    EditorDriver(const QDialogButtonBox::StandardButton press, const QString& newText = QString()) {
        poll.setInterval(5);
        QObject::connect(&poll, &QTimer::timeout, &poll, [this, press, newText] {
            auto* const dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());

            if (dialog == nullptr || qobject_cast<QMessageBox*>(dialog) != nullptr) {
                return;
            }

            auto* const editor = dialog->findChild<CodeViewer*>();
            auto* const buttons = dialog->findChild<QDialogButtonBox*>();

            opened = true;
            title = dialog->windowTitle();
            readOnly = editor->isReadOnly();
            shownSource = editor->toPlainText();
            hasSave = buttons->button(QDialogButtonBox::Save) != nullptr;
            hasClose = buttons->button(QDialogButtonBox::Close) != nullptr;

            if (!newText.isNull()) {
                editor->setPlainText(newText);
            }

            buttons->button(press)->click();
            poll.stop();
        });
        poll.start();

        QTimer::singleShot(3000, &poll, [] {
            if (auto* const widget = QApplication::activeModalWidget()) {
                widget->close();
            }
        });
    }

    bool opened = false;
    bool readOnly = false;
    bool hasSave = false;
    bool hasClose = false;
    QString title;
    QString shownSource;

   private:
    QTimer poll;
};

struct Request {
    JSScript script;
    Selected selected;
    u8 column = 0;
    BatchVariant variant;
};

// The scripts directory is seeded before the menu loads it
struct DataDir {
    explicit DataDir(const vector<pair<QString, QString>>& scripts) {
        qApp->setProperty("data-location", dir.path());
        QVERIFY(QDir().mkpath(scriptsPath()));

        for (const auto& [name, text] : scripts) {
            writeFile(scriptsPath() % u'/' % name, text);
        }
    }

    [[nodiscard]] auto scriptsPath() const -> QString { return dir.path() % u"/scripts"_qsv; }

    QTemporaryDir dir;
};
}  // namespace

struct Fixture {
    explicit Fixture(const vector<pair<QString, QString>>& scripts = {}) : data(scripts), menu(&host) {
        projectSettings.lineLengthHint = 60;
        menu.init(files, &projectSettings);
        menu.addColumn(u"Translation"_s);

        // The built-in scripts are what the list holds besides the scripts of the directory
        builtIn = scriptSelect()->count() - static_cast<i32>(ranges::count_if(scripts, [](const auto& script) -> bool {
            return script.first.endsWith(u".js"_qsv);
        }));

        QObject::connect(
            &menu,
            &BatchMenu::actionRequested,
            &menu,
            [this](JSScript script, Selected selected, u8 column, BatchVariant variant) {
                requests.push_back(Request{ std::move(script), selected, column, variant });
            }
        );
    }

    [[nodiscard]] auto scriptSelect() const -> QComboBox* { return part<QComboBox>(menu, u"scriptSelect"_s); }

    [[nodiscard]] auto scriptEdit() const -> QPlainTextEdit* { return part<QPlainTextEdit>(menu, u"scriptEdit"_s); }

    [[nodiscard]] auto columnSelect() const -> QComboBox* { return part<QComboBox>(menu, u"translationColumnSelect"_s); }

    [[nodiscard]] auto trimSelect() const -> QComboBox* { return part<QComboBox>(menu, u"trimSelect"_s); }

    [[nodiscard]] auto wrapSpinBox() const -> QSpinBox* { return part<QSpinBox>(menu, u"wrapSpinBox"_s); }

    [[nodiscard]] auto button(const QString& name) const -> QPushButton* { return part<QPushButton>(menu, name); }

    [[nodiscard]] auto fileSelect() const -> FileSelectMenu* { return host.findChild<FileSelectMenu*>(); }

    [[nodiscard]] auto itemTexts() const -> QStringList {
        QStringList texts;

        for (i32 idx = 0; idx < scriptSelect()->count(); idx++) {
            texts.push_back(scriptSelect()->itemText(idx));
        }

        return texts;
    }

    void selectFile(const QString& name) const {
        for (QCheckBox* const box : fileSelect()->findChildren<QCheckBox*>()) {
            if (box->text() == name) {
                box->setChecked(true);
            }
        }
    }

    // A script, a column and a file picked: a request goes out when processing
    void readyToProcess(const i32 script) const {
        scriptSelect()->setCurrentIndex(script);
        columnSelect()->setCurrentIndex(1);
        selectFile(u"actors"_s);
    }

    // Opens the popup the way a user would, which is when the list of scripts is re-read
    void openPopup() const {
        QTest::keyClick(scriptSelect(), Qt::Key_F4);
        scriptSelect()->hidePopup();
    }

    DataDir data;
    QWidget host;
    ProjectSettings projectSettings;
    BatchMenu menu;
    vector<Request> requests;
    vector<TabListItem> files{ { .name = u"actors"_s, .properties = {} }, { .name = u"items"_s, .properties = {} } };
    i32 builtIn = 0;
};

class TestBatchMenu final : public QObject {
    Q_OBJECT

   private slots:
    void startsOnTheFirstBuiltInScript() {
        Fixture fixture;

        QCOMPARE(fixture.itemTexts(), QStringList({ u"Trim"_s, u"Wrap"_s, u"Latinize"_s }));
        QCOMPARE(fixture.scriptSelect()->currentIndex(), 0);
        QVERIFY(fixture.scriptEdit()->isReadOnly());
        QCOMPARE(fixture.scriptEdit()->toPlainText(), TRIM_SCRIPT.toString());
        QVERIFY(!fixture.scriptSelect()->isEditable());
        QVERIFY(!part<QWidget>(fixture.menu, u"trimWidget"_s)->isHidden());
        QVERIFY(part<QWidget>(fixture.menu, u"wrapWidget"_s)->isHidden());
    }

    void eachBuiltInScriptShowsItsOwnSourceAndOptions() {
        Fixture fixture;
        const auto* const trim = part<QWidget>(fixture.menu, u"trimWidget"_s);
        const auto* const wrap = part<QWidget>(fixture.menu, u"wrapWidget"_s);

        fixture.scriptSelect()->setCurrentIndex(1);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), WRAP_SCRIPT.toString());
        QVERIFY(trim->isHidden());
        QVERIFY(!wrap->isHidden());

        fixture.scriptSelect()->setCurrentIndex(2);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), LATINIZE_SCRIPT.toString());
        QVERIFY(trim->isHidden());
        QVERIFY(wrap->isHidden());
        QVERIFY(fixture.scriptEdit()->isReadOnly());
    }

    void processingNeedsATranslationColumn() {
        Fixture fixture;
        fixture.selectFile(u"actors"_s);
        const ModalCatcher catcher;

        fixture.button(u"processButton"_s)->click();

        QVERIFY(catcher.text.contains(u"translation column"_qsv));
        QVERIFY(fixture.requests.empty());
    }

    void processingNeedsFiles() {
        Fixture fixture;
        fixture.columnSelect()->setCurrentIndex(1);
        const ModalCatcher catcher;

        fixture.button(u"processButton"_s)->click();

        QVERIFY(catcher.text.contains(u"files"_qsv));
        QVERIFY(fixture.requests.empty());
    }

    void wrappingNeedsALength() {
        Fixture fixture;
        fixture.readyToProcess(1);
        fixture.wrapSpinBox()->clear();
        const ModalCatcher catcher;

        fixture.button(u"processButton"_s)->click();

        QVERIFY(catcher.text.contains(u"20 to 255"_qsv));
        QVERIFY(fixture.requests.empty());
    }

    void theWrapLengthIsBoundByWhatTheActionTakes() {
        Fixture fixture;

        fixture.wrapSpinBox()->setValue(1000);
        QCOMPARE(fixture.wrapSpinBox()->value(), 255);

        fixture.wrapSpinBox()->setValue(0);
        QCOMPARE(fixture.wrapSpinBox()->value(), 20);
    }

    void trimmingAsksForTheEndsNamedInTheList() {
        Fixture fixture;
        fixture.readyToProcess(0);
        const array<LintFlags, 3> expected{
            LintFlags::LeadingWhitespace,
            LintFlags::TrailingWhitespace,
            LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace,
        };

        QCOMPARE(fixture.trimSelect()->itemText(0), u"Leading"_s);
        QCOMPARE(fixture.trimSelect()->itemText(1), u"Trailing"_s);
        QCOMPARE(fixture.trimSelect()->itemText(2), u"Leading + Trailing"_s);

        for (i32 idx = 0; idx < 3; idx++) {
            fixture.trimSelect()->setCurrentIndex(idx);
            fixture.button(u"processButton"_s)->click();
        }

        QCOMPARE(fixture.requests.size(), usize{ 3 });

        for (const auto [idx, request] : views::enumerate(fixture.requests)) {
            QVERIFY(request.script.action == BatchAction::Trim);
            QVERIFY(std::get<LintFlags>(request.variant) == expected[idx]);
        }
    }

    void wrappingAsksForTheLength() {
        Fixture fixture;
        fixture.readyToProcess(1);
        fixture.wrapSpinBox()->setValue(80);

        fixture.button(u"processButton"_s)->click();

        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QVERIFY(fixture.requests[0].script.action == BatchAction::Wrap);
        QCOMPARE(std::get<u8>(fixture.requests[0].variant), u8{ 80 });
        QCOMPARE(fixture.requests[0].script.source, WRAP_SCRIPT.toString());
        QCOMPARE(fixture.requests[0].column, u8{ 1 });
    }

    void latinizingAsksForNoOption() {
        Fixture fixture;
        fixture.readyToProcess(2);

        fixture.button(u"processButton"_s)->click();

        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QVERIFY(fixture.requests[0].script.action == BatchAction::Latinize);
        QVERIFY(std::get<LintFlags>(fixture.requests[0].variant) == static_cast<LintFlags>(0));
    }

    void aRequestCarriesTheSelectedFiles() {
        Fixture fixture;
        fixture.readyToProcess(2);

        fixture.button(u"processButton"_s)->click();

        QVERIFY((fixture.requests.at(0).selected.flags & FileFlags_Actors) != 0);
        QVERIFY((fixture.requests.at(0).selected.flags & FileFlags_Items) == 0);
    }

    void theWrapButtonTakesTheLengthOfTheProject() {
        Fixture fixture;

        fixture.button(u"wrapValueButton"_s)->click();

        QCOMPARE(fixture.wrapSpinBox()->value(), 60);
    }

    void aNewScriptStartsFromTheTemplate() {
        Fixture fixture;

        fixture.button(u"createScriptButton"_s)->click();

        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 1);
        QCOMPARE(fixture.scriptSelect()->currentIndex(), fixture.builtIn);
        QCOMPARE(fixture.scriptSelect()->currentText(), u"New Script"_s);
        QVERIFY(fixture.scriptSelect()->isEditable());
        QVERIFY(!fixture.scriptEdit()->isReadOnly());
        QCOMPARE(fixture.scriptEdit()->toPlainText(), TEMPLATE_SCRIPT.toString());
        QVERIFY(part<QWidget>(fixture.menu, u"trimWidget"_s)->isHidden());
        QVERIFY(part<QWidget>(fixture.menu, u"wrapWidget"_s)->isHidden());
    }

    void aCustomScriptIsRequestedWithItsNameAndSource() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptSelect()->setEditText(u"mine"_s);
        fixture.scriptEdit()->setPlainText(u"return 1;"_s);
        fixture.columnSelect()->setCurrentIndex(1);
        fixture.selectFile(u"items"_s);

        fixture.button(u"processButton"_s)->click();

        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QVERIFY(fixture.requests[0].script.action == BatchAction::Custom);
        QCOMPARE(fixture.requests[0].script.name, u"mine"_s);
        QCOMPARE(fixture.requests[0].script.source, u"return 1;"_s);
        QVERIFY(!fixture.requests[0].script.hostApi);
    }

    void typingANameRenamesTheScript() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();

        fixture.scriptSelect()->setEditText(u"renamed"_s);

        QCOMPARE(fixture.scriptSelect()->itemText(fixture.builtIn), u"renamed"_s);
    }

    void anEditSurvivesPickingAnotherScript() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptEdit()->setPlainText(u"edited"_s);

        fixture.scriptSelect()->setCurrentIndex(0);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), TRIM_SCRIPT.toString());

        fixture.scriptSelect()->setCurrentIndex(fixture.builtIn);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), u"edited"_s);
    }

    void theEditorReturnsWhatWasSaved() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        const EditorDriver driver(QDialogButtonBox::Save, u"changed in the editor"_s);

        fixture.button(u"editScriptButton"_s)->click();

        QVERIFY(driver.opened);
        QVERIFY(driver.hasSave);
        QVERIFY(!driver.readOnly);
        QCOMPARE(driver.title, u"New Script"_s);
        QCOMPARE(driver.shownSource, TEMPLATE_SCRIPT.toString());
        QCOMPARE(fixture.scriptEdit()->toPlainText(), u"changed in the editor"_s);
    }

    void cancellingTheEditorKeepsTheScript() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        const EditorDriver driver(QDialogButtonBox::Cancel, u"thrown away"_s);

        fixture.button(u"editScriptButton"_s)->click();

        QVERIFY(driver.opened);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), TEMPLATE_SCRIPT.toString());
    }

    void aBuiltInScriptOpensReadOnly() {
        Fixture fixture;
        const EditorDriver driver(QDialogButtonBox::Close);

        fixture.button(u"editScriptButton"_s)->click();

        QVERIFY(driver.opened);
        QVERIFY(driver.readOnly);
        QVERIFY(driver.hasClose);
        QVERIFY(!driver.hasSave);
        QCOMPARE(driver.title, u"Trim"_s);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), TRIM_SCRIPT.toString());
    }

    void savingWritesTheCustomScripts() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptSelect()->setEditText(u"alpha"_s);
        fixture.scriptEdit()->setPlainText(u"var a = 1;"_s);

        QVERIFY(fixture.menu.save());

        QCOMPARE(readFile(fixture.data.scriptsPath() % u"/alpha.js"_qsv), u"var a = 1;"_s);
        QCOMPARE(fixture.scriptSelect()->itemText(fixture.builtIn), u"alpha.js"_s);
    }

    void savingWithoutCustomScriptsSucceeds() {
        Fixture fixture;

        QVERIFY(fixture.menu.save());
        QVERIFY(QDir(fixture.data.scriptsPath()).entryList(QDir::Files).isEmpty());
    }

    void renamingASavedScriptMovesItsFile() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptSelect()->setEditText(u"alpha"_s);
        QVERIFY(fixture.menu.save());

        fixture.scriptSelect()->setEditText(u"beta.JS"_s);
        QVERIFY(fixture.menu.save());

        QVERIFY(!QFile::exists(fixture.data.scriptsPath() % u"/alpha.js"_qsv));
        QVERIFY(QFile::exists(fixture.data.scriptsPath() % u"/beta.JS"_qsv));
    }

    void aSavedNameIsOnlyAFileName() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptSelect()->setEditText(u"../../outside"_s);

        QVERIFY(fixture.menu.save());

        QVERIFY(QFile::exists(fixture.data.scriptsPath() % u"/outside.js"_qsv));
        QVERIFY(!QFile::exists(fixture.data.dir.path() % u"/outside.js"_qsv));
    }

    void anUnnamedScriptIsSavedAsScript() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptSelect()->setEditText(QString());

        QVERIFY(fixture.menu.save());

        QVERIFY(QFile::exists(fixture.data.scriptsPath() % u"/script.js"_qsv));
    }

    void aScriptThatCannotBeWrittenAbortsTheSaveOrIsSkipped_data() {
        QTest::addColumn<QString>("button");

        QTest::newRow("abort") << u"Abort"_s;
        QTest::newRow("continue anyway") << u"Continue anyway"_s;
    }

    void aScriptThatCannotBeWrittenAbortsTheSaveOrIsSkipped() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        QFETCH(const QString, button);

        Fixture fixture({ { u"a.js"_s, u"A source"_s } });
        const QString path = fixture.data.scriptsPath() % u"/a.js"_qsv;
        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner));

        ModalCatcher modal;
        modal.press = button;

        QVERIFY(!fixture.menu.save());
        QCOMPARE(readFile(path), u"A source"_s);

        if (button == u"Continue anyway"_s) {
            QVERIFY(modal.seen.last().contains(u"Failed to save scripts"_qsv));
            QVERIFY(modal.seen.last().contains(u"Skipped saving script a.js"_qsv));
        }

        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }

    void aScriptThatCannotBeReadIsLeftOutOfTheList() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        Fixture fixture;
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));

        const QString path = fixture.data.scriptsPath() % u"/secret.js"_qsv;
        writeFile(path, u"x"_s);
        QVERIFY(QFile::setPermissions(path, QFileDevice::WriteOwner));

        QTest::keyClick(fixture.scriptSelect(), Qt::Key_Down, Qt::AltModifier);
        fixture.scriptSelect()->hidePopup();

        QVERIFY(!fixture.itemTexts().contains(u"secret.js"_qsv));
        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }

    // The old text's colors stay over the new one until the timer fires; then a comment is a single colored run
    void theEditorHighlightsAgainOnceTheTextSettles() {
        ScriptEditor editor(nullptr, u"script"_s, u"var a = 1;"_s, false);
        auto* const edit = editor.findChild<CodeViewer*>();
        const auto formats = [edit] -> QList<QTextLayout::FormatRange> {
            return edit->document()->firstBlock().layout()->formats();
        };
        const qsizetype colored = formats().size();
        QVERIFY(colored > 1);

        // The same text again: there is nothing to highlight
        edit->setPlainText(u"var a = 1;"_s);
        QTest::qWait(400);
        QCOMPARE(formats().size(), colored);

        edit->setPlainText(u"// a note"_s);

        QTRY_COMPARE_WITH_TIMEOUT(formats().size(), qsizetype{ 1 }, 3000);
        QCOMPARE(formats().first().length, 9);
    }

    void scriptsOnDiskAreListedAndOpenable() {
        Fixture fixture({ { u"a.js"_s, u"A source"_s }, { u"b.js"_s, u"B source"_s }, { u"note.txt"_s, u"not a script"_s } });

        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 2);
        QVERIFY(fixture.itemTexts().contains(u"a.js"_qsv));
        QVERIFY(fixture.itemTexts().contains(u"b.js"_qsv));

        fixture.scriptSelect()->setCurrentIndex(static_cast<i32>(fixture.itemTexts().indexOf(u"b.js"_s)));

        QCOMPARE(fixture.scriptEdit()->toPlainText(), u"B source"_s);
        QVERIFY(!fixture.scriptEdit()->isReadOnly());
        QVERIFY(fixture.scriptSelect()->isEditable());
    }

    void openingTheListPicksUpNewFiles() {
        Fixture fixture;
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));
        writeFile(fixture.data.scriptsPath() % u"/late.js"_qsv, u"late"_s);
        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn);

        fixture.openPopup();

        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 1);
        QVERIFY(fixture.itemTexts().contains(u"late.js"_qsv));
    }

    void otherKeysDoNotReadTheDirectory() {
        Fixture fixture;
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));
        writeFile(fixture.data.scriptsPath() % u"/late.js"_qsv, u"late"_s);

        QTest::keyClick(fixture.scriptSelect(), Qt::Key_A);

        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn);
    }

    void altDownAndAClickAlsoReadTheDirectory() {
        Fixture fixture;
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));

        writeFile(fixture.data.scriptsPath() % u"/one.js"_qsv, u"1"_s);
        QTest::keyClick(fixture.scriptSelect(), Qt::Key_Down, Qt::AltModifier);
        fixture.scriptSelect()->hidePopup();
        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 1);

        writeFile(fixture.data.scriptsPath() % u"/two.js"_qsv, u"2"_s);
        QMouseEvent press(
            QEvent::MouseButtonPress,
            QPointF(5, 5),
            fixture.scriptSelect()->mapToGlobal(QPointF(5, 5)),
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier
        );
        QCoreApplication::sendEvent(fixture.scriptSelect(), &press);
        fixture.scriptSelect()->hidePopup();
        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 2);
    }

    void aRemovedScriptLeavesTheList() {
        Fixture fixture({ { u"a.js"_s, u"A"_s }, { u"b.js"_s, u"B"_s } });
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));
        QVERIFY(QFile::remove(fixture.data.scriptsPath() % u"/a.js"_qsv));

        fixture.openPopup();

        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 1);
        QVERIFY(!fixture.itemTexts().contains(u"a.js"_qsv));
        // Trim stayed open: nothing to warn about
        QCOMPARE(fixture.scriptSelect()->currentIndex(), 0);
    }

    void removingTheOpenScriptWarnsAndMovesToTheFirst() {
        Fixture fixture({ { u"a.js"_s, u"A"_s } });
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));
        fixture.scriptSelect()->setCurrentIndex(fixture.builtIn);
        QVERIFY(QFile::remove(fixture.data.scriptsPath() % u"/a.js"_qsv));
        const ModalCatcher catcher;

        fixture.openPopup();

        QVERIFY(catcher.text.contains(u"a.js"_qsv));
        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn);
        QCOMPARE(fixture.scriptSelect()->currentIndex(), 0);
        QVERIFY(fixture.scriptEdit()->isReadOnly());
    }

    void aScriptNeverSavedIsNotRemovedByTheRefresh() {
        Fixture fixture;
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));
        fixture.button(u"createScriptButton"_s)->click();
        fixture.scriptEdit()->setPlainText(u"unsaved"_s);

        fixture.openPopup();

        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 1);
        QCOMPARE(fixture.scriptSelect()->currentIndex(), fixture.builtIn);
        QCOMPARE(fixture.scriptEdit()->toPlainText(), u"unsaved"_s);
    }

    void clearingForgetsTheColumnsAndFiles() {
        Fixture fixture;
        fixture.menu.addColumn(u"Second"_s);
        fixture.columnSelect()->setCurrentIndex(2);
        fixture.wrapSpinBox()->setValue(80);
        fixture.selectFile(u"actors"_s);

        fixture.menu.clear();

        QCOMPARE(fixture.columnSelect()->count(), 1);
        QCOMPARE(fixture.columnSelect()->currentIndex(), 0);
        QVERIFY(fixture.wrapSpinBox()->text().isEmpty());
        QVERIFY(fixture.fileSelect()->findChildren<QCheckBox*>().isEmpty());
    }

    void columnsCanBeRenamed() {
        Fixture fixture;

        fixture.menu.renameColumn(1, u"Renamed"_s);

        QCOMPARE(fixture.columnSelect()->itemText(1), u"Renamed"_s);
    }

    void theFileMenuOpensBesideTheMenuAndClosesWithIt() {
        Fixture fixture;
        fixture.menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&fixture.menu));
        auto* const fileButton = fixture.button(u"fileSelectButton"_s);
        QVERIFY(fixture.fileSelect()->isHidden());

        fileButton->click();

        QVERIFY(!fixture.fileSelect()->isHidden());
        QCOMPARE(fixture.fileSelect()->pos(), fixture.menu.mapToGlobal(QPoint(fixture.menu.width(), 0)));

        fixture.menu.hide();

        QVERIFY(fixture.fileSelect()->isHidden());
        QVERIFY(!fileButton->isChecked());
    }

    void aLanguageChangeRetranslatesTheMenuAndKeepsTheScripts() {
        Fixture fixture;
        fixture.button(u"createScriptButton"_s)->click();
        const MarkingTranslator translator;
        translator.retranslate(&fixture.menu);

        QVERIFY(fixture.button(u"processButton"_s)->text().startsWith(MarkingTranslator::MARK));
        QCOMPARE(fixture.scriptSelect()->count(), fixture.builtIn + 1);
        QCOMPARE(fixture.scriptSelect()->itemText(fixture.builtIn), u"New Script"_s);
    }
};

QTEST_MAIN(TestBatchMenu)
#include "tst_batchmenu.moc"
