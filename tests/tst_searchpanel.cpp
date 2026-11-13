#include "SearchPanelDock.hpp"
#include "MarkingTranslator.hpp"

#include "ProjectSettings.hpp"
#include "TabListModel.hpp"
#include "Utils.hpp"

#include <QAbstractItemModelTester>
#include <QApplication>
#include <QComboBox>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTimer>
#include <QtTest>

Q_DECLARE_METATYPE(SearchPanelDock::Action)

namespace {
// One result: the text of the cell, where it is, and the part of the text that matched
struct Hit {
    QString text;
    u32 row;
    u8 col;
    QString entry = QString();
};

auto matchesOf(const QString& file, const vector<Hit>& hits) -> CellMatches {
    FileMatches matches;

    for (const Hit& hit : hits) {
        CellMatch cell{};
        cell.entryIndex = NO_ENTRY;
        cell.matchIndex = MatchIndex(static_cast<i32>(hit.row), hit.col);

        matches.setText(cell, hit.text);
        matches.setEntry(cell, hit.entry);

        const array<TextMatch, 1> found = { TextMatch(0, 3, false) };
        matches.setMatches(cell, found);
        matches.cells.push_back(cell);
    }

    CellMatches all;
    FilenameArray name{};
    copyFilenameArray(file.toLatin1().data(), file.size(), name.data());
    all.emplace(name, std::move(matches));
    return all;
}

auto mergedMatches(CellMatches first, CellMatches second) -> CellMatches {
    for (auto& [name, matches] : second) {
        first.emplace(name, std::move(matches));
    }

    return first;
}

// The panel as the main window wires it: a window with a status bar around it, the file filter, the list and the
// clear button
struct Fixture {
    Fixture() : list(new SearchResultList), fileSelect(new QComboBox), clearButton(new QPushButton) {
        window.addDockWidget(Qt::RightDockWidgetArea, &panel);
        window.statusBar();

        // The panel is made before the controls that belong to it are known
        panel.init(fileSelect.get(), list.get(), clearButton.get());
        panel.clear();

        vector<TabListItem> files;
        files.push_back(TabListItem{ u"map001"_s, TabListItemProps(1, 0, false) });
        files.push_back(TabListItem{ u"map002"_s, TabListItemProps(1, 0, false) });
        panel.setFiles(files);

        settings.columns = { ColumnInfo{ u"Source"_s, 100 }, ColumnInfo{ u"English"_s, 100 }, ColumnInfo{ u"German"_s, 100 } };

        list->resize(300, 400);
        list->show();
    }

    [[nodiscard]] auto model() const -> SearchResultListModel* { return list->model(); }

    [[nodiscard]] auto shownRows() const -> i32 { return list->proxy()->rowCount(); }

    [[nodiscard]] auto subtitleOf(const i32 row) const -> QString {
        return model()->data(model()->index(row), SearchResultListRole::SubtitleRole).toString();
    }

    [[nodiscard]] auto titleOf(const i32 row) const -> QString {
        return model()->data(model()->index(row), SearchResultListRole::TitleRole).toString();
    }

    // Clicks the item as the list reports it, with the button
    void click(const Qt::MouseButton button, const i32 row) const {
        emit list->clicked(button, list->proxy()->index(row, 0));
    }

    QMainWindow window;
    SearchPanelDock panel;
    ProjectSettings settings;
    unique_ptr<SearchResultList> list;
    unique_ptr<QComboBox> fileSelect;
    unique_ptr<QPushButton> clearButton;
};

// Answers the box or menu a click opens, from the event loop it runs in. The second timer closes everything if there
// was none, rather than hanging the test.
void answerBox(const QMessageBox::StandardButton button) {
    QTimer::singleShot(0, [button]() -> void {
        if (auto* const box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            box->button(button)->click();
        }
    });
    QTimer::singleShot(3000, []() -> void { QApplication::closeAllWindows(); });
}

void triggerMenuItem(const i32 index) {
    QTimer::singleShot(0, [index]() -> void {
        if (auto* const menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
            menu->actions().at(index)->trigger();
            menu->close();
        }
    });
    QTimer::singleShot(3000, []() -> void {
        if (QWidget* const popup = QApplication::activePopupWidget()) {
            popup->close();
        }
    });
}
}  // namespace

class TestSearchPanel final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() { qRegisterMetaType<SearchPanelDock::Action>(); }

    void resultsAreInTheOrderOfTheTabs() {
        Fixture fixture;

        fixture.panel.showMatches(
            mergedMatches(matchesOf(u"map002"_s, { Hit{ u"second file"_s, 0, 1 } }), matchesOf(u"map001"_s, { Hit{ u"first file"_s, 0, 1 } })),
            &fixture.settings
        );

        QCOMPARE(fixture.model()->rowCount(), 2);
        QCOMPARE(fixture.titleOf(0), u"first file"_s);
        QCOMPARE(fixture.titleOf(1), u"second file"_s);
    }

    void subtitlesNameTheFileRowAndColumn() {
        Fixture fixture;

        fixture.panel.showMatches(
            matchesOf(
                u"map001"_s,
                { Hit{ u"in the source"_s, 4, 0 },
                  Hit{ u"in english"_s, 9, 1, u"Harold"_s },
                  Hit{ u"far column"_s, 0, 7 } }
            ),
            &fixture.settings
        );

        QCOMPARE(fixture.subtitleOf(0), u"File map001 / Row 5 / Column 1 (Source)"_s);
        QCOMPARE(fixture.subtitleOf(1), u"File map001 / Row 10 / Column 2 (English) / Entry Harold"_s);
        // A column the settings don't name is told by its number
        QCOMPARE(fixture.subtitleOf(2), u"File map001 / Row 1 / Column 8 (8)"_s);
    }

    void titlesShowLineBreaksAsTheirSymbol() {
        Fixture fixture;

        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"one\ntwo"_s, 0, 1 } }), &fixture.settings);

        QCOMPARE(fixture.titleOf(0), u"one"_s + QString(LINE_SEPARATOR) + u"two"_s);
    }

    void roleDataFollowsTheMatch() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"hello"_s, 0, 1 } }), &fixture.settings);

        const QModelIndex idx = fixture.model()->index(0);
        const auto spans = fixture.model()->data(idx, SearchResultListRole::SpansRole).value<span<const TextMatch>>();

        QCOMPARE(spans.size(), usize{ 1 });
        QCOMPARE(spans[0].get<"len">(), u32{ 3 });
        QVERIFY(!fixture.model()->data(idx, Qt::DecorationRole).isValid());
        QVERIFY(!fixture.model()->data(QModelIndex(), SearchResultListRole::TitleRole).isValid());
        QVERIFY(!fixture.model()->data(fixture.model()->index(0).siblingAtRow(5), SearchResultListRole::TitleRole).isValid());
        QCOMPARE(fixture.model()->flags(idx), Qt::ItemIsEnabled);
        QCOMPARE(fixture.model()->flags(QModelIndex()), Qt::NoItemFlags);
    }

    void modelObeysTheModelContract() {
        Fixture fixture;
        const QAbstractItemModelTester tester(fixture.model(), QAbstractItemModelTester::FailureReportingMode::QtTest);

        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 }, Hit{ u"b"_s, 1, 1 } }), &fixture.settings);
        fixture.panel.clearList();
    }

    void theFileFilterHidesTheOtherFiles() {
        Fixture fixture;
        fixture.panel.showMatches(
            mergedMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), matchesOf(u"map002"_s, { Hit{ u"b"_s, 0, 1 }, Hit{ u"c"_s, 1, 1 } })),
            &fixture.settings
        );
        QCOMPARE(fixture.shownRows(), 3);

        fixture.fileSelect->setCurrentIndex(2);
        QCOMPARE(fixture.shownRows(), 2);

        fixture.fileSelect->setCurrentIndex(1);
        QCOMPARE(fixture.shownRows(), 1);

        fixture.fileSelect->setCurrentIndex(0);
        QCOMPARE(fixture.shownRows(), 3);
    }

    // The old filter would hide nothing of the new results while the box still showed it
    void newResultsResetTheFileFilter() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);
        fixture.fileSelect->setCurrentIndex(2);

        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);

        QCOMPARE(fixture.fileSelect->currentIndex(), 0);
        QCOMPARE(fixture.shownRows(), 1);
    }

    void clearingEmptiesTheListAndTheFileFilter() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);
        QCOMPARE(fixture.fileSelect->count(), 3);

        fixture.panel.clear();

        QCOMPARE(fixture.model()->rowCount(), 0);
        QCOMPARE(fixture.fileSelect->count(), 1);
        QCOMPARE(fixture.fileSelect->itemText(0), u"- Filter by file -"_s);
    }

    void theClearButtonEmptiesTheList() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);

        fixture.clearButton->click();

        QCOMPARE(fixture.model()->rowCount(), 0);
        // The files stay for the next search
        QCOMPARE(fixture.fileSelect->count(), 3);
    }

    void thePlaceholderFollowsTheLanguage() {
        Fixture fixture;

        const MarkingTranslator translator;
        translator.retranslate(&fixture.panel);

        QCOMPARE(fixture.fileSelect->itemText(0), MarkingTranslator::MARK + u"- Filter by file -"_s);
    }

    void generationsCountTheRefills() {
        Fixture fixture;
        const u32 before = fixture.panel.generation();

        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);
        const u32 filled = fixture.panel.generation();
        fixture.panel.clearList();

        QVERIFY(filled > before);
        QVERIFY(fixture.panel.generation() > filled);
    }

    void aChangedItemIsReportedToTheView() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);
        QSignalSpy changed(fixture.model(), &QAbstractItemModel::dataChanged);

        fixture.panel.itemChanged(fixture.model()->item(0));

        QCOMPARE(changed.size(), qsizetype{ 1 });
    }

    void leftClickGoesToTheResult() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 6, 2 } }), &fixture.settings);
        QSignalSpy requested(&fixture.panel, &SearchPanelDock::actionRequested);

        fixture.click(Qt::LeftButton, 0);

        QCOMPARE(requested.size(), qsizetype{ 1 });
        QCOMPARE(requested.at(0).at(0).value<SearchPanelDock::Action>(), SearchPanelDock::Action::GoTo);
        QCOMPARE(requested.at(0).at(1).toString(), u"map001"_s);
        QCOMPARE(requested.at(0).at(2).toUInt(), 6U);
        QCOMPARE(requested.at(0).at(3).toUInt(), 2U);
    }

    void rightClickReplacesOnceAndRefusesTheSource() {
        Fixture fixture;
        fixture.panel.showMatches(
            matchesOf(u"map001"_s, { Hit{ u"source"_s, 0, 0 }, Hit{ u"translation"_s, 1, 1 } }),
            &fixture.settings
        );
        QSignalSpy requested(&fixture.panel, &SearchPanelDock::actionRequested);

        fixture.click(Qt::RightButton, 0);
        QCOMPARE(requested.size(), qsizetype{ 0 });
        QCOMPARE(fixture.window.statusBar()->currentMessage(), u"Source column cannot be replaced."_s);

        fixture.click(Qt::RightButton, 1);
        QCOMPARE(requested.size(), qsizetype{ 1 });
        QCOMPARE(requested.at(0).at(0).value<SearchPanelDock::Action>(), SearchPanelDock::Action::Replace);
        QVERIFY(fixture.model()->item(1).replaced);

        fixture.click(Qt::RightButton, 1);
        QCOMPARE(requested.size(), qsizetype{ 1 });
        QCOMPARE(fixture.window.statusBar()->currentMessage(), u"This result was already replaced."_s);
    }

    void middleClickOnATranslationAsksBeforePuttingTheTextInPlace() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 3, 1 } }), &fixture.settings);
        QSignalSpy requested(&fixture.panel, &SearchPanelDock::actionRequested);

        QTest::ignoreMessage(QtInfoMsg, "Put-in-place replacement was rejected by user.");
        answerBox(QMessageBox::No);
        fixture.click(Qt::MiddleButton, 0);
        QCOMPARE(requested.size(), qsizetype{ 0 });
        QVERIFY(!fixture.model()->item(0).replaced);

        answerBox(QMessageBox::Yes);
        fixture.click(Qt::MiddleButton, 0);
        QCOMPARE(requested.size(), qsizetype{ 1 });
        QCOMPARE(requested.at(0).at(0).value<SearchPanelDock::Action>(), SearchPanelDock::Action::Put);
        QCOMPARE(requested.at(0).at(3).toUInt(), 1U);
        QVERIFY(fixture.model()->item(0).replaced);
    }

    void middleClickOnASourcePicksTheColumnToPutInto() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 3, 0 } }), &fixture.settings);
        QSignalSpy requested(&fixture.panel, &SearchPanelDock::actionRequested);

        // The menu lists the columns after the source: English, German
        triggerMenuItem(1);
        fixture.click(Qt::MiddleButton, 0);

        QCOMPARE(requested.size(), qsizetype{ 1 });
        QCOMPARE(requested.at(0).at(0).value<SearchPanelDock::Action>(), SearchPanelDock::Action::Put);
        QCOMPARE(requested.at(0).at(2).toUInt(), 3U);
        QCOMPARE(requested.at(0).at(3).toUInt(), 2U);
    }

    void clickingOutsideTheResultsDoesNothing() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);
        QSignalSpy requested(&fixture.panel, &SearchPanelDock::actionRequested);

        emit fixture.list->clicked(Qt::LeftButton, QModelIndex());

        QCOMPARE(requested.size(), qsizetype{ 0 });
    }

    void clickingTheViewReportsTheButtonAndTheItem() {
        Fixture fixture;
        fixture.panel.showMatches(matchesOf(u"map001"_s, { Hit{ u"a"_s, 0, 1 } }), &fixture.settings);
        QVERIFY(QTest::qWaitForWindowExposed(fixture.list.get()));
        QSignalSpy clicked(fixture.list.get(), &SearchResultList::clicked);

        const QPoint point = fixture.list->visualRect(fixture.list->proxy()->index(0, 0)).center();
        QTest::mouseClick(fixture.list->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        QTest::mouseClick(fixture.list->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(150, 390));

        QCOMPARE(clicked.size(), qsizetype{ 1 });
        QCOMPARE(clicked.at(0).at(0).value<Qt::MouseButton>(), Qt::LeftButton);
    }

    void longTextsMakeTallerItemsAndTheyPaint() {
        Fixture fixture;
        fixture.panel.showMatches(
            matchesOf(u"map001"_s, { Hit{ u"short"_s, 0, 1 }, Hit{ u"A long sentence that has to wrap over a few lines of the list. "_s.repeated(6), 1, 1 } }),
            &fixture.settings
        );
        QVERIFY(QTest::qWaitForWindowExposed(fixture.list.get()));

        const auto longHeight = [&fixture]() -> i32 {
            return fixture.list->visualRect(fixture.list->proxy()->index(1, 0)).height();
        };

        QTRY_VERIFY(longHeight() > fixture.list->visualRect(fixture.list->proxy()->index(0, 0)).height());

        // A narrower list lays the items out again, and the long text wraps over more lines
        const i32 wideHeight = longHeight();
        fixture.list->resize(150, 400);
        QTRY_VERIFY(longHeight() > wideHeight);
    }
};

QTEST_MAIN(TestSearchPanel)
#include "tst_searchpanel.moc"
