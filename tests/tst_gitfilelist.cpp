// The list of changed files of the source control dock. What it holds decides what is committed: the commit takes the
// staged items of the model.
#include "Pointer.hpp"
#include "GitFileList.hpp"

#ifdef ENABLE_LIBGIT2
#include <QAbstractItemModelTester>
#include <QImage>
#include <QSignalSpy>
#include <QtTest>

namespace {
using HoverButton = GitFileDelegate::HoverButton;

constexpr QRect ROW_RECT(0, 0, 300, 24);

[[nodiscard]] auto itemOf(const QString& path, const git_status_t status = GIT_STATUS_WT_MODIFIED) -> GitFileItem {
    return GitFileItem{ .filePath = path, .status = status };
}

// The x of the middle of each button of the row at its middle height, found by walking the row: the order of the
// buttons is the contract here, not their exact pixels
struct ButtonSpots {
    QPoint stage;
    QPoint revert;
    QPoint diff;
};

[[nodiscard]] auto spotsOf(const QRect& rect) -> ButtonSpots {
    ButtonSpots spots;
    array<i32, 3> first = { -1, -1, -1 };
    array<i32, 3> last = { -1, -1, -1 };

    for (i32 xPos = rect.left(); xPos <= rect.right(); xPos++) {
        const auto hit = GitFileDelegate::hitTestButton(rect, QPoint(xPos, rect.center().y()));

        if (hit == HoverButton::None) {
            continue;
        }

        const auto slot = static_cast<usize>(hit) - 1;
        first[slot] = first[slot] < 0 ? xPos : first[slot];
        last[slot] = xPos;
    }

    spots.stage = QPoint((first[0] + last[0]) / 2, rect.center().y());
    spots.revert = QPoint((first[1] + last[1]) / 2, rect.center().y());
    spots.diff = QPoint((first[2] + last[2]) / 2, rect.center().y());
    return spots;
}

[[nodiscard]] auto release(const QPoint& pos, const Qt::MouseButton button = Qt::LeftButton) -> QMouseEvent {
    return QMouseEvent(QEvent::MouseButtonRelease, pos, pos, button, Qt::NoButton, Qt::NoModifier);
}

// Clicks `text` in the context menu the list opens, a moment after it blocks
void chooseInMenu(const QString& text) {
    QTimer::singleShot(100, [text] -> void {
        auto* const menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());

        if (menu == nullptr) {
            return;
        }

        for (QAction* const action : menu->actions()) {
            if (action->text() == text) {
                menu->setActiveAction(action);
                QTest::keyClick(menu, Qt::Key_Return);
                return;
            }
        }

        menu->close();
    });
}

[[nodiscard]] auto hasColor(const QImage& image, const QColor& color) -> bool {
    for (i32 row = 0; row < image.height(); row++) {
        for (i32 col = 0; col < image.width(); col++) {
            if (image.pixelColor(col, row) == color) {
                return true;
            }
        }
    }

    return false;
}
}  // namespace

class TestGitFileList final : public QObject {
    Q_OBJECT

   private slots:
    void theModelPassesTheItemModelContract() {
        GitFileModel model;
        const QAbstractItemModelTester tester(&model);

        model.appendItem(itemOf(u"a.txt"_s));
        model.appendItem(itemOf(u"b.txt"_s));
        model.removeItem(0);
        model.setItems({ itemOf(u"c.txt"_s) });
        model.clear();

        QCOMPARE(model.rowCount(), 0);
    }

    void theItemRoleHoldsTheItem() {
        GitFileModel model;
        model.appendItem(itemOf(u"dir/a.txt"_s, GIT_STATUS_INDEX_NEW));

        const auto item = model.data(model.index(0), GitFileModel::GitChangeItemRole).value<GitFileItem>();

        QCOMPARE(item.filePath, u"dir/a.txt"_s);
        QCOMPARE(item.status, GIT_STATUS_INDEX_NEW);
        QCOMPARE(item.fileName(), u"a.txt"_qsv);
        QVERIFY(!model.data(QModelIndex(), GitFileModel::GitChangeItemRole).isValid());
        QVERIFY(!model.data(model.index(0), Qt::DisplayRole).isValid());
    }

    void onlyTheStagedItemsAreSelectedAndRemoved() {
        GitFileModel model;
        model.setItems({ itemOf(u"a"_s), itemOf(u"b"_s), itemOf(u"c"_s) });
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        model.setStaged(0, true);
        model.setStaged(2, true);

        QCOMPARE(changed.size(), qsizetype{ 2 });
        QCOMPARE(changed.at(0).at(2).value<QList<int>>(), QList<int>{ GitFileModel::GitChangeItemRole });

        const auto selected = model.selectedItems();
        QCOMPARE(selected.size(), qsizetype{ 2 });
        QCOMPARE(selected[0]->filePath, u"a"_s);
        QCOMPARE(selected[1]->filePath, u"c"_s);

        model.removeSelectedItems();

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.items().at(0).filePath, u"b"_s);
        QVERIFY(model.selectedItems().isEmpty());
    }

    void removingAnItemKeepsTheOthersInOrder() {
        GitFileModel model;
        model.setItems({ itemOf(u"a"_s), itemOf(u"b"_s), itemOf(u"c"_s) });

        model.removeItem(1);

        QCOMPARE(model.items().size(), qsizetype{ 2 });
        QCOMPARE(model.items().at(0).filePath, u"a"_s);
        QCOMPARE(model.items().at(1).filePath, u"c"_s);
    }

    void theButtonsLieLeftOfTheCheckboxInOrder() {
        // Left to right: nothing, stage, revert, diff, nothing (the checkbox)
        QList<HoverButton> runs;

        for (i32 xPos = ROW_RECT.left(); xPos <= ROW_RECT.right(); xPos++) {
            const auto hit = GitFileDelegate::hitTestButton(ROW_RECT, QPoint(xPos, ROW_RECT.center().y()));

            if (runs.isEmpty() || runs.last() != hit) {
                runs.append(hit);
            }
        }

        const QList<HoverButton> expected = { HoverButton::None,   HoverButton::Stage, HoverButton::None,
                                              HoverButton::Revert, HoverButton::None,  HoverButton::Diff,
                                              HoverButton::None };
        QCOMPARE(runs, expected);

        const ButtonSpots spots = spotsOf(ROW_RECT);
        QCOMPARE(GitFileDelegate::hitTestButton(ROW_RECT, spots.stage), HoverButton::Stage);
        // Above and below the row there is no button
        QCOMPARE(GitFileDelegate::hitTestButton(ROW_RECT, QPoint(spots.stage.x(), ROW_RECT.bottom() + 20)), HoverButton::None);
    }

    void clickingTheCheckboxTogglesTheStateAndReportsIt() {
        GitFileModel model;
        model.setItems({ itemOf(u"a.txt"_s) });
        GitFileDelegate delegate(&model);
        QSignalSpy staged(&delegate, &GitFileDelegate::staged);
        QStyleOptionViewItem option;
        option.rect = ROW_RECT;
        auto click = release(QPoint(ROW_RECT.right() - 12, ROW_RECT.center().y()));

        QVERIFY(delegate.editorEvent(&click, &model, option, model.index(0)));
        QVERIFY(model.items().at(0).staged);
        QCOMPARE(staged.size(), qsizetype{ 1 });
        QCOMPARE(staged.at(0).at(0).toString(), u"a.txt"_s);

        QVERIFY(delegate.editorEvent(&click, &model, option, model.index(0)));
        QVERIFY(!model.items().at(0).staged);
        QCOMPARE(staged.size(), qsizetype{ 2 });
    }

    void clicksThatAreNotACheckboxReleaseChangeNothing() {
        GitFileModel model;
        model.setItems({ itemOf(u"a.txt"_s) });
        GitFileDelegate delegate(&model);
        QSignalSpy staged(&delegate, &GitFileDelegate::staged);
        QStyleOptionViewItem option;
        option.rect = ROW_RECT;
        const QPoint onCheckbox(ROW_RECT.right() - 12, ROW_RECT.center().y());

        auto rightClick = release(onCheckbox, Qt::RightButton);
        auto press = QMouseEvent(QEvent::MouseButtonPress, onCheckbox, onCheckbox, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        auto elsewhere = release(QPoint(2, ROW_RECT.center().y()));

        QVERIFY(!delegate.editorEvent(&rightClick, &model, option, model.index(0)));
        QVERIFY(!delegate.editorEvent(&press, &model, option, model.index(0)));
        QVERIFY(!delegate.editorEvent(&elsewhere, &model, option, model.index(0)));
        QVERIFY(!model.items().at(0).staged);
        QCOMPARE(staged.size(), qsizetype{ 0 });
    }

    void aButtonOfTheHoveredRowRequestsItsAction() {
        GitFileModel model;
        model.setItems({ itemOf(u"a.txt"_s) });
        GitFileDelegate delegate(&model);
        QSignalSpy stageRequested(&delegate, &GitFileDelegate::stageRequested);
        QSignalSpy revertRequested(&delegate, &GitFileDelegate::revertRequested);
        QSignalSpy diffRequested(&delegate, &GitFileDelegate::diffRequested);
        QStyleOptionViewItem option;
        option.rect = ROW_RECT;
        const ButtonSpots spots = spotsOf(ROW_RECT);
        delegate.setHoveredRow(0);

        auto stage = release(spots.stage);
        auto revert = release(spots.revert);
        auto diff = release(spots.diff);

        QVERIFY(delegate.editorEvent(&stage, &model, option, model.index(0)));
        QVERIFY(delegate.editorEvent(&revert, &model, option, model.index(0)));
        QVERIFY(delegate.editorEvent(&diff, &model, option, model.index(0)));

        QCOMPARE(stageRequested.size(), qsizetype{ 1 });
        QCOMPARE(revertRequested.size(), qsizetype{ 1 });
        QCOMPARE(diffRequested.size(), qsizetype{ 1 });
        QCOMPARE(diffRequested.at(0).at(0).toString(), u"a.txt"_s);
        // A button is not the checkbox
        QVERIFY(!model.items().at(0).staged);
    }

    void aButtonOfARowThatIsNotHoveredDoesNothing() {
        GitFileModel model;
        model.setItems({ itemOf(u"a.txt"_s) });
        GitFileDelegate delegate(&model);
        QSignalSpy stageRequested(&delegate, &GitFileDelegate::stageRequested);
        QStyleOptionViewItem option;
        option.rect = ROW_RECT;
        delegate.setHoveredRow(5);
        auto stage = release(spotsOf(ROW_RECT).stage);

        QVERIFY(!delegate.editorEvent(&stage, &model, option, model.index(0)));

        QCOMPARE(stageRequested.size(), qsizetype{ 0 });
    }

    void theBadgeShowsTheStatusInItsColor_data() {
        QTest::addColumn<git_status_t>("status");
        QTest::addColumn<QColor>("color");

        QTest::newRow("conflicted") << GIT_STATUS_CONFLICTED << QColor(0xF1, 0x4C, 0x4C);
        QTest::newRow("ignored") << GIT_STATUS_IGNORED << QColor(0x8C, 0x8C, 0x8C);
        QTest::newRow("deleted") << GIT_STATUS_WT_DELETED << QColor(0xF1, 0x4C, 0x4C);
        QTest::newRow("renamed") << GIT_STATUS_INDEX_RENAMED << QColor(0xE2, 0xC0, 0x8D);
        QTest::newRow("untracked") << GIT_STATUS_WT_NEW << QColor(0x73, 0xC9, 0x91);
        QTest::newRow("added") << GIT_STATUS_INDEX_NEW << QColor(0x73, 0xC9, 0x91);
        QTest::newRow("modified") << GIT_STATUS_WT_MODIFIED << QColor(0xE2, 0xC0, 0x8D);
    }

    void theBadgeShowsTheStatusInItsColor() {
        QFETCH(const git_status_t, status);
        QFETCH(const QColor, color);

        GitFileList list;
        list.appendItem(itemOf(u"a.txt"_s, status));
        list.resize(300, 100);
        list.show();
        QVERIFY(QTest::qWaitForWindowExposed(&list));

        QVERIFY(hasColor(list.viewport()->grab().toImage(), color));
    }

    void anUnknownStatusDrawsNoBadge() {
        // No bit of the status is one the badge knows: not one of the badge colors may appear
        GitFileList list;
        list.appendItem(itemOf(u"a.txt"_s, GIT_STATUS_CURRENT));
        list.resize(300, 100);
        list.show();
        QVERIFY(QTest::qWaitForWindowExposed(&list));

        const QImage image = list.viewport()->grab().toImage();

        QVERIFY(!hasColor(image, QColor(0xE2, 0xC0, 0x8D)));
        QVERIFY(!hasColor(image, QColor(0x73, 0xC9, 0x91)));
        QVERIFY(!hasColor(image, QColor(0xF1, 0x4C, 0x4C)));
    }

    void hoveringARowShowsItsButtonsAndLeavingHidesThem() {
        GitFileList list;
        list.appendItem(itemOf(u"a.txt"_s));
        list.resize(300, 100);
        list.show();
        QVERIFY(QTest::qWaitForWindowExposed(&list));
        const QImage idle = list.viewport()->grab().toImage();

        const QRect rect = list.visualRect(list.model()->index(0));
        hoverAt(list.viewport(), spotsOf(rect).revert);
        QTRY_VERIFY(list.viewport()->grab().toImage() != idle);

        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&list, &leave);
        QCoreApplication::sendEvent(list.viewport(), &leave);
        QTRY_COMPARE(list.viewport()->grab().toImage(), idle);
    }

    void theContextMenuReportsTheSelectedPaths_data() {
        QTest::addColumn<QString>("action");
        QTest::addColumn<bool>("stage");

        QTest::newRow("stage") << u"Stage Selected"_s << true;
        QTest::newRow("revert") << u"Revert Selected"_s << false;
    }

    void theContextMenuReportsTheSelectedPaths() {
        QFETCH(const QString, action);
        QFETCH(const bool, stage);

        GitFileList list;
        list.model()->setItems({ itemOf(u"a"_s), itemOf(u"b"_s), itemOf(u"c"_s) });
        list.resize(300, 100);
        list.show();
        QVERIFY(QTest::qWaitForWindowExposed(&list));
        list.selectionModel()->select(list.model()->index(0), QItemSelectionModel::Select);
        list.selectionModel()->select(list.model()->index(2), QItemSelectionModel::Select);
        QSignalSpy stages(&list, &GitFileList::stageMultipleRequested);
        QSignalSpy reverts(&list, &GitFileList::revertMultipleRequested);

        chooseInMenu(action);
        emit list.customContextMenuRequested(QPoint(10, 10));

        QSignalSpy& chosen = stage ? stages : reverts;
        QSignalSpy& other = stage ? reverts : stages;
        QCOMPARE(chosen.size(), qsizetype{ 1 });
        QCOMPARE(chosen.at(0).at(0).toStringList(), (QStringList{ u"a"_s, u"c"_s }));
        QCOMPARE(other.size(), qsizetype{ 0 });
    }

    void theContextMenuDoesNotOpenWithNothingSelected() {
        GitFileList list;
        list.model()->setItems({ itemOf(u"a"_s) });
        QSignalSpy stages(&list, &GitFileList::stageMultipleRequested);

        // Would block on the menu if it opened
        emit list.customContextMenuRequested(QPoint(10, 10));

        QCOMPARE(stages.size(), qsizetype{ 0 });
        QVERIFY(QApplication::activePopupWidget() == nullptr);
    }
};

QTEST_MAIN(TestGitFileList)
#include "tst_gitfilelist.moc"
#else
#include <QtTest>

class TestGitFileList final : public QObject {
    Q_OBJECT
};

QTEST_MAIN(TestGitFileList)
#include "tst_gitfilelist.moc"
#endif
