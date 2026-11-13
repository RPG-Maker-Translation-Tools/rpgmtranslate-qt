#include "AboutWindow.hpp"
#include "FileSelectMenu.hpp"
#include "GlossaryMenu.hpp"
#include "LintSelectMenu.hpp"
#include "MarkingTranslator.hpp"
#include "Settings.hpp"
#include "ModalCatcher.hpp"
#include "ProjectSettings.hpp"
#include "ReadMenu.hpp"
#include "SearchMenu.hpp"
#include "StatisticsDialog.hpp"
#include "TabListModel.hpp"
#include "TestFiles.hpp"
#include "Types.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QGroupBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QCheckBox>
#include <QTableWidget>
#include <QtTest>

namespace {
// What the search menu asked for
struct Request {
    Selected selected;
    SearchAction action;
    QString searchText;
    QString replaceText;
    SearchLocation location;
    i8 column;
    SearchFlags flags;
};

auto files(const QStringList& names) -> vector<TabListItem> {
    vector<TabListItem> result;

    for (const QString& name : names) {
        result.push_back(TabListItem{ .name = name, .properties = {} });
    }

    return result;
}

// A search menu whose requests are collected, with the actors file selected
struct SearchFixture {
    SearchFixture() : menu(&host) {
        menu.init(files({ u"actors"_s, u"items"_s }));
        host.findChild<FileSelectMenu*>()->findChild<QCheckBox*>()->setChecked(true);

        QObject::connect(
            &menu,
            &SearchMenu::actionRequested,
            &menu,
            [this](
                const Selected selected,
                const SearchAction action,
                const QString& searchText,
                const QString& replaceText,
                const SearchLocation location,
                const i8 column,
                const SearchFlags flags
            ) -> void { requests.push_back(Request{ selected, action, searchText, replaceText, location, column, flags }); }
        );
    }

    template <typename Type>
    auto find(const char* const name) const -> Type* {
        return menu.findChild<Type*>(QString::fromLatin1(name));
    }

    void search(const QString& text) const { find<QPlainTextEdit>("searchInput")->setPlainText(text); }

    void replaceWith(const QString& text) const { find<QPlainTextEdit>("replaceInput")->setPlainText(text); }

    void clickSearch() const { find<QPushButton>("searchButton")->click(); }

    void clickReplace() const { find<QPushButton>("replaceButton")->click(); }

    QWidget host;
    SearchMenu menu;
    vector<Request> requests;
};

auto groupNamed(const QWidget& parent, const QString& title) -> QGroupBox* {
    for (auto* const group : parent.findChildren<QGroupBox*>()) {
        if (group->title() == title) {
            return group;
        }
    }

    return nullptr;
}

// The checkbox that selects or clears the whole group
auto allBoxOf(const QGroupBox* const group) -> QCheckBox* {
    for (auto* const box : group->findChildren<QCheckBox*>()) {
        if (box->text() == u"All"_s) {
            return box;
        }
    }

    return nullptr;
}

auto termOf(const QString& source, const QString& translation, const QString& note = {}, const bool enabled = true) -> Term {
    return Term{ .term = source,
                 .translation = translation,
                 .note = note,
                 .sourceMatchMode = MatchModeInfo{ .mode = { .tag = MatchMode::Tag::Exact }, .case_sensitive = false, .permissive = false },
                 .translationMatchMode = MatchModeInfo{ .mode = { .tag = MatchMode::Tag::Exact }, .case_sensitive = false, .permissive = false },
                 .enabled = enabled };
}

// The line edits of a cell of the glossary: the text first, then the fuzzy threshold
auto cellEdits(const GlossaryMenu& menu, const i32 row, const i32 column) -> QList<QLineEdit*> {
    auto* const tree = menu.findChild<QTreeWidget*>();
    return tree->itemWidget(tree->topLevelItem(row), column)->findChildren<QLineEdit*>();
}

auto actionCells(const GlossaryMenu& menu) -> QList<ActionButtonsCell*> {
    return menu.findChildren<ActionButtonsCell*>();
}

auto statistics(const char* const name, const u32 sourceWords, const u32 sourceChars, const u32 words, const u32 chars)
    -> FileStatistics {
    FileStatistics result{};
    std::copy_n(name, strlen(name), result.filename.data());
    result.source = { .words = sourceWords, .characters = sourceChars, .characters_no_whitespace = sourceChars };
    result.translation = { .words = words, .characters = chars, .characters_no_whitespace = chars };
    return result;
}
}  // namespace

struct LintFixture {
    LintFixture() : menu(&host), glossary(&host) {
        projectSettings.projectPath = dir.path();
        projectSettings.engineType = EngineType::MVMZ;
        projectSettings.sourceDirectory = SourceDirectory::LowercaseData;
        menu.init(&settings, &projectSettings, &glossary);
    }

    QTemporaryDir dir;
    QWidget host;
    Settings settings;
    ProjectSettings projectSettings;
    LintSelectMenu menu;
    GlossaryMenu glossary;
};

// The About window reads a file next to the executable: whatever is there is kept aside and put back
struct ThirdPartyNotice {
    ThirdPartyNotice() {
        QFile file(path);

        if (file.open(QIODevice::ReadOnly)) {
            original = file.readAll();
            existed = true;
            file.close();
            QFile::remove(path);
        }
    }

    ~ThirdPartyNotice() {
        QFile::remove(path);

        if (existed) {
            QFile file(path);

            if (file.open(QIODevice::WriteOnly)) {
                file.write(original);
            }
        }
    }

    ThirdPartyNotice(const ThirdPartyNotice&) = delete;
    auto operator=(const ThirdPartyNotice&) -> ThirdPartyNotice& = delete;

    QString path = qApp->applicationDirPath() + u"/THIRD-PARTY-NOTICE.md"_s;
    QByteArray original;
    bool existed = false;
};

class TestDialogs final : public QObject {
    Q_OBJECT

   private slots:
    // SearchMenu
    void aSearchAsksForTheSelectedFiles() {
        SearchFixture fixture;
        fixture.search(u"hero"_s);

        fixture.clickSearch();

        QCOMPARE(fixture.requests.size(), usize{ 1 });

        const Request& request = fixture.requests[0];
        QCOMPARE(request.action, SearchAction::Search);
        QCOMPARE(request.searchText, u"hero"_s);
        QVERIFY(request.replaceText.isEmpty());
        QCOMPARE(request.selected.flags, static_cast<FileFlags>(FileFlags_Actors));
        // "Search Everywhere" is both locations, "All Columns" is -1
        QCOMPARE(static_cast<u8>(request.location), u8{ 3 });
        QCOMPARE(request.column, i8{ -1 });
        QCOMPARE(static_cast<u8>(request.flags), u8{ 0 });
    }

    void theSearchOptionsBecomeFlags() {
        SearchFixture fixture;
        fixture.search(u"hero"_s);
        fixture.find<QPushButton>("sensitivityButton")->setChecked(true);
        fixture.find<QPushButton>("wholeButton")->setChecked(true);
        fixture.find<QPushButton>("commentButton")->setChecked(true);

        fixture.clickSearch();

        QCOMPARE(
            static_cast<u8>(fixture.requests[0].flags),
            static_cast<u8>(SearchFlags::CaseSensitive | SearchFlags::WholeWord | SearchFlags::Comment)
        );
    }

    void theLocationAndColumnAreChosenFromTheSelects() {
        SearchFixture fixture;
        fixture.search(u"hero"_s);
        fixture.menu.addColumn(u"Translation"_s);
        fixture.menu.addColumn(u"Notes"_s);

        fixture.find<QComboBox>("searchLocationSelect")->setCurrentIndex(1);
        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(3);
        fixture.clickSearch();

        fixture.find<QComboBox>("searchLocationSelect")->setCurrentIndex(2);
        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(1);
        fixture.clickSearch();

        QCOMPARE(fixture.requests.size(), usize{ 2 });
        QCOMPARE(static_cast<u8>(fixture.requests[0].location), u8{ 1 });
        QCOMPARE(fixture.requests[0].column, i8{ 2 });
        QCOMPARE(static_cast<u8>(fixture.requests[1].location), u8{ 2 });
        QCOMPARE(fixture.requests[1].column, i8{ 0 });
    }

    void columnsCanBeAddedRenamedAndCleared() {
        SearchFixture fixture;
        auto* const columns = fixture.find<QComboBox>("searchColumnSelect");
        const i32 base = columns->count();

        fixture.menu.addColumn(u"Translation"_s);
        fixture.menu.addColumn(u"Notes"_s);
        QCOMPARE(columns->count(), base + 2);

        fixture.menu.renameColumn(1, u"Remarks"_s);
        QCOMPARE(columns->itemText(base), u"Remarks"_s);
        QCOMPARE(columns->itemText(base + 1), u"Notes"_s);

        fixture.search(u"hero"_s);
        fixture.find<QPushButton>("regexButton")->setChecked(true);
        fixture.menu.clear();

        QCOMPARE(columns->count(), base);
        QVERIFY(fixture.find<QPlainTextEdit>("searchInput")->toPlainText().isEmpty());
        QVERIFY(!fixture.find<QPushButton>("regexButton")->isChecked());
        QVERIFY(host(fixture).findChild<FileSelectMenu*>()->empty());
    }

    void aSearchNeedsFilesAndText() {
        SearchFixture fixture;
        host(fixture).findChild<FileSelectMenu*>()->findChild<QPushButton*>(u"deselectAllButton"_s)->click();
        fixture.search(u"hero"_s);

        {
            ModalCatcher modal;
            fixture.clickSearch();
            QVERIFY2(modal.text.contains(u"Select some files"_s), qPrintable(modal.text));
        }

        host(fixture).findChild<FileSelectMenu*>()->findChild<QPushButton*>(u"selectAllButton"_s)->click();
        fixture.search(QString());

        {
            ModalCatcher modal;
            fixture.clickSearch();
            QCOMPARE(modal.text, u"Search input is empty."_s);
        }

        QVERIFY(fixture.requests.empty());
    }

    void aBadRegularExpressionIsRefused() {
        SearchFixture fixture;
        fixture.find<QPushButton>("regexButton")->setChecked(true);

        fixture.search(u"(unclosed"_s);
        {
            ModalCatcher modal;
            fixture.clickSearch();
            QVERIFY2(modal.text.contains(u"Regular expression is invalid"_s), qPrintable(modal.text));
        }

        // Matching nothing at all would match between every two characters
        fixture.search(u".*"_s);
        {
            ModalCatcher modal;
            fixture.clickSearch();
            QVERIFY2(modal.text.contains(u"matches empty text"_s), qPrintable(modal.text));
        }

        QVERIFY(fixture.requests.empty());

        fixture.search(u"h.ro"_s);
        fixture.clickSearch();
        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QVERIFY((fixture.requests[0].flags & SearchFlags::RegExp) != 0);
    }

    void aReplaceNeedsACertainColumn() {
        SearchFixture fixture;
        fixture.search(u"hero"_s);
        fixture.replaceWith(u"knight"_s);
        fixture.menu.addColumn(u"Translation"_s);

        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(0);
        {
            ModalCatcher modal;
            fixture.clickReplace();
            QVERIFY2(modal.text.contains(u"certain column"_s), qPrintable(modal.text));
        }

        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(1);
        {
            ModalCatcher modal;
            fixture.clickReplace();
            QVERIFY2(modal.text.contains(u"certain column"_s), qPrintable(modal.text));
        }

        QVERIFY(fixture.requests.empty());

        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(2);
        fixture.clickReplace();

        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QCOMPARE(fixture.requests[0].action, SearchAction::Replace);
        QCOMPARE(fixture.requests[0].replaceText, u"knight"_s);
        QCOMPARE(fixture.menu.replaceText(), u"knight"_s);
    }

    void thePutOptionTurnsAReplaceIntoAPut() {
        SearchFixture fixture;
        fixture.search(u"hero"_s);
        fixture.replaceWith(u"knight"_s);
        fixture.menu.addColumn(u"Translation"_s);
        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(2);
        fixture.find<QPushButton>("putButton")->setChecked(true);

        fixture.clickReplace();

        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QCOMPARE(fixture.requests[0].action, SearchAction::Put);
        QVERIFY((fixture.requests[0].flags & SearchFlags::Put) != 0);
    }

    void replacingWithNothingNeedsConfirmation() {
        SearchFixture fixture;
        fixture.search(u"hero"_s);
        fixture.menu.addColumn(u"Translation"_s);
        fixture.find<QComboBox>("searchColumnSelect")->setCurrentIndex(2);

        {
            ModalCatcher modal(QMessageBox::No);
            fixture.clickReplace();
            QVERIFY2(modal.text.contains(u"replace the text to nothing"_s), qPrintable(modal.text));
        }
        QVERIFY(fixture.requests.empty());

        {
            ModalCatcher modal(QMessageBox::Yes);
            fixture.clickReplace();
        }
        QCOMPARE(fixture.requests.size(), usize{ 1 });
        QVERIFY(fixture.requests[0].replaceText.isEmpty());
    }

    void theFileMenuOfTheSearchFollowsTheSearchMenu() {
        SearchFixture fixture;
        fixture.menu.show();

        auto* const fileMenu = host(fixture).findChild<FileSelectMenu*>();
        auto* const button = fixture.find<QPushButton>("fileSelectButton");

        button->click();
        QVERIFY(fileMenu->isVisible());
        button->click();
        QVERIFY(!fileMenu->isVisible());

        button->setCheckable(true);
        button->setChecked(true);
        fileMenu->show();
        fixture.menu.hide();

        QVERIFY(!fileMenu->isVisible());
        QVERIFY(!button->isChecked());
    }

    void theSearchMenuRetranslates() {
        SearchFixture fixture;
        const MarkingTranslator translator;
        translator.retranslate(&fixture.menu);

        QVERIFY(fixture.menu.findChild<QPushButton*>(u"searchButton"_s)->toolTip().startsWith(MarkingTranslator::MARK));
    }

    // StatisticsDialog
    void theStatisticsDialogListsEveryFileAndTheTotals() {
        StatisticsDialog* const dialog = new StatisticsDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose, false);
        const QScopeGuard guard([dialog] { delete dialog; });

        dialog->fill({ statistics("actors", 3, 17, 2, 12), statistics("items", 10, 50, 8, 40) });

        auto* const table = dialog->findChild<QTableWidget*>();
        QCOMPARE(table->rowCount(), 3);
        QCOMPARE(table->columnCount(), 5);
        QCOMPARE(table->item(0, 0)->text(), u"actors"_s);
        QCOMPARE(table->item(1, 2)->text(), u"50"_s);
        QCOMPARE(table->item(2, 0)->text(), u"Total"_s);
        QCOMPARE(table->item(2, 1)->text(), u"13"_s);
        QCOMPARE(table->item(2, 2)->text(), u"67"_s);
        QCOMPARE(table->item(2, 3)->text(), u"10"_s);
        QCOMPARE(table->item(2, 4)->text(), u"52"_s);
        QVERIFY(table->item(2, 1)->font().bold());
        QVERIFY(!table->item(0, 1)->font().bold());
        QCOMPARE(table->item(0, 1)->textAlignment() & Qt::AlignRight, static_cast<i32>(Qt::AlignRight));
    }

    void anEmptyProjectHasOnlyTheTotalsRow() {
        StatisticsDialog* const dialog = new StatisticsDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose, false);
        const QScopeGuard guard([dialog] { delete dialog; });

        dialog->fill({});

        auto* const table = dialog->findChild<QTableWidget*>();
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 1)->text(), u"0"_s);
    }

    void theStatisticsDialogRetranslatesItsTotalLabelAndHeaders() {
        StatisticsDialog* const dialog = new StatisticsDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose, false);
        const QScopeGuard guard([dialog] { delete dialog; });

        dialog->fill({ statistics("actors", 1, 1, 1, 1) });
        const MarkingTranslator translator;
        translator.retranslate(dialog);

        auto* const table = dialog->findChild<QTableWidget*>();
        QCOMPARE(table->item(1, 0)->text(), MarkingTranslator::MARK + u"Total"_s);
        QCOMPARE(table->horizontalHeaderItem(0)->text(), MarkingTranslator::MARK + u"File"_s);
    }

    // AboutWindow
    void theAboutWindowShowsTheVersionAndTheBuild() {
        AboutWindow* const window = new AboutWindow;
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        const QScopeGuard guard([window] { delete window; });

        const auto labels = window->findChildren<QLabel*>();
        QString all;

        for (const QLabel* const label : labels) {
            all += label->text() + u'\n';
        }

        QVERIFY2(all.contains(u"RPGMTranslate v"_s), qPrintable(all.left(300)));
        QVERIFY(all.contains(u"Compiler:"_s));
        QVERIFY(all.contains(u"Rust version:"_s));
        QVERIFY(all.contains(u"Qt "_s + QString::fromLatin1(qVersion())));
    }

    void theAboutWindowSaysWhenTheThirdPartyNoticeIsMissing() {
        const ThirdPartyNotice kept;

        AboutWindow* const window = new AboutWindow;
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        const QScopeGuard guard([window] { delete window; });

        QString all;
        for (const QLabel* const label : window->findChildren<QLabel*>()) {
            all += label->text() + u'\n';
        }

        QVERIFY2(all.contains(u"Failed to load THIRD-PARTY-NOTICE.md"_s), qPrintable(all));
    }

    void theAboutWindowShowsTheThirdPartyNoticeWhenThereIsOne() {
        const ThirdPartyNotice kept;
        writeFile(kept.path, "Third party notice text");

        AboutWindow* const window = new AboutWindow;
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        const QScopeGuard guard([window] { delete window; });

        QString all;
        for (const QLabel* const label : window->findChildren<QLabel*>()) {
            all += label->text() + u'\n';
        }

        QVERIFY2(all.contains(u"Third party notice text"_s), qPrintable(all));

        // A language change fills the texts again
        QEvent change(QEvent::LanguageChange);
        QApplication::sendEvent(window, &change);

        all.clear();
        for (const QLabel* const label : window->findChildren<QLabel*>()) {
            all += label->text() + u'\n';
        }

        QVERIFY(all.contains(u"Third party notice text"_s));
    }

    // ReadMenu
    void aNewReadMenuReadsWithTheDefaults() {
        QWidget host;
        ReadMenu menu(&host);

        QCOMPARE(menu.readMode(), ReadMode::Default);
        QCOMPARE(menu.duplicateMode(), DuplicateMode::Allow);
        QCOMPARE(static_cast<u32>(menu.flags()), u32{ 0 });
        QVERIFY(!menu.parseMapEvents());
        QVERIFY(menu.title().isEmpty());
        QVERIFY(menu.selected().empty());
    }

    void anOpenProjectAppendsWithItsOwnSettings() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;
        settings.duplicateMode = DuplicateMode::Remove;
        settings.engineType = EngineType::MVMZ;

        menu.init(&settings);

        QCOMPARE(menu.readMode(), ReadMode::AppendDefault);
        QCOMPARE(menu.duplicateMode(), DuplicateMode::Remove);
        QVERIFY(!menu.findChild<QComboBox*>(u"duplicateModeSelect"_s)->isEnabled());
        QVERIFY(!menu.findChild<QComboBox*>(u"encodingSelect"_s)->isEnabled());
        QVERIFY(menu.findChild<QCheckBox*>(u"ignoreCheckbox"_s)->isEnabled());
        QVERIFY(menu.findChild<QCheckBox*>(u"skipObsoleteCheckbox"_s)->isEnabled());

        // Forcing a read starts over with the defaults
        menu.findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(static_cast<i32>(ReadMode::DefaultForce));
        QCOMPARE(menu.duplicateMode(), DuplicateMode::Allow);
        QVERIFY(menu.findChild<QComboBox*>(u"duplicateModeSelect"_s)->isEnabled());
        QVERIFY(menu.findChild<QComboBox*>(u"encodingSelect"_s)->isEnabled());

        menu.findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(static_cast<i32>(ReadMode::AppendForce));
        QCOMPARE(menu.duplicateMode(), DuplicateMode::Remove);
    }

    void anAppendUsesTheEncodingOfTheProject() {
        QWidget host;
        ReadMenu menu(&host);
        auto* const encodings = menu.findChild<QComboBox*>(u"encodingSelect"_s);
        QVERIFY(encodings->count() > 1);

        ProjectSettings settings;
        settings.readEncoding = encodings->itemText(1);

        menu.init(&settings);
        QCOMPARE(menu.readEncoding(), encodings->itemText(1));

        // An encoding the list does not have, like the "" that older versions stored for UTF-8, is the first one
        settings.readEncoding = u"no such encoding"_s;
        menu.findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(static_cast<i32>(ReadMode::AppendForce));
        QCOMPARE(menu.readEncoding(), encodings->itemText(0));
    }

    void theDefaultModeDoesNothingOnFilesThatAreAlreadyRead() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;
        menu.init(&settings);
        menu.init(files({ u"actors"_s }));

        ModalCatcher modal;
        menu.findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(static_cast<i32>(ReadMode::Default));

        QVERIFY2(modal.text.contains(u"Default mode does nothing"_s), qPrintable(modal.text));
        QCOMPARE(menu.readMode(), ReadMode::AppendDefault);
    }

    void theModesAreDescribed() {
        QWidget host;
        ReadMenu menu(&host);
        auto* const mode = menu.findChild<QLabel*>(u"readModeLabel"_s);
        auto* const duplicates = menu.findChild<QLabel*>(u"duplicateModeLabel"_s);

        QSet<QString> descriptions;
        ProjectSettings settings;
        menu.init(&settings);

        for (i32 index = 1; index <= 3; index++) {
            menu.findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(index);
            QVERIFY(!mode->text().isEmpty());
            descriptions.insert(mode->text());
        }

        QCOMPARE(descriptions.size(), qsizetype{ 3 });

        menu.findChild<QComboBox*>(u"readModeSelect"_s)->setCurrentIndex(static_cast<i32>(ReadMode::DefaultForce));
        const QString allow = duplicates->text();
        menu.findChild<QComboBox*>(u"duplicateModeSelect"_s)->setCurrentIndex(static_cast<i32>(DuplicateMode::Remove));
        QVERIFY(duplicates->text() != allow);

        QEvent change(QEvent::LanguageChange);
        QApplication::sendEvent(&menu, &change);
        QVERIFY(!mode->text().isEmpty());
    }

    void theOptionsBecomeFlags() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;
        menu.init(&settings);

        menu.findChild<QCheckBox*>(u"ignoreCheckbox"_s)->setChecked(true);
        QCOMPARE(static_cast<u32>(menu.flags()), u32{ BaseFlags_Ignore });

        menu.findChild<QCheckBox*>(u"skipObsoleteCheckbox"_s)->setChecked(true);
        QCOMPARE(static_cast<u32>(menu.flags()), u32{ BaseFlags_Ignore | BaseFlags_SkipObsolete });

        menu.findChild<QCheckBox*>(u"mapEventsCheckbox"_s)->setChecked(true);
        QVERIFY(menu.parseMapEvents());
    }

    void applyingAcceptsAndClosingRejectsAndBothHideTheMenu() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;
        menu.init(&settings);
        QSignalSpy accepted(&menu, &ReadMenu::accepted);
        QSignalSpy rejected(&menu, &ReadMenu::rejected);

        menu.show();
        menu.findChild<QPushButton*>(u"applyButton"_s)->click();
        QCOMPARE(accepted.count(), 1);
        QVERIFY(menu.isHidden());

        // Hiding rejects: the menu closed any other way is not applied
        rejected.clear();
        menu.show();
        menu.findChild<QPushButton*>(u"closeButton"_s)->click();
        QCOMPARE(accepted.count(), 1);
        QCOMPARE(rejected.count(), 1);
        QVERIFY(menu.isHidden());
    }

    void theFileSelectButtonShowsAndHidesTheFileSelection() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;
        menu.init(&settings);
        auto* const selection = host.findChild<FileSelectMenu*>();
        QVERIFY(selection != nullptr);
        QVERIFY(selection->isHidden());

        auto* const button = menu.findChild<QPushButton*>(u"fileSelectButton"_s);
        button->click();
        QVERIFY(!selection->isHidden());

        button->click();
        QVERIFY(selection->isHidden());
    }

    void theEngineDecidesWhichChoicesAreShown() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;

        settings.engineType = EngineType::MVMZ;
        menu.init(&settings);
        QVERIFY(menu.findChild<QWidget*>(u"iniTitleWidget"_s)->isHidden());
        QVERIFY(menu.findChild<QWidget*>(u"encodingWidget"_s)->isHidden());

        settings.engineType = EngineType::XP;
        menu.init(&settings);
        QVERIFY(!menu.findChild<QWidget*>(u"iniTitleWidget"_s)->isHidden());
        QVERIFY(!menu.findChild<QWidget*>(u"encodingWidget"_s)->isHidden());

        settings.engineType = EngineType::VXAce;
        menu.init(&settings);
        QVERIFY(!menu.findChild<QWidget*>(u"iniTitleWidget"_s)->isHidden());
        QVERIFY(menu.findChild<QWidget*>(u"encodingWidget"_s)->isHidden());
        // The archive is only extracted when a project is first opened
        QVERIFY(menu.findChild<QWidget*>(u"archiveEncodingWidget"_s)->isHidden());
    }

    void clearingTheReadMenuStartsOver() {
        QWidget host;
        ReadMenu menu(&host);
        ProjectSettings settings;
        settings.duplicateMode = DuplicateMode::Remove;
        menu.init(&settings);
        menu.init(files({ u"actors"_s }));
        menu.findChild<QCheckBox*>(u"ignoreCheckbox"_s)->setChecked(true);
        menu.findChild<QCheckBox*>(u"mapEventsCheckbox"_s)->setChecked(true);

        menu.clear();

        QCOMPARE(menu.readMode(), ReadMode::Default);
        QCOMPARE(menu.duplicateMode(), DuplicateMode::Allow);
        QVERIFY(!menu.findChild<QComboBox*>(u"readModeSelect"_s)->isEnabled());
        QVERIFY(!menu.findChild<QCheckBox*>(u"ignoreCheckbox"_s)->isEnabled());
        QVERIFY(!menu.findChild<QCheckBox*>(u"ignoreCheckbox"_s)->isChecked());
        QVERIFY(!menu.parseMapEvents());
        QVERIFY(menu.selected().empty());
    }

    void execReturnsAcceptedOnApply() {
        QWidget host;
        ReadMenu menu(&host);
        menu.show();

        QTimer::singleShot(0, &menu, [&menu] { menu.findChild<QPushButton*>(u"applyButton"_s)->click(); });

        QCOMPARE(menu.exec(u"/game"_s, EngineType::MVMZ, false), QDialog::Accepted);
        QVERIFY(!menu.isVisible());
    }

    void execReturnsRejectedOnClose() {
        QWidget host;
        ReadMenu menu(&host);
        menu.show();

        QSignalSpy rejected(&menu, &ReadMenu::rejected);
        QTimer::singleShot(0, &menu, [&menu] { menu.findChild<QPushButton*>(u"closeButton"_s)->click(); });

        QCOMPARE(menu.exec(u"/game"_s, EngineType::MVMZ, true), QDialog::Rejected);
        QVERIFY(rejected.count() >= 1);
    }

    void theTitleOfTheIniFileCanBeUsed() {
        QTemporaryDir dir;
        writeFile(dir.path() + u"/Game.ini"_s, "[Game]\nTitle=My Game\n");

        QWidget host;
        ReadMenu menu(&host);
        menu.show();

        QString title;

        QTimer::singleShot(0, &menu, [&] {
            menu.findChild<QCheckBox*>(u"useIniTitleCheckbox"_s)->setChecked(true);
            title = menu.title();
            menu.findChild<QPushButton*>(u"applyButton"_s)->click();
        });

        QCOMPARE(menu.exec(dir.path(), EngineType::VXAce, false), QDialog::Accepted);
        QCOMPARE(title, u"My Game"_s);
        QVERIFY(menu.findChild<QLabel*>(u"iniTitleLabel"_s)->text().contains(u"Game.ini"_s));

        // Untick it and the title is no longer used
        menu.findChild<QCheckBox*>(u"useIniTitleCheckbox"_s)->setChecked(false);
        QVERIFY(menu.title().isEmpty());
    }

    void aMissingIniFileIsReported() {
        QTemporaryDir dir;
        QWidget host;
        ReadMenu menu(&host);
        menu.show();

        QString said;

        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(u"Failed to extract title"_s));
        QTimer::singleShot(0, &menu, [&] {
            ModalCatcher modal;
            menu.findChild<QCheckBox*>(u"useIniTitleCheckbox"_s)->setChecked(true);
            said = modal.text;
            menu.findChild<QPushButton*>(u"closeButton"_s)->click();
        });

        menu.exec(dir.path(), EngineType::VXAce, false);

        QVERIFY2(said.contains(u"Failed to extract title"_s), qPrintable(said));
        QVERIFY(!menu.findChild<QCheckBox*>(u"useIniTitleCheckbox"_s)->isChecked());
    }

    void anEmptyIniTitleIsReported() {
        QTemporaryDir dir;
        writeFile(dir.path() + u"/Game.ini"_s, "[Game]\nTitle=\n");

        QWidget host;
        ReadMenu menu(&host);
        menu.show();

        QString said;

        QTimer::singleShot(0, &menu, [&] {
            ModalCatcher modal;
            menu.findChild<QCheckBox*>(u"useIniTitleCheckbox"_s)->setChecked(true);
            said = modal.text;
            menu.findChild<QPushButton*>(u"closeButton"_s)->click();
        });

        menu.exec(dir.path(), EngineType::VXAce, false);

        QVERIFY2(said.contains(u"Title is empty"_s), qPrintable(said));
    }

    // GlossaryMenu
    void theGlossaryRoundTripsItsTerms() {
        GlossaryMenu menu;
        Term fuzzy = termOf(u"hero"_s, u"герой"_s, u"a note"_s, false);
        fuzzy.sourceMatchMode = MatchModeInfo{ .mode = { .fuzzy = { .tag = MatchMode::Tag::Fuzzy, .threshold = 0.5 } },
                                               .case_sensitive = true,
                                               .permissive = false };
        fuzzy.translationMatchMode = MatchModeInfo{ .mode = { .both = { .tag = MatchMode::Tag::Both, .threshold = 0.25 } },
                                                    .case_sensitive = false,
                                                    .permissive = true };

        menu.fill({ fuzzy, termOf(u"king"_s, u"король"_s) });
        const Glossary out = menu.glossary();

        QCOMPARE(out.size(), usize{ 2 });
        QCOMPARE(out[0].term, u"hero"_s);
        QCOMPARE(out[0].translation, u"герой"_s);
        QCOMPARE(out[0].note, u"a note"_s);
        QVERIFY(!out[0].enabled);
        QCOMPARE(out[0].sourceMatchMode.mode.tag, MatchMode::Tag::Fuzzy);
        QCOMPARE(out[0].sourceMatchMode.mode.fuzzy.threshold, 0.5);
        QVERIFY(out[0].sourceMatchMode.case_sensitive);
        QCOMPARE(out[0].translationMatchMode.mode.tag, MatchMode::Tag::Both);
        QCOMPARE(out[0].translationMatchMode.mode.both.threshold, 0.25);
        QVERIFY(out[0].translationMatchMode.permissive);
        QCOMPARE(out[1].sourceMatchMode.mode.tag, MatchMode::Tag::Exact);
        QVERIFY(out[1].enabled);
    }

    void termsAreLockedUntilEdited() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s) });

        auto* const cell = actionCells(menu).first();
        auto* const source = cellEdits(menu, 0, 0).first();
        QVERIFY(source->isReadOnly());
        QVERIFY(!cell->editButton->isChecked());

        cell->editButton->setChecked(true);
        QVERIFY(!source->isReadOnly());

        cell->editButton->setChecked(false);
        QVERIFY(source->isReadOnly());
    }

    void aTermNeedsASourceAndATranslationToBeSaved() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s) });
        auto* const cell = actionCells(menu).first();
        auto lineEdits = cellEdits(menu, 0, 0);

        cell->editButton->setChecked(true);
        lineEdits[0]->clear();
        {
            ModalCatcher modal;
            cell->editButton->setChecked(false);
            QCOMPARE(modal.text, u"Empty term is not allowed."_s);
        }
        QVERIFY(cell->editButton->isChecked());

        lineEdits[0]->setText(u"hero"_s);
        cellEdits(menu, 0, 1).first()->clear();
        {
            ModalCatcher modal;
            cell->editButton->setChecked(false);
            QCOMPARE(modal.text, u"Empty term translation is not allowed."_s);
        }
        QVERIFY(cell->editButton->isChecked());
    }

    void addingATermMakesAnEditableEmptyRow() {
        GlossaryMenu menu;
        menu.show();

        menu.findChild<QPushButton*>(u"addTermButton"_s)->click();

        QCOMPARE(menu.glossary().size(), usize{ 1 });
        QVERIFY(menu.glossary()[0].term.isEmpty());
        QVERIFY(actionCells(menu).first()->editButton->isChecked());
    }

    void deletingATermAsksFirst() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s), termOf(u"king"_s, u"король"_s) });
        QSignalSpy removed(&menu, &GlossaryMenu::termRemoved);

        auto deleteButtonOf = [&](const i32 row) -> QPushButton* {
            return actionCells(menu).at(row)->findChildren<QPushButton*>().at(1);
        };

        {
            ModalCatcher modal(QMessageBox::No);
            deleteButtonOf(0)->click();
            QVERIFY2(modal.text.contains(u"delete this entry"_s), qPrintable(modal.text));
        }
        QCOMPARE(menu.glossary().size(), usize{ 2 });
        QCOMPARE(removed.count(), 0);

        {
            ModalCatcher modal(QMessageBox::Yes);
            deleteButtonOf(0)->click();
        }
        QCOMPARE(menu.glossary().size(), usize{ 1 });
        QCOMPARE(menu.glossary()[0].term, u"king"_s);
        QCOMPARE(removed.count(), 1);
    }

    void searchingTheGlossaryCyclesThroughTheMatches() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s), termOf(u"king"_s, u"король"_s), termOf(u"Hero's sword"_s, u"меч"_s) });
        auto* const tree = menu.findChild<QTreeWidget*>();
        auto* const input = menu.findChild<QLineEdit*>(u"searchInput"_s);
        auto* const search = menu.findChild<QPushButton*>(u"searchButton"_s);

        // Nothing to look for, or nothing to look in
        search->click();
        QVERIFY(tree->currentItem() == nullptr);

        input->setText(u"  HERO  "_s);
        search->click();
        QCOMPARE(tree->indexOfTopLevelItem(tree->currentItem()), 2);

        // The next search starts after the last match and wraps around
        search->click();
        QCOMPARE(tree->indexOfTopLevelItem(tree->currentItem()), 0);

        // The translation is looked in too
        input->setText(u"король"_s);
        search->click();
        QCOMPARE(tree->indexOfTopLevelItem(tree->currentItem()), 1);
    }

    void searchingForSomethingMissingSaysSo() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s) });
        menu.findChild<QLineEdit*>(u"searchInput"_s)->setText(u"dragon"_s);

        ModalCatcher modal;
        menu.findChild<QPushButton*>(u"searchButton"_s)->click();

        QVERIFY2(modal.text.contains(u"No match found for: dragon"_s), qPrintable(modal.text));
    }

    void searchingAnEmptyGlossaryDoesNothing() {
        GlossaryMenu menu;
        menu.findChild<QLineEdit*>(u"searchInput"_s)->setText(u"hero"_s);

        ModalCatcher modal;
        menu.findChild<QPushButton*>(u"searchButton"_s)->click();

        QVERIFY(modal.text.isEmpty());
    }

    void aTermCanBeSwitchedOffByItsPosition() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s), termOf(u"king"_s, u"король"_s) });

        menu.setTermEnabled(1, false);
        menu.setTermEnabled(7, false);

        QVERIFY(menu.glossary()[0].enabled);
        QVERIFY(!menu.glossary()[1].enabled);
    }

    void clearingTheGlossaryEmptiesIt() {
        GlossaryMenu menu;
        menu.fill({ termOf(u"hero"_s, u"герой"_s) });
        menu.findChild<QLineEdit*>(u"searchInput"_s)->setText(u"hero"_s);

        menu.clear();

        QVERIFY(menu.glossary().empty());
        QVERIFY(menu.findChild<QLineEdit*>(u"searchInput"_s)->text().isEmpty());

        QEvent change(QEvent::LanguageChange);
        QVERIFY(QApplication::sendEvent(&menu, &change));
    }

    void theFuzzyThresholdAppearsWithTheMatchMode() {
        GlossaryMenu menu;
        menu.show();
        menu.fill({ termOf(u"hero"_s, u"герой"_s) });
        actionCells(menu).first()->editButton->setChecked(true);

        auto* const tree = menu.findChild<QTreeWidget*>();
        auto* const modes = tree->itemWidget(tree->topLevelItem(0), 0)->findChild<QComboBox*>();
        auto* const threshold = cellEdits(menu, 0, 0).at(1);
        QVERIFY(threshold->isHidden());

        modes->setCurrentIndex(static_cast<i32>(MatchMode::Tag::Fuzzy));
        QVERIFY(!threshold->isHidden());
        QVERIFY(!threshold->text().isEmpty());

        // Unparsable text falls back to the default
        threshold->setText(u"0.3"_s);
        QCOMPARE(static_cast<f32>(menu.glossary()[0].sourceMatchMode.mode.fuzzy.threshold), 0.3F);

        threshold->clear();
        QCOMPARE(
            static_cast<f32>(menu.glossary()[0].sourceMatchMode.mode.fuzzy.threshold),
            DEFAULT_FUZZY_THRESHOLD
        );

        modes->setCurrentIndex(static_cast<i32>(MatchMode::Tag::Exact));
        QVERIFY(threshold->isHidden());
        QCOMPARE(menu.glossary()[0].sourceMatchMode.mode.tag, MatchMode::Tag::Exact);
    }

    // LintSelectMenu
    void theLintMenuStaysEmptyUntilItKnowsTheProject() {
        QWidget host;
        LintSelectMenu menu(&host);

        menu.refresh();

        QVERIFY(menu.findChildren<QGroupBox*>().isEmpty());
        QCOMPARE(menu.sizeHint(), QSize(520, 480));
    }

    void theLintMenuListsEveryCategory() {
        LintFixture fixture;
        fixture.menu.refresh();

        for (const QString& title : { u"Text Codes"_s, u"Note Tags"_s, u"Plugin Commands"_s, u"Comment Tags"_s, u"Custom Lints"_s }) {
            QVERIFY2(groupNamed(fixture.menu, title) != nullptr, qPrintable(title));
        }

        // There are no custom lints yet
        QVERIFY(!groupNamed(fixture.menu, u"Custom Lints"_s)->findChildren<QLabel*>().isEmpty());
        QVERIFY(allBoxOf(groupNamed(fixture.menu, u"Text Codes"_s)) != nullptr);
        QCOMPARE(allBoxOf(groupNamed(fixture.menu, u"Text Codes"_s))->checkState(), Qt::Checked);
    }

    void switchingALintOffDisablesItInTheSettings() {
        LintFixture fixture;
        fixture.menu.refresh();
        auto* const group = groupNamed(fixture.menu, u"Text Codes"_s);
        auto boxes = group->findChildren<QCheckBox*>();
        boxes.removeOne(allBoxOf(group));

        QVERIFY(!fixture.settings.translation.textCodes[0].disabled);
        boxes[0]->setChecked(false);
        QVERIFY(fixture.settings.translation.textCodes[0].disabled);
        QCOMPARE(allBoxOf(group)->checkState(), Qt::PartiallyChecked);

        boxes[0]->setChecked(true);
        QVERIFY(!fixture.settings.translation.textCodes[0].disabled);
        QCOMPARE(allBoxOf(group)->checkState(), Qt::Checked);
    }

    void theAllBoxSwitchesAWholeCategory() {
        LintFixture fixture;
        fixture.menu.refresh();
        auto* const group = groupNamed(fixture.menu, u"Note Tags"_s);
        auto* const all = allBoxOf(group);

        all->click();
        QCOMPARE(all->checkState(), Qt::Unchecked);
        QVERIFY(ranges::all_of(fixture.settings.translation.notetags, [](const SequenceSettings& entry) -> bool { return entry.disabled; }));

        all->click();
        QCOMPARE(all->checkState(), Qt::Checked);
        QVERIFY(ranges::none_of(fixture.settings.translation.notetags, [](const SequenceSettings& entry) -> bool { return entry.disabled; }));

        // From a mixed state it selects everything
        auto boxes = group->findChildren<QCheckBox*>();
        boxes.removeOne(all);
        boxes[0]->setChecked(false);
        QCOMPARE(all->checkState(), Qt::PartiallyChecked);
        all->click();
        QCOMPARE(all->checkState(), Qt::Checked);
        QVERIFY(!fixture.settings.translation.notetags[0].disabled);
    }

    void customLintsCanBeSwitchedOff() {
        LintFixture fixture;
        fixture.settings.translation.customLints.push_back(CustomLint{ .sequence = u"foo"_s, .tooltip = u"a foo"_s });
        fixture.menu.refresh();

        auto* const group = groupNamed(fixture.menu, u"Custom Lints"_s);
        auto boxes = group->findChildren<QCheckBox*>();
        boxes.removeOne(allBoxOf(group));

        QCOMPARE(boxes.size(), qsizetype{ 1 });
        QCOMPARE(boxes[0]->text(), u"foo"_s);
        QCOMPARE(boxes[0]->toolTip(), u"a foo"_s);

        boxes[0]->setChecked(false);
        QVERIFY(fixture.settings.translation.customLints[0].disabled);
    }

    void theActiveSaveIsChosenFromTheSavesOfTheProject() {
        LintFixture fixture;
        QVERIFY(QDir().mkpath(fixture.dir.path() + u"/save/slot"_s));
        writeFile(fixture.dir.path() + u"/save/file2.rpgsave"_s, "x");
        writeFile(fixture.dir.path() + u"/save/file1.rpgsave"_s, "x");
        writeFile(fixture.dir.path() + u"/save/slot/file3.rmmzsave"_s, "x");
        writeFile(fixture.dir.path() + u"/save/readme.txt"_s, "x");
        fixture.projectSettings.activeSave = u"save/file2.rpgsave"_s;
        fixture.menu.refresh();

        auto* const combo = fixture.menu.findChild<QComboBox*>();
        QCOMPARE(combo->count(), 4);
        QCOMPARE(combo->itemText(0), u"None"_s);
        QCOMPARE(combo->itemData(1).toString(), u"save/file1.rpgsave"_s);
        QCOMPARE(combo->currentData().toString(), u"save/file2.rpgsave"_s);

        combo->setCurrentIndex(0);
        QVERIFY(fixture.projectSettings.activeSave.isEmpty());

        combo->setCurrentIndex(3);
        QCOMPARE(fixture.projectSettings.activeSave, u"save/slot/file3.rmmzsave"_s);
    }

    void aSaveThatIsGoneFallsBackToNone() {
        LintFixture fixture;
        fixture.projectSettings.activeSave = u"save/missing.rpgsave"_s;
        fixture.menu.refresh();

        QCOMPARE(fixture.menu.findChild<QComboBox*>()->currentIndex(), 0);
    }

    void theTermsTabListsTheGlossary() {
        LintFixture fixture;
        fixture.menu.refresh();
        QVERIFY(ranges::any_of(fixture.menu.findChildren<QLabel*>(), [](const QLabel* const label) -> bool {
            return label->text() == u"The glossary is empty."_s;
        }));

        fixture.glossary.fill({ termOf(u"hero"_s, u"герой"_s, u"the hero"_s), termOf(u"king"_s, u"король"_s) });
        fixture.menu.refresh();

        auto* const group = groupNamed(fixture.menu, u"Terms"_s);
        QVERIFY(group != nullptr);

        auto boxes = group->findChildren<QCheckBox*>();
        boxes.removeOne(allBoxOf(group));
        QCOMPARE(boxes.size(), qsizetype{ 2 });
        QCOMPARE(boxes[0]->text(), u"hero - герой"_s);
        QCOMPARE(boxes[0]->toolTip(), u"the hero"_s);

        boxes[1]->setChecked(false);
        QVERIFY(fixture.glossary.glossary()[0].enabled);
        QVERIFY(!fixture.glossary.glossary()[1].enabled);

        allBoxOf(group)->click();
        QVERIFY(fixture.glossary.glossary()[1].enabled);
    }

    void clearingTheLintMenuForgetsTheProject() {
        LintFixture fixture;
        fixture.menu.show();
        fixture.menu.refresh();
        QVERIFY(groupNamed(fixture.menu, u"Text Codes"_s) != nullptr);

        fixture.menu.clear();

        QVERIFY(!fixture.menu.isVisible());
        QVERIFY(fixture.menu.findChildren<QGroupBox*>().isEmpty());

        // Without a project there is nothing to list
        fixture.menu.refresh();
        QVERIFY(fixture.menu.findChildren<QGroupBox*>().isEmpty());
    }

    void aLanguageChangeListsTheLintsAgain() {
        LintFixture fixture;
        fixture.menu.refresh();

        const MarkingTranslator translator;
        translator.retranslate(&fixture.menu);

        QCOMPARE(fixture.menu.findChild<QTabWidget*>()->tabText(0), MarkingTranslator::MARK + u"Lints"_s);
        QCOMPARE(fixture.menu.findChild<QTabWidget*>()->tabText(1), MarkingTranslator::MARK + u"Terms"_s);
        QVERIFY(groupNamed(fixture.menu, MarkingTranslator::MARK + u"Text Codes"_s) != nullptr);
    }

   private:
    static auto host(const SearchFixture& fixture) -> const QWidget& { return fixture.host; }
};

QTEST_MAIN(TestDialogs)
#include "tst_dialogs.moc"
