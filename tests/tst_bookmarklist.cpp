#include "BookmarkList.hpp"

#include <QSignalSpy>
#include <QtTest>

namespace {
// The view is shown, as it only knows where its items are once it is laid out
void showList(BookmarkList& list) {
    list.resize(300, 300);
    list.show();
    QVERIFY(QTest::qWaitForWindowExposed(&list));
}

auto centerOf(const BookmarkList& list, const i32 viewRow) -> QPoint {
    return list.visualRect(list.model()->index(viewRow, 0)).center();
}
}  // namespace

class TestBookmarkList final : public QObject {
    Q_OBJECT

   private slots:
    void startsEmpty() {
        const BookmarkList list(nullptr);

        QCOMPARE(list.rowCount(), 0);
        QCOMPARE(list.model()->rowCount(), 0);
        QVERIFY(!list.showEntryNames());
    }

    void appendingKeepsTheFieldsInPlace() {
        const BookmarkList list(nullptr);

        list.appendRow(u"Boss door"_qsv, u"map001"_qsv, 41, u"Entry name"_qsv);

        QCOMPARE(list.rowCount(), 1);
        QCOMPARE(list.model()->rowCount(), 1);

        const Bookmark& bookmark = list.bookmark(0);
        QCOMPARE(bookmark.description, u"Boss door"_s);
        QCOMPARE(bookmark.entry, u"Entry name"_s);
        QCOMPARE(bookmark.row, u32{ 41 });
        QVERIFY(!bookmark.hidden);
        QCOMPARE(QL1SV(bookmark.filename.data()), u"map001"_qsv);
    }

    void hiddenBookmarksLeaveTheViewOnRefilter() {
        const BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 0, QStringView());
        list.appendRow(u"b"_qsv, u"map002"_qsv, 1, QStringView());
        list.appendRow(u"c"_qsv, u"map003"_qsv, 2, QStringView());

        list.setRowHidden(1, true);

        // The proxy has not been asked to filter yet
        QCOMPARE(list.model()->rowCount(), 3);

        list.refilter();
        QCOMPARE(list.model()->rowCount(), 2);
        QCOMPARE(list.rowCount(), 3);

        list.setRowHidden(1, false);
        list.refilter();
        QCOMPARE(list.model()->rowCount(), 3);
    }

    void removingARowRemovesTheBookmark() {
        const BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 0, QStringView());
        list.appendRow(u"b"_qsv, u"map002"_qsv, 1, QStringView());

        list.removeRow(0);

        QCOMPARE(list.rowCount(), 1);
        QCOMPARE(list.bookmark(0).description, u"b"_s);
    }

    void clearingEmptiesTheList() {
        const BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 0, QStringView());

        list.clear();

        QCOMPARE(list.rowCount(), 0);
        QCOMPARE(list.model()->rowCount(), 0);
    }

    void clickingABookmarkNamesItsFileAndRow() {
        BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 5, QStringView());
        list.appendRow(u"b"_qsv, u"map002"_qsv, 9, QStringView());
        showList(list);
        QSignalSpy clicked(&list, &BookmarkList::bookmarkClicked);

        QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier, centerOf(list, 1));

        QCOMPARE(clicked.size(), qsizetype{ 1 });
        QCOMPARE(clicked.at(0).at(0).value<QL1SV>(), u"map002"_qsv);
        QCOMPARE(clicked.at(0).at(1).toUInt(), 9U);
    }

    // The view shows the filtered rows, so its second row is not the bookmark in the second place
    void clickingFollowsTheFilter() {
        BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 0, QStringView());
        list.appendRow(u"b"_qsv, u"map002"_qsv, 1, QStringView());
        list.appendRow(u"c"_qsv, u"map003"_qsv, 2, QStringView());
        list.setRowHidden(0, true);
        list.refilter();
        showList(list);
        QSignalSpy clicked(&list, &BookmarkList::bookmarkClicked);

        QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier, centerOf(list, 1));

        QCOMPARE(clicked.size(), qsizetype{ 1 });
        QCOMPARE(clicked.at(0).at(1).toUInt(), 2U);
    }

    void onlyTheLeftButtonOnABookmarkClicks() {
        BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 0, QStringView());
        showList(list);
        QSignalSpy clicked(&list, &BookmarkList::bookmarkClicked);

        QTest::mouseClick(list.viewport(), Qt::RightButton, Qt::NoModifier, centerOf(list, 0));
        QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(150, 280));

        QCOMPARE(clicked.size(), qsizetype{ 0 });
    }

    void theEntryNamesArePaintedOnlyWhenAsked() {
        BookmarkList list(nullptr);
        list.appendRow(u"A rather long description that has to be elided to fit"_qsv, u"map001"_qsv, 0, u"Entry"_qsv);
        list.appendRow(u"No entry"_qsv, u"map002"_qsv, 1, QStringView());
        showList(list);

        const QImage without = list.grab().toImage();

        list.setShowEntryNames(true);
        QVERIFY(list.showEntryNames());
        const QImage with = list.grab().toImage();

        QVERIFY(with != without);

        list.setShowEntryNames(false);
        QCOMPARE(list.grab().toImage(), without);
    }

    void itemsAreTwoLinesTall() {
        BookmarkList list(nullptr);
        list.appendRow(u"a"_qsv, u"map001"_qsv, 0, QStringView());
        showList(list);

        const QRect rect = list.visualRect(list.model()->index(0, 0));

        QVERIFY(rect.height() >= list.fontMetrics().height() * 2);
    }
};

QTEST_MAIN(TestBookmarkList)
#include "tst_bookmarklist.moc"
