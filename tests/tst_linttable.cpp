#include "LintTable.hpp"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QtTest>

namespace {
auto rowOf(const QString& file, const u32 line, const QString& info = u"info"_s) -> LintRow {
    return LintRow{ .filename = file,
                    .sourceText = u"source "_s + file,
                    .translationText = u"translation "_s + file,
                    .sourceHighlights = { Span{ .start = 0, .len = 6 } },
                    .translationHighlights = { Span{ .start = 0, .len = 11 } },
                    .info = info,
                    .lineNumber = line };
}

auto cellText(const LintTableModel& model, const i32 row, const i32 column) -> QVariant {
    return model.data(model.index(row, column), Qt::DisplayRole);
}
}  // namespace

class TestLintTable final : public QObject {
    Q_OBJECT

   private slots:
    void startsEmptyWithFiveColumns() {
        const LintTableModel model;

        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.columnCount(), 5);
        QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
        QCOMPARE(model.flags(QModelIndex()), Qt::NoItemFlags);
    }

    void headersNameTheColumns() {
        const LintTableModel model;

        QCOMPARE(model.headerData(LintTableModel::Filename, Qt::Horizontal, Qt::DisplayRole).toString(), u"File"_s);
        QCOMPARE(model.headerData(LintTableModel::Line, Qt::Horizontal, Qt::DisplayRole).toString(), u"Line"_s);
        QCOMPARE(model.headerData(LintTableModel::SourceMatch, Qt::Horizontal, Qt::DisplayRole).toString(), u"Source"_s);
        QCOMPARE(
            model.headerData(LintTableModel::TranslationMatch, Qt::Horizontal, Qt::DisplayRole).toString(),
            u"Translation"_s
        );
        QCOMPARE(model.headerData(LintTableModel::Info, Qt::Horizontal, Qt::DisplayRole).toString(), u"Info"_s);
        QVERIFY(!model.headerData(7, Qt::Horizontal, Qt::DisplayRole).isValid());
        QVERIFY(!model.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());
    }

    void cellsShowTheFieldsOfTheirRow() {
        LintTableModel model;
        model.appendRow(rowOf(u"actors"_s, 12, u"Tags do not match"_s));

        QCOMPARE(cellText(model, 0, LintTableModel::Filename).toString(), u"actors"_s);
        QCOMPARE(cellText(model, 0, LintTableModel::Line).toUInt(), 12U);
        QCOMPARE(cellText(model, 0, LintTableModel::SourceMatch).toString(), u"source actors"_s);
        QCOMPARE(cellText(model, 0, LintTableModel::TranslationMatch).toString(), u"translation actors"_s);
        QCOMPARE(cellText(model, 0, LintTableModel::Info).toString(), u"Tags do not match"_s);
        QVERIFY(!model.data(model.index(0, 0), Qt::ToolTipRole).isValid());

        // The text starts at the top of a cell, as a row of several lines is taller than the rest
        QCOMPARE(model.data(model.index(0, 0), Qt::TextAlignmentRole).toInt(), static_cast<i32>(Qt::AlignTop | Qt::AlignLeft));

        const Qt::ItemFlags flags = model.flags(model.index(0, 0));
        QVERIFY(flags.testFlag(Qt::ItemIsEnabled));
        QVERIFY(flags.testFlag(Qt::ItemIsSelectable));
        QVERIFY(!flags.testFlag(Qt::ItemIsEditable));
    }

    void appendingRowsOneByOneAndTogether() {
        LintTableModel model;
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);

        model.appendRow(rowOf(u"a"_s, 1));
        model.appendRows({ rowOf(u"b"_s, 2), rowOf(u"c"_s, 3), rowOf(u"d"_s, 4) });

        // One insertion for the batch, so the view lays it out once
        QCOMPARE(inserted.size(), qsizetype{ 2 });
        QCOMPARE(inserted.at(1).at(1).toInt(), 1);
        QCOMPARE(inserted.at(1).at(2).toInt(), 3);
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(model.row(2).filename, u"c"_s);
    }

    void appendingNoRowsChangesNothing() {
        LintTableModel model;
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);

        model.appendRows({});

        QCOMPARE(inserted.size(), qsizetype{ 0 });
        QCOMPARE(model.rowCount(), 0);
    }

    void clearingResetsTheModel() {
        LintTableModel model;
        model.appendRow(rowOf(u"a"_s, 1));
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);

        model.clear();

        QCOMPARE(reset.size(), qsizetype{ 1 });
        QCOMPARE(model.rowCount(), 0);
    }

    void modelObeysTheModelContract() {
        LintTableModel model;
        const QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        model.appendRow(rowOf(u"a"_s, 1));
        model.appendRows({ rowOf(u"b"_s, 2), rowOf(u"c"_s, 3) });
        model.clear();
    }

    void columnsAndRowsFollowTheContentsAfterAppends() {
        LintTable table;
        table.resize(900, 400);
        table.show();
        QVERIFY(QTest::qWaitForWindowExposed(&table));

        const QString longText = u"A fairly long sentence that has to wrap onto several lines of the column "_s.repeated(8);
        table.appendRows({ rowOf(u"short"_s, 1, u"short"_s), rowOf(u"long"_s, 2, longText) });

        // The sizing waits out a burst of appends
        QTRY_VERIFY(table.rowHeight(1) > table.rowHeight(0));

        // A wrapped column stops growing at a width of its own, however long the text is
        QVERIFY(table.columnWidth(LintTableModel::Info) < 420);
        QVERIFY(table.columnWidth(LintTableModel::Info) > 0);
    }

    void textWithLineBreaksIsTallerThanOneLine() {
        LintTable table;
        table.resize(900, 400);
        table.show();
        QVERIFY(QTest::qWaitForWindowExposed(&table));

        LintRow multi = rowOf(u"multi"_s, 1);
        multi.sourceText = u"one\ntwo\nthree"_s;
        table.appendRows({ rowOf(u"single"_s, 2), multi });

        QTRY_VERIFY(table.rowHeight(1) > table.rowHeight(0));
    }

    void paintingHighlightsTheMatchedText() {
        LintRow plainRow = rowOf(u"a"_s, 1);
        plainRow.sourceHighlights.clear();
        plainRow.translationHighlights.clear();
        LintTable marked;
        LintTable plain;

        for (auto* const table : { &marked, &plain }) {
            table->resize(900, 400);
            table->show();
            QVERIFY(QTest::qWaitForWindowExposed(table));
            table->appendRow(table == &marked ? rowOf(u"a"_s, 1) : plainRow);
            QTRY_VERIFY(table->rowHeight(0) > 0);
        }

        QVERIFY(marked.grab().toImage() != plain.grab().toImage());
    }

    void headersAreAskedAgainWhenTheLanguageChanges() {
        LintTable table;
        QSignalSpy changed(table.model(), &QAbstractItemModel::headerDataChanged);

        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(&table, &event);

        QCOMPARE(changed.size(), qsizetype{ 1 });
        QCOMPARE(changed.at(0).at(0).value<Qt::Orientation>(), Qt::Horizontal);
        QCOMPARE(changed.at(0).at(1).toInt(), 0);
        QCOMPARE(changed.at(0).at(2).toInt(), 4);
    }

    void aDraggedColumnKeepsItsWidthWhenRowsAreSizedAgain() {
        LintTable table;
        table.resize(900, 400);
        table.show();
        QVERIFY(QTest::qWaitForWindowExposed(&table));
        table.appendRow(rowOf(u"a"_s, 1));
        // Past the debounce of the sizing the append started
        QTest::qWait(200);

        table.setColumnWidth(LintTableModel::Info, 333);
        QTest::qWait(200);

        QCOMPARE(table.columnWidth(LintTableModel::Info), 333);
    }
};

QTEST_MAIN(TestLintTable)
#include "tst_linttable.moc"
