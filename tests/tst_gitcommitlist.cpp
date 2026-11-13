#include "Pointer.hpp"
#include "GitCommitList.hpp"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QtTest>

namespace {
auto commitOf(const QString& hash, const QString& message, const bool remote = false) -> GitCommit {
    return GitCommit{ .hash = hash, .message = message, .author = u"Ann"_s, .isRemote = remote };
}

// Whether anything but one flat color was drawn
auto isFlat(const QImage& image) -> bool {
    const QRgb first = image.pixel(0, 0);

    for (i32 row = 0; row < image.height(); row++) {
        for (i32 col = 0; col < image.width(); col++) {
            if (image.pixel(col, row) != first) {
                return false;
            }
        }
    }

    return true;
}

void showList(GitCommitList& list) {
    list.resize(300, 300);
    list.show();
    QVERIFY(QTest::qWaitForWindowExposed(&list));
}

auto centerOf(GitCommitList& list, const i32 row) -> QPoint {
    return list.visualRect(list.model()->index(row)).center();
}
}  // namespace

class TestGitCommitList final : public QObject {
    Q_OBJECT

   private slots:
    void startsEmpty() {
        const GitCommitModel model;

        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!model.data(QModelIndex()).isValid());
    }

    void commitsAreGivenByRole() {
        GitCommitModel model;
        model.appendCommit(commitOf(u"abc1234"_s, u"Fix the thing"_s, true));

        const QModelIndex idx = model.index(0);

        QCOMPARE(model.data(idx).toString(), u"Fix the thing"_s);
        QCOMPARE(model.data(idx, Role::Message).toString(), u"Fix the thing"_s);
        QCOMPARE(model.data(idx, Role::Hash).toString(), u"abc1234"_s);
        QCOMPARE(model.data(idx, Role::Author).toString(), u"Ann"_s);
        QVERIFY(model.data(idx, Role::IsRemote).toBool());
        QVERIFY(!model.data(idx, Qt::DecorationRole).isValid());
        QCOMPARE(model.commitAt(0).hash, u"abc1234"_s);
    }

    void newCommitsGoFirstAndOldOnesLast() {
        GitCommitModel model;
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);

        model.appendCommit(commitOf(u"2"_s, u"second"_s));
        model.prependCommit(commitOf(u"1"_s, u"first"_s));
        model.appendCommit(commitOf(u"3"_s, u"third"_s));

        QCOMPARE(inserted.size(), qsizetype{ 3 });
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.commitAt(0).hash, u"1"_s);
        QCOMPARE(model.commitAt(1).hash, u"2"_s);
        QCOMPARE(model.commitAt(2).hash, u"3"_s);
    }

    void clearingResetsTheModel() {
        GitCommitModel model;
        model.appendCommit(commitOf(u"1"_s, u"first"_s));
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);

        model.clear();

        QCOMPARE(reset.size(), qsizetype{ 1 });
        QCOMPARE(model.rowCount(), 0);
    }

    void modelObeysTheModelContract() {
        GitCommitModel model;
        const QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        model.appendCommit(commitOf(u"1"_s, u"a"_s));
        model.prependCommit(commitOf(u"2"_s, u"b"_s));
        model.clear();
    }

    void everyRowIsEquallyTall() {
        GitCommitList list;
        list.appendCommit(commitOf(u"1"_s, u"a"_s));
        list.appendCommit(commitOf(u"2"_s, u"a much longer message that is not wrapped"_s, true));
        const GitCommitDelegate delegate;

        const QSize first = delegate.sizeHint(QStyleOptionViewItem(), list.model()->index(0));
        const QSize second = delegate.sizeHint(QStyleOptionViewItem(), list.model()->index(1));

        QVERIFY(first.height() > 0);
        QCOMPARE(first.height(), second.height());
    }

    void paintingCoversLocalRemoteAndSelectedCommits() {
        GitCommitList list;
        list.appendCommit(commitOf(u"abcdef0123456789"_s, u"A message long enough that it has to be elided to fit the row"_s, true));
        list.appendCommit(commitOf(u"1234567"_s, u"Local"_s));
        showList(list);
        list.setCurrentIndex(list.model()->index(0));

        QVERIFY(!isFlat(list.grab().toImage()));
    }

    void hoveringACommitReportsItAfterAWhile() {
        GitCommitList list;
        list.appendCommit(commitOf(u"1"_s, u"first"_s));
        list.appendCommit(commitOf(u"2"_s, u"second"_s));
        showList(list);
        QSignalSpy hovered(&list, &GitCommitList::commitHovered);

        hoverAt(list.viewport(), centerOf(list, 1));

        // Not at once: a mouse passing over the list would show a tooltip for each row
        QCOMPARE(hovered.size(), qsizetype{ 0 });
        QVERIFY(hovered.wait(2000));
        QCOMPARE(hovered.at(0).at(0).value<QModelIndex>().row(), 1);
    }

    void movingOnBeforeTheDelayReportsOnlyTheLastCommit() {
        GitCommitList list;
        list.appendCommit(commitOf(u"1"_s, u"first"_s));
        list.appendCommit(commitOf(u"2"_s, u"second"_s));
        showList(list);
        QSignalSpy hovered(&list, &GitCommitList::commitHovered);

        hoverAt(list.viewport(), centerOf(list, 0));
        hoverAt(list.viewport(), centerOf(list, 1));

        QVERIFY(hovered.wait(2000));
        QCOMPARE(hovered.size(), qsizetype{ 1 });
        QCOMPARE(hovered.at(0).at(0).value<QModelIndex>().row(), 1);
    }

    void leavingTheListCancelsAPendingHover() {
        GitCommitList list;
        list.appendCommit(commitOf(u"1"_s, u"first"_s));
        showList(list);
        QSignalSpy hovered(&list, &GitCommitList::commitHovered);

        hoverAt(list.viewport(), centerOf(list, 0));
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&list, &leave);

        QVERIFY(!hovered.wait(1000));
    }

    void hoveringEmptySpaceReportsNothing() {
        GitCommitList list;
        list.appendCommit(commitOf(u"1"_s, u"first"_s));
        showList(list);
        QSignalSpy hovered(&list, &GitCommitList::commitHovered);

        hoverAt(list.viewport(), QPoint(150, 280));

        QVERIFY(!hovered.wait(1000));
    }
};

QTEST_MAIN(TestGitCommitList)
#include "tst_gitcommitlist.moc"
