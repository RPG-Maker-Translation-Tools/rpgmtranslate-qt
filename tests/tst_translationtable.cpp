#include "TranslationTable.hpp"

#include "Constants.hpp"
#include "TranslationTableHeader.hpp"
#include "TranslationTableModel.hpp"

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTimer>
#include <QtTest>

Q_DECLARE_METATYPE(TranslationTable::MultilineAction)

namespace {
void answerBox(const QMessageBox::StandardButton button) {
    QTimer::singleShot(0, [button]() -> void {
        if (auto* const box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            box->button(button)->click();
        }
    });
    QTimer::singleShot(3000, []() -> void { QApplication::closeAllWindows(); });
}

// A table in a window with a status bar, over the rows `lines`, as the main window shows a file
struct Fixture {
    explicit Fixture(const QStringList& lines) {
        window.setCentralWidget(&table);
        window.statusBar();
        table.init(&settings, &projectSettings);

        vector<QStringView> views;
        views.reserve(static_cast<usize>(lines.size()));
        for (const QString& line : lines) {
            views.emplace_back(line);
        }

        table.fill(
            std::span<QStringView>(views.data(), views.size()),
            { ColumnInfo{ u"Source"_s, 140 }, ColumnInfo{ u"English"_s, 220 } },
            u"test.txt"_s
        );

        window.resize(700, 400);
        window.show();
    }

    [[nodiscard]] auto cell(const i32 row, const i32 column) const -> QString {
        return table.model()->data(table.model()->index(row, column), Qt::EditRole).toString();
    }

    void select(const i32 row, const i32 column) { table.setCurrentIndex(table.model()->index(row, column)); }

    void selectRows(const i32 first, const i32 last, const i32 column) {
        table.selectionModel()->clearSelection();
        table.selectionModel()->select(
            QItemSelection(table.model()->index(first, column), table.model()->index(last, column)),
            QItemSelectionModel::Select
        );
    }

    QMainWindow window;
    Settings settings;
    ProjectSettings projectSettings;
    TranslationTable table;
};

const QStringList THREE_ROWS = { u"a<#>one"_s, u"b<#>"_s, u"c<#>three"_s };
}  // namespace

class TestTranslationTable final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() { qRegisterMetaType<TranslationTable::MultilineAction>(); }

    // Filling
    void fillingNamesTheColumnsAndSizesThem() {
        Fixture fixture({ u"a<#>one<#>two<#>three"_s });

        const TranslationTableModel* const model = fixture.table.model();
        QCOMPARE(model->columnCount(), 4);
        QCOMPARE(model->headerData(0, Qt::Horizontal).toString(), u"Source"_s);
        QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), u"English"_s);
        QCOMPARE(fixture.table.columnWidth(0), 140);
        QCOMPARE(fixture.table.columnWidth(1), 220);

        // A file edited outside can have more columns than the project knows
        QCOMPARE(model->headerData(2, Qt::Horizontal).toString(), u"Translation"_s);
        QCOMPARE(model->headerData(3, Qt::Horizontal).toString(), u"Translation"_s);
        QCOMPARE(fixture.table.columnWidth(3), DEFAULT_COLUMN_WIDTH);
    }

    // Columns
    void theAddButtonAddsAColumn() {
        Fixture fixture(THREE_ROWS);
        QSignalSpy added(&fixture.table, &TranslationTable::columnAdded);

        emit fixture.table.header()->addButtonClicked();

        QCOMPARE(added.size(), qsizetype{ 1 });
        QCOMPARE(fixture.table.model()->columnCount(), 3);
        QCOMPARE(fixture.table.model()->headerData(2, Qt::Horizontal).toString(), u"Translation"_s);
        QCOMPARE(fixture.table.columnWidth(2), DEFAULT_COLUMN_WIDTH);
    }

    void thereIsALimitToTheColumns() {
        Fixture fixture(THREE_ROWS);

        while (fixture.table.model()->columnCount() < TranslationTableModel::MAX_COLUMNS) {
            emit fixture.table.header()->addButtonClicked();
        }

        QSignalSpy added(&fixture.table, &TranslationTable::columnAdded);
        emit fixture.table.header()->addButtonClicked();

        QCOMPARE(added.size(), qsizetype{ 0 });
        QCOMPARE(fixture.table.model()->columnCount(), TranslationTableModel::MAX_COLUMNS);
        QVERIFY(fixture.window.statusBar()->currentMessage().contains(u"more than %1 columns"_s.arg(TranslationTableModel::MAX_COLUMNS)));
    }

    void renamingAndResizingColumnsIsReported() {
        Fixture fixture(THREE_ROWS);
        QSignalSpy renamed(&fixture.table, &TranslationTable::columnRenamed);
        QSignalSpy resized(&fixture.table, &TranslationTable::columnResized);

        fixture.table.model()->setHeaderData(1, Qt::Horizontal, u"French"_s);
        QCOMPARE(renamed.size(), qsizetype{ 1 });
        QCOMPARE(renamed.at(0).at(0).toInt(), 1);
        QCOMPARE(renamed.at(0).at(1).toString(), u"French"_s);

        // The source column is not a language
        fixture.table.model()->setHeaderData(0, Qt::Horizontal, u"Origin"_s);
        QCOMPARE(renamed.size(), qsizetype{ 1 });

        fixture.table.header()->resizeSection(1, 333);
        QVERIFY(!resized.isEmpty());
        QCOMPARE(resized.last().at(0).toInt(), 1);
        QCOMPARE(resized.last().at(1).toInt(), 333);
    }

    // Rows
    void aRemovedRowReportsWhatItWas_data() {
        QTest::addColumn<QStringList>("lines");
        QTest::addColumn<i32>("row");
        QTest::addColumn<u8>("flags");

        QTest::newRow("translated") << QStringList{ u"a<#>one"_s } << 0 << static_cast<u8>(RowFlags::TranslatedFlag);
        QTest::newRow("untranslated") << QStringList{ u"a<#>"_s } << 0 << u8{ 0 };
        QTest::newRow("whitespace only") << QStringList{ u"a<#>  "_s } << 0 << u8{ 0 };
        QTest::newRow("comment") << QStringList{ u"<!>FILE: x<#>"_s, u"a<#>one"_s } << 0
                                 << static_cast<u8>(RowFlags::CommentFlag);
        QTest::newRow("bookmark") << QStringList{ u"<!>BOOKMARK<#>note"_s, u"a<#>one"_s } << 0
                                  << static_cast<u8>(RowFlags::CommentFlag | RowFlags::BookmarkFlag);
    }

    void aRemovedRowReportsWhatItWas() {
        QFETCH(const QStringList, lines);
        QFETCH(const i32, row);
        QFETCH(const u8, flags);

        Fixture fixture(lines);
        optional<RemovedRowInfo> info;
        connect(&fixture.table, &TranslationTable::rowRemoved, &fixture.table, [&info](const RemovedRowInfo removed) -> void {
            info = removed;
        });

        QVERIFY(fixture.table.model()->removeRows(row, 1));

        QVERIFY(info.has_value());
        QCOMPARE(info->get<"row">(), static_cast<u32>(row));
        QCOMPARE(static_cast<u8>(info->get<"flags">()), flags);
    }

    void clickingASourceCopiesItToTheClipboard() {
        Fixture fixture({ u"the source text<#>one"_s });

        emit fixture.table.pressed(fixture.table.model()->index(0, 0));

        QCOMPARE(QApplication::clipboard()->text(), u"the source text"_s);
        QCOMPARE(fixture.window.statusBar()->currentMessage(), u"Source text from row 1 is copied to clipboard."_s);
    }

    void clickingASourceOfACommentCopiesNothing() {
        Fixture fixture({ u"<!>FILE: x<#>"_s });
        QApplication::clipboard()->setText(u"untouched"_s);

        emit fixture.table.pressed(fixture.table.model()->index(0, 0));

        QCOMPARE(QApplication::clipboard()->text(), u"untouched"_s);
    }

    // Copy, cut and paste
    void copyingTakesTheEditableCellsOfTheSelection() {
        Fixture fixture({ u"a<#>one"_s, u"<!>FILE: x<#>"_s, u"c<#>three"_s });
        fixture.selectRows(0, 2, 1);

        QCOMPARE(fixture.table.copy(), u32{ 2 });
        QCOMPARE(QApplication::clipboard()->text(), u"one\nthree"_s);
    }

    void copyingNothingCopiesNothing() {
        Fixture fixture(THREE_ROWS);
        fixture.table.selectionModel()->clearSelection();

        QCOMPARE(fixture.table.copy(), u32{ 0 });
        QCOMPARE(fixture.table.cut(), u32{ 0 });
        QCOMPARE(fixture.table.paste(), u32{ 0 });
    }

    void cuttingEmptiesTheCellsAndKeepsTheText() {
        Fixture fixture(THREE_ROWS);
        fixture.selectRows(0, 2, 1);

        QCOMPARE(fixture.table.cut(), u32{ 3 });

        QCOMPARE(QApplication::clipboard()->text(), u"one\n\nthree"_s);
        QCOMPARE(fixture.cell(0, 1), QString());
        QCOMPARE(fixture.cell(2, 1), QString());
        // The sources are not touched
        QCOMPARE(fixture.cell(0, 0), u"a"_s);
    }

    void pastingFromOneCellFillsTheRowsBelow() {
        Fixture fixture({ u"a<#>"_s, u"<!>FILE: x<#>"_s, u"b<#>"_s, u"c<#>"_s });
        QApplication::clipboard()->setText(u"one\ntwo\nthree\nfour"_s);
        fixture.selectRows(0, 0, 1);

        // The comment is skipped, and the text that does not fit is dropped
        QCOMPARE(fixture.table.paste(), u32{ 3 });

        QCOMPARE(fixture.cell(0, 1), u"one"_s);
        QCOMPARE(fixture.cell(2, 1), u"two"_s);
        QCOMPARE(fixture.cell(3, 1), u"three"_s);
    }

    void pastingIntoSeveralRowsGivesEachItsLine() {
        Fixture fixture({ u"a<#>"_s, u"b<#>"_s, u"c<#>"_s });
        QApplication::clipboard()->setText(u"one\ntwo\nthree"_s);
        fixture.table.selectionModel()->clearSelection();
        fixture.table.selectionModel()->select(fixture.table.model()->index(0, 1), QItemSelectionModel::Select);
        fixture.table.selectionModel()->select(fixture.table.model()->index(2, 1), QItemSelectionModel::Select);

        QCOMPARE(fixture.table.paste(), u32{ 2 });

        QCOMPARE(fixture.cell(0, 1), u"one"_s);
        QCOMPARE(fixture.cell(1, 1), QString());
        QCOMPARE(fixture.cell(2, 1), u"two"_s);
    }

    void pastingTakesWindowsLineEndingsAndATrailingOne() {
        Fixture fixture({ u"a<#>"_s, u"b<#>"_s });
        QApplication::clipboard()->setText(u"one\r\ntwo\r\n"_s);
        fixture.selectRows(0, 0, 1);

        QCOMPARE(fixture.table.paste(), u32{ 2 });

        QCOMPARE(fixture.cell(0, 1), u"one"_s);
        QCOMPARE(fixture.cell(1, 1), u"two"_s);
    }

    void pastingBringsBackTheLineBreaksOfCopiedCells() {
        Fixture fixture({ u"a<#>x"_s, u"b<#>"_s });
        fixture.table.model()->setData(fixture.table.model()->index(0, 1), u"first\nsecond"_s, Qt::EditRole);
        fixture.selectRows(0, 0, 1);
        fixture.table.copy();

        fixture.selectRows(1, 1, 1);
        fixture.table.paste();

        QCOMPARE(fixture.cell(1, 1), u"first\nsecond"_s);
    }

    void pastingAnEmptyClipboardDoesNothing() {
        Fixture fixture(THREE_ROWS);
        QApplication::clipboard()->clear();
        fixture.selectRows(0, 0, 1);

        QCOMPARE(fixture.table.paste(), u32{ 0 });
        QCOMPARE(fixture.cell(0, 1), u"one"_s);
    }

    void theKeyboardShortcutsReportTheirCounts() {
        Fixture fixture(THREE_ROWS);
        fixture.table.setFocus();
        fixture.selectRows(0, 1, 1);
        QSignalSpy actions(&fixture.table, &TranslationTable::multilineAction);

        QTest::keyClick(&fixture.table, Qt::Key_C, Qt::ControlModifier);
        QTest::keyClick(&fixture.table, Qt::Key_X, Qt::ControlModifier);
        QTest::keyClick(&fixture.table, Qt::Key_V, Qt::ControlModifier);

        QCOMPARE(actions.size(), qsizetype{ 3 });
        QCOMPARE(actions.at(0).at(0).value<TranslationTable::MultilineAction>(), TranslationTable::MultilineAction::Copy);
        QCOMPARE(actions.at(0).at(1).toUInt(), 2U);
        QCOMPARE(actions.at(1).at(0).value<TranslationTable::MultilineAction>(), TranslationTable::MultilineAction::Cut);
        QCOMPARE(actions.at(1).at(1).toUInt(), 2U);
        QCOMPARE(actions.at(2).at(0).value<TranslationTable::MultilineAction>(), TranslationTable::MultilineAction::Paste);
        // What was cut is two lines, the last of them empty, and the trailing empty line is not pasted
        QCOMPARE(actions.at(2).at(1).toUInt(), 1U);
    }

    // Inserting a translation
    void aTranslationGoesIntoTheEmptyCurrentCell() {
        Fixture fixture(THREE_ROWS);
        fixture.select(1, 1);

        fixture.table.insertTranslation(u"translated"_s);

        QCOMPARE(fixture.cell(1, 1), u"translated"_s);
    }

    void aFilledCellIsOverwrittenOnlyWhenTheUserAgrees() {
        Fixture fixture(THREE_ROWS);
        fixture.select(0, 1);

        QTest::ignoreMessage(QtInfoMsg, "Overwriting a non-empty cell was rejected by user.");
        answerBox(QMessageBox::No);
        fixture.table.insertTranslation(u"new"_s);
        QCOMPARE(fixture.cell(0, 1), u"one"_s);

        answerBox(QMessageBox::Yes);
        fixture.table.insertTranslation(u"new"_s);
        QCOMPARE(fixture.cell(0, 1), u"new"_s);
    }

    void noTranslationGoesIntoASourceOrWithoutACell() {
        Fixture fixture(THREE_ROWS);

        fixture.table.setCurrentIndex(QModelIndex());
        fixture.table.insertTranslation(u"x"_s);
        fixture.select(1, 0);
        fixture.table.insertTranslation(u"x"_s);

        QCOMPARE(fixture.cell(1, 0), u"b"_s);
        QCOMPARE(fixture.cell(1, 1), QString());
    }

    void aTranslationGoesIntoTheOpenEditor() {
        Fixture fixture(THREE_ROWS);
        fixture.select(1, 1);
        fixture.table.edit(fixture.table.currentIndex());
        QVERIFY(fixture.table.activeInput() != nullptr);

        fixture.table.insertTranslation(u"typed in"_s);

        QCOMPARE(fixture.table.activeInput()->toPlainText(), u"typed in"_s);
    }

    // The cells of a row
    void onlyOneCellOfARowStaysSelected() {
        Fixture fixture({ u"a<#>one<#>two<#>three"_s });
        fixture.select(0, 1);

        fixture.table.selectionModel()->select(fixture.table.model()->index(0, 2), QItemSelectionModel::Select);

        QCOMPARE(fixture.table.selectionModel()->selectedIndexes().size(), qsizetype{ 1 });
    }

    // Editing
    void editingShowsTheCellAndSavingWritesItBack() {
        Fixture fixture(THREE_ROWS);
        QSignalSpy focused(&fixture.table, &TranslationTable::inputFocused);
        QSignalSpy changed(&fixture.table, &TranslationTable::textChanged);
        fixture.select(0, 1);

        fixture.table.edit(fixture.table.currentIndex());

        TranslationInput* const input = fixture.table.activeInput();
        QVERIFY(input != nullptr);
        QCOMPARE(input->toPlainText(), u"one"_s);
        QCOMPARE(focused.size(), qsizetype{ 1 });

        // Filling the editor with the cell is a change of text too
        changed.clear();
        QTest::keyClicks(input, u" more"_s);
        QCOMPARE(changed.size(), qsizetype{ 5 });

        // The editor is committed by Escape, as it is by leaving it
        QTest::keyClick(input, Qt::Key_Escape);
        QCOMPARE(fixture.cell(0, 1), u"one more"_s);
    }

    void shiftArrowsMoveTheEditorToTheNextEditableCell() {
        Fixture fixture({ u"a<#>one"_s, u"<!>FILE: x<#>"_s, u"c<#>three"_s });
        fixture.select(0, 1);
        fixture.table.edit(fixture.table.currentIndex());
        QVERIFY(fixture.table.activeInput() != nullptr);

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Down, Qt::ShiftModifier);

        // The comment is skipped
        QTRY_COMPARE(fixture.table.currentIndex().row(), 2);
        QTRY_VERIFY(fixture.table.activeInput() != nullptr);
        QCOMPARE(fixture.table.activeInput()->toPlainText(), u"three"_s);

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Up, Qt::ShiftModifier);
        QTRY_COMPARE(fixture.table.currentIndex().row(), 0);
    }

    void shiftLeftAndRightMoveTheEditorToTheNeighbouringColumn() {
        Fixture fixture({ u"a<#>one<#>two"_s });
        fixture.select(0, 1);
        fixture.table.edit(fixture.table.currentIndex());
        QVERIFY(fixture.table.activeInput() != nullptr);

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Right, Qt::ShiftModifier);
        QTRY_COMPARE(fixture.table.currentIndex().column(), 2);
        QTRY_VERIFY(fixture.table.activeInput() != nullptr);
        QCOMPARE(fixture.table.activeInput()->toPlainText(), u"two"_s);

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Left, Qt::ShiftModifier);
        QTRY_COMPARE(fixture.table.currentIndex().column(), 1);
    }

    // A right click opens a menu that blocks until it is answered, so the keys go to it from a timer
    void rightClickingARowOffersToRemoveOrBookmarkIt_data() {
        QTest::addColumn<i32>("downs");
        QTest::addColumn<i32>("rows");
        QTest::addColumn<i32>("bookmarks");

        QTest::newRow("escape") << 0 << 3 << 0;
        QTest::newRow("remove row") << 1 << 2 << 0;
        QTest::newRow("bookmark row") << 2 << 4 << 1;
    }

    void rightClickingARowOffersToRemoveOrBookmarkIt() {
        QFETCH(const i32, downs);
        QFETCH(const i32, rows);
        QFETCH(const i32, bookmarks);

        Fixture fixture(THREE_ROWS);
        QSignalSpy bookmarked(&fixture.table, &TranslationTable::bookmarked);

        QTimer::singleShot(300, [downs]() -> void {
            auto* const menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());

            if (menu == nullptr) {
                return;
            }

            for (i32 press = 0; press < downs; press++) {
                QTest::keyClick(menu, Qt::Key_Down);
            }

            QTest::keyClick(menu, downs == 0 ? Qt::Key_Escape : Qt::Key_Return);
        });

        const QPoint inCell = fixture.table.visualRect(fixture.table.model()->index(1, 1)).center();
        QTest::mouseClick(
            fixture.window.windowHandle(),
            Qt::RightButton,
            Qt::NoModifier,
            fixture.table.viewport()->mapTo(&fixture.window, inCell)
        );

        QCOMPARE(fixture.table.model()->rowCount(), rows);
        QCOMPARE(bookmarked.size(), static_cast<qsizetype>(bookmarks));

        if (bookmarks != 0) {
            QCOMPARE(fixture.cell(1, 0), BOOKMARK_COMMENT.toString());
        }
    }

    void rightClickingACommentRowOpensNoMenu() {
        Fixture fixture({ u"<!>FILE: x<#>"_s, u"a<#>one"_s });

        bool menuOpened = false;
        QTimer::singleShot(300, [&menuOpened]() -> void {
            if (auto* const menu = QApplication::activePopupWidget()) {
                menuOpened = true;
                menu->close();
            }
        });

        const QPoint inCell = fixture.table.visualRect(fixture.table.model()->index(0, 1)).center();
        QTest::mouseClick(
            fixture.window.windowHandle(),
            Qt::RightButton,
            Qt::NoModifier,
            fixture.table.viewport()->mapTo(&fixture.window, inCell)
        );
        // Past the moment the timer looks for a menu
        QTest::qWait(500);

        QVERIFY(!menuOpened);
        QCOMPARE(fixture.table.model()->rowCount(), 2);
    }

    void clearingTheCurrentCellForgetsNothingAndChangesNothing() {
        Fixture fixture(THREE_ROWS);
        fixture.select(1, 1);
        const QString before = fixture.cell(1, 1);

        fixture.table.setCurrentIndex(QModelIndex());

        QVERIFY(!fixture.table.currentIndex().isValid());
        QVERIFY(!fixture.table.selectionModel()->hasSelection());
        QVERIFY(fixture.table.activeInput() == nullptr);
        QCOMPARE(fixture.table.model()->rowCount(), 3);
        QCOMPARE(fixture.cell(1, 1), before);

        // The table is still usable: the same cell can be made current again
        fixture.select(1, 1);
        QCOMPARE(fixture.table.currentIndex().row(), 1);
        QVERIFY(fixture.table.selectionModel()->isSelected(fixture.table.model()->index(1, 1)));
    }

    void shiftArrowsStopAtTheEdgeOfTheTable() {
        Fixture fixture(THREE_ROWS);
        fixture.select(2, 1);
        fixture.table.edit(fixture.table.currentIndex());
        QVERIFY(fixture.table.activeInput() != nullptr);

        // Nothing below: the editor closes and no other opens
        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Down, Qt::ShiftModifier);
        QTRY_VERIFY(fixture.table.activeInput() == nullptr);

        QCOMPARE(fixture.table.currentIndex().row(), 2);
        QCOMPARE(fixture.cell(2, 1), u"three"_s);
    }

    void controlArrowsJumpToTheEndsOfTheColumn() {
        Fixture fixture({ u"<!>FILE: x<#>"_s, u"a<#>one"_s, u"b<#>two"_s, u"c<#>three"_s, u"<!>END<#>"_s });
        fixture.select(2, 1);
        fixture.table.edit(fixture.table.currentIndex());

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Down, Qt::ControlModifier);
        QTRY_COMPARE(fixture.table.currentIndex().row(), 3);

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Up, Qt::ControlModifier);
        QTRY_COMPARE(fixture.table.currentIndex().row(), 1);
    }

    void controlReturnMovesOnToTheNextInput() {
        Fixture fixture(THREE_ROWS);
        fixture.select(0, 1);
        fixture.table.edit(fixture.table.currentIndex());

        QTest::keyClick(fixture.table.activeInput(), Qt::Key_Return, Qt::ControlModifier);

        QTRY_COMPARE(fixture.table.currentIndex().row(), 1);
    }

    void aCellIsTallAsItsLinesAndTheColumnsKeepTheirWidth() {
        Fixture fixture({ u"a<#>one"_s, u"b<#>x\\ny"_s });
        fixture.table.model()->setData(fixture.table.model()->index(1, 1), u"x\ny\nz"_s, Qt::EditRole);
        fixture.table.resizeRowsToContents();

        QVERIFY(fixture.table.rowHeight(1) > fixture.table.rowHeight(0));
        // The widths are the ones the file's columns ask for, whatever the text
        QCOMPARE(fixture.table.columnWidth(0), 140);
        QCOMPARE(fixture.table.columnWidth(1), 220);
    }
};

QTEST_MAIN(TestTranslationTable)
#include "tst_translationtable.moc"
