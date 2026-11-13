#include "BookmarkList.hpp"
#include "BookmarkMenu.hpp"
#include "MarkingTranslator.hpp"
#include "TabListModel.hpp"

#include <QComboBox>
#include <QSignalSpy>
#include <QtTest>

namespace {
constexpr auto ALL_FILES = u"- Filter by file -"_qsv;

auto comboOf(const BookmarkMenu& menu) -> QComboBox* {
    return menu.findChild<QComboBox*>();
}

auto listOf(const BookmarkMenu& menu) -> BookmarkList* {
    return menu.findChild<BookmarkList*>();
}

auto files() -> vector<TabListItem> {
    return { { .name = u"map001"_s, .properties = {} }, { .name = u"map002"_s, .properties = {} } };
}
}  // namespace

class TestBookmarkMenu final : public QObject {
    Q_OBJECT

   private slots:
    void startsHiddenWithOnlyTheAllFilesItem() {
        const BookmarkMenu menu;

        QVERIFY(menu.isHidden());
        QCOMPARE(comboOf(menu)->count(), 1);
        QCOMPARE(comboOf(menu)->itemText(0), QString(ALL_FILES));
        QCOMPARE(listOf(menu)->rowCount(), 0);
    }

    void initListsTheFilesAfterTheAllFilesItem() {
        BookmarkMenu menu;

        menu.init(files());

        QCOMPARE(comboOf(menu)->count(), 3);
        QCOMPARE(comboOf(menu)->itemText(1), u"map001"_s);
        QCOMPARE(comboOf(menu)->itemText(2), u"map002"_s);
    }

    void addingKeepsTheFields() {
        BookmarkMenu menu;

        menu.addBookmark(u"Boss door"_qsv, u"map001"_qsv, 4, u"Entry"_qsv);

        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->rowCount(), 1);
        QCOMPARE(list->bookmark(0).description, u"Boss door"_s);
        QCOMPARE(list->bookmark(0).entry, u"Entry"_s);
        QCOMPARE(list->bookmark(0).row, u32{ 4 });
    }

    void addingBeforeABookmarkPushesItDown() {
        BookmarkMenu menu;
        menu.addBookmark(u"old"_qsv, u"map001"_qsv, 5);
        menu.addBookmark(u"other file"_qsv, u"map002"_qsv, 5);
        menu.addBookmark(u"above"_qsv, u"map001"_qsv, 2);

        // The new bookmark of map001 at row 2 moves only the map001 bookmark at row 5 and above
        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->bookmark(0).row, u32{ 6 });
        QCOMPARE(list->bookmark(1).row, u32{ 5 });
        QCOMPARE(list->bookmark(2).row, u32{ 2 });
    }

    void addingAtTheSameRowPushesTheOldOneDown() {
        BookmarkMenu menu;
        menu.addBookmark(u"old"_qsv, u"map001"_qsv, 5);

        menu.addBookmark(u"new"_qsv, u"map001"_qsv, 5);

        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->bookmark(0).row, u32{ 6 });
        QCOMPARE(list->bookmark(1).row, u32{ 5 });
    }

    void shiftingUpAfterARowWasRemoved() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 3);
        menu.addBookmark(u"b"_qsv, u"map001"_qsv, 8);
        menu.addBookmark(u"c"_qsv, u"map002"_qsv, 8);

        menu.shiftIndices(u"map001"_qsv, 3);

        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->bookmark(0).row, u32{ 3 });
        QCOMPARE(list->bookmark(1).row, u32{ 7 });
        QCOMPARE(list->bookmark(2).row, u32{ 8 });
    }

    void filteringByFileHidesTheOthers() {
        BookmarkMenu menu;
        menu.init(files());
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 0);
        menu.addBookmark(u"b"_qsv, u"map002"_qsv, 0);
        menu.addBookmark(u"c"_qsv, u"map001"_qsv, 9);

        comboOf(menu)->setCurrentIndex(1);

        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->model()->rowCount(), 2);
        QCOMPARE(list->rowCount(), 3);

        comboOf(menu)->setCurrentIndex(2);
        QCOMPARE(list->model()->rowCount(), 1);

        comboOf(menu)->setCurrentIndex(0);
        QCOMPARE(list->model()->rowCount(), 3);
    }

    void aBookmarkOfAnotherFileStaysOutOfTheFilter() {
        BookmarkMenu menu;
        menu.init(files());
        comboOf(menu)->setCurrentIndex(1);

        menu.addBookmark(u"a"_qsv, u"map002"_qsv, 0);
        menu.addBookmark(u"b"_qsv, u"map001"_qsv, 1);

        QCOMPARE(listOf(menu)->rowCount(), 2);
        QCOMPARE(listOf(menu)->model()->rowCount(), 1);
    }

    void updatingTouchesOnlyTheBookmarkOfThatFileAndRow() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 3);
        menu.addBookmark(u"b"_qsv, u"map002"_qsv, 3);
        menu.addBookmark(u"c"_qsv, u"map001"_qsv, 4);

        menu.updateBookmark(u"map001"_qsv, 3, u"changed"_s);

        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->bookmark(0).description, u"changed"_s);
        QCOMPARE(list->bookmark(1).description, u"b"_s);
        QCOMPARE(list->bookmark(2).description, u"c"_s);
    }

    void removingTouchesOnlyTheBookmarkOfThatFileAndRow() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 3);
        menu.addBookmark(u"b"_qsv, u"map002"_qsv, 3);
        menu.addBookmark(u"c"_qsv, u"map001"_qsv, 4);

        menu.removeBookmark(u"map001"_qsv, 3);

        const BookmarkList* const list = listOf(menu);
        QCOMPARE(list->rowCount(), 2);
        QCOMPARE(list->bookmark(0).description, u"b"_s);
        QCOMPARE(list->bookmark(1).description, u"c"_s);
    }

    void removingAnUnknownBookmarkChangesNothing() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 3);

        menu.removeBookmark(u"map001"_qsv, 9);
        menu.removeBookmark(u"map002"_qsv, 3);

        QCOMPARE(listOf(menu)->rowCount(), 1);
    }

    void clearingDropsTheBookmarksAndTheFiles() {
        BookmarkMenu menu;
        menu.init(files());
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 3);

        menu.clear();

        QCOMPARE(listOf(menu)->rowCount(), 0);
        QCOMPARE(comboOf(menu)->count(), 1);
        QCOMPARE(comboOf(menu)->itemText(0), QString(ALL_FILES));
    }

    void showingEntryNamesIsPassedToTheList() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 3, u"Entry"_qsv);

        menu.setShowEntryNames(true);
        QVERIFY(listOf(menu)->showEntryNames());

        menu.setShowEntryNames(false);
        QVERIFY(!listOf(menu)->showEntryNames());
    }

    void clickingABookmarkIsForwarded() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 5);
        menu.addBookmark(u"b"_qsv, u"map002"_qsv, 9);
        menu.resize(300, 300);
        menu.show();
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        const BookmarkList* const list = listOf(menu);
        QSignalSpy clicked(&menu, &BookmarkMenu::bookmarkClicked);

        QTest::mouseClick(
            list->viewport(),
            Qt::LeftButton,
            Qt::NoModifier,
            list->visualRect(list->model()->index(1, 0)).center()
        );

        QCOMPARE(clicked.size(), qsizetype{ 1 });
        QCOMPARE(clicked.at(0).at(0).value<QL1SV>(), u"map002"_qsv);
        QCOMPARE(clicked.at(0).at(1).toUInt(), 9U);
    }

    void aLanguageChangeRetranslatesTheAllFilesItemAndKeepsTheBookmarks() {
        BookmarkMenu menu;
        menu.addBookmark(u"a"_qsv, u"map001"_qsv, 5);
        const MarkingTranslator translator;
        translator.retranslate(&menu);

        QCOMPARE(comboOf(menu)->itemText(0), MarkingTranslator::MARK + QString(ALL_FILES));
        QCOMPARE(listOf(menu)->rowCount(), 1);
    }
};

QTEST_MAIN(TestBookmarkMenu)
#include "tst_bookmarkmenu.moc"
