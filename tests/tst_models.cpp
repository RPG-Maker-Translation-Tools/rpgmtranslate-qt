#include "TabListModel.hpp"
#include "TranslationTableModel.hpp"

#include "Constants.hpp"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QtTest>

namespace {
// The model keeps views into nothing: `fill` copies every cell, so the lines only have to outlive the call
void fillModel(TranslationTableModel& model, const QStringList& lines) {
    QList<QStringView> views;
    views.reserve(lines.size());

    for (const QString& line : lines) {
        views.append(line);
    }

    model.fill(std::span<QStringView>(views.data(), static_cast<usize>(views.size())), u"test.txt"_s);
}

auto textAt(const TranslationTableModel& model, const i32 row, const i32 column) -> QString {
    return model.data(model.index(row, column)).toString();
}

auto editableAt(const TranslationTableModel& model, const i32 row, const i32 column) -> bool {
    return model.flags(model.index(row, column)).testFlag(Qt::ItemIsEditable);
}

auto ignoreSplitWarning() -> void {
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Couldn't split text"_s));
}
}  // namespace

class TestModels final : public QObject {
    Q_OBJECT

   private slots:
    // TranslationTableModel
    void startsEmpty() {
        TranslationTableModel model;

        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.columnCount(), 0);
        QVERIFY(!model.data(QModelIndex()).isValid());
        QCOMPARE(model.flags(QModelIndex()), Qt::NoItemFlags);
    }

    void fillSplitsLinesIntoColumns() {
        TranslationTableModel model;
        fillModel(model, { u"hello<#>bonjour"_s, u"bye<#>au revoir"_s });

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.columnCount(), 2);
        QCOMPARE(textAt(model, 0, 0), u"hello"_s);
        QCOMPARE(textAt(model, 0, 1), u"bonjour"_s);
        QCOMPARE(textAt(model, 1, 0), u"bye"_s);
        QCOMPARE(textAt(model, 1, 1), u"au revoir"_s);
    }

    void fillTurnsTheLineBreakMarkerIntoALineFeed() {
        TranslationTableModel model;
        fillModel(model, { u"one\\#two<#>un\\#deux"_s });

        QCOMPARE(textAt(model, 0, 0), u"one\ntwo"_s);
        QCOMPARE(textAt(model, 0, 1), u"un\ndeux"_s);
    }

    void fillSkipsBlankLines() {
        TranslationTableModel model;
        fillModel(model, { QString(), u"a<#>b"_s, u"   "_s, u"\t"_s, u"c<#>d"_s });

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(textAt(model, 1, 0), u"c"_s);
    }

    void fillReplacesWhatWasThere() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b<#>c"_s, u"d<#>e<#>f"_s });
        QCOMPARE(model.columnCount(), 3);

        fillModel(model, { u"x<#>y"_s });

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.columnCount(), 2);
        QCOMPARE(textAt(model, 0, 0), u"x"_s);
    }

    void onlyTranslationCellsAreEditable() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b<#>c"_s });

        QVERIFY(!editableAt(model, 0, 0));
        QVERIFY(editableAt(model, 0, 1));
        QVERIFY(editableAt(model, 0, 2));
        QVERIFY(model.flags(model.index(0, 0)).testFlag(Qt::ItemIsEnabled));
        QVERIFY(!model.flags(model.index(0, 0)).testFlag(Qt::ItemIsSelectable));
        QVERIFY(model.flags(model.index(0, 1)).testFlag(Qt::ItemIsSelectable));
    }

    void tableIsAsWideAsItsWidestRow() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s, u"c<#>d<#>e<#>f"_s, u"g<#>h"_s });

        QCOMPARE(model.columnCount(), 4);
        // The earlier rows keep their cells when the table is widened
        QCOMPARE(textAt(model, 0, 0), u"a"_s);
        QCOMPARE(textAt(model, 0, 1), u"b"_s);
        QCOMPARE(textAt(model, 0, 2), QString());
        QCOMPARE(textAt(model, 1, 3), u"f"_s);
        QCOMPARE(textAt(model, 2, 1), u"h"_s);
        // A narrower row can still be translated in the other columns
        QVERIFY(editableAt(model, 0, 3));
        QVERIFY(editableAt(model, 2, 2));
        QVERIFY(!editableAt(model, 2, 0));
    }

    void ordinaryCommentsAreKeptWhole() {
        TranslationTableModel model;
        fillModel(model, { u"<!>FILE: Map001<#>"_s, u"a<#>b"_s });

        QCOMPARE(textAt(model, 0, 0), u"<!>FILE: Map001<#>"_s);
        QVERIFY(!editableAt(model, 0, 1));
        QVERIFY(!editableAt(model, 0, 0));
    }

    void displayNameAndBookmarksAreEditable() {
        TranslationTableModel model;
        fillModel(model, { u"<!>IN-GAME DISPLAYED NAME: Cave<#>Grotte"_s, u"<!>BOOKMARK<#>remember"_s, u"a<#>b"_s });

        QCOMPARE(textAt(model, 0, 0), u"<!>IN-GAME DISPLAYED NAME: Cave"_s);
        QCOMPARE(textAt(model, 0, 1), u"Grotte"_s);
        QVERIFY(editableAt(model, 0, 1));

        QCOMPARE(textAt(model, 1, 0), u"<!>BOOKMARK"_s);
        QCOMPARE(textAt(model, 1, 1), u"remember"_s);
        QVERIFY(editableAt(model, 1, 1));
    }

    void linesWithoutASeparatorAreKeptWhole() {
        TranslationTableModel model;

        ignoreSplitWarning();
        fillModel(model, { u"no separator here"_s, u"a<#>b"_s });

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(textAt(model, 0, 0), u"no separator here"_s);
        QVERIFY(!editableAt(model, 0, 1));
        QVERIFY(editableAt(model, 1, 1));
    }

    void linesWithTooManyColumnsAreKeptWhole() {
        TranslationTableModel model;

        QStringList parts;
        for (i32 idx = 0; idx <= TranslationTableModel::MAX_COLUMNS; idx++) {
            parts.append(u"c%1"_s.arg(idx));
        }
        const QString tooWide = parts.join(u"<#>"_s);

        fillModel(model, { tooWide });

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.columnCount(), 2);
        QCOMPARE(textAt(model, 0, 0), tooWide);
        QVERIFY(!editableAt(model, 0, 1));

        // Exactly the most columns is still a row
        parts.removeLast();
        fillModel(model, { parts.join(u"<#>"_s) });
        QCOMPARE(model.columnCount(), TranslationTableModel::MAX_COLUMNS);
        QVERIFY(editableAt(model, 0, TranslationTableModel::MAX_COLUMNS - 1));
    }

    void dataRoles() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s });

        const QModelIndex index = model.index(0, 1);

        QCOMPARE(model.data(index, Qt::DisplayRole).toString(), u"b"_s);
        QCOMPARE(model.data(index, Qt::EditRole).toString(), u"b"_s);
        QCOMPARE(model.data(index, Qt::TextAlignmentRole).toInt(), static_cast<i32>(Qt::AlignLeft | Qt::AlignTop));
        QVERIFY(!model.data(index, Qt::ToolTipRole).isValid());
    }

    void setDataCountsTranslatedCells() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>"_s, u"b<#>old"_s });

        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        QSignalSpy translated(&model, &TranslationTableModel::translatedChanged);

        // Translating an empty cell
        QVERIFY(model.setData(model.index(0, 1), u"new"_s));
        QCOMPARE(textAt(model, 0, 1), u"new"_s);
        QCOMPARE(translated.size(), qsizetype{ 1 });
        QCOMPARE(translated.last().at(0).value<i8>(), i8{ 1 });

        // Editing a translated one changes nothing about the count
        QVERIFY(model.setData(model.index(0, 1), u"newer"_s));
        QCOMPARE(translated.size(), qsizetype{ 1 });

        // Clearing it, even down to whitespace
        QVERIFY(model.setData(model.index(1, 1), u"   "_s));
        QCOMPARE(translated.size(), qsizetype{ 2 });
        QCOMPARE(translated.last().at(0).value<i8>(), i8{ -1 });

        // Whitespace to nothing is not a change
        QVERIFY(model.setData(model.index(1, 1), QString()));
        QCOMPARE(translated.size(), qsizetype{ 2 });

        QCOMPARE(changed.size(), 4);
    }

    void setDataRefusesWhatItCannotStore() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s });

        QVERIFY(!model.setData(QModelIndex(), u"x"_s));
        QVERIFY(!model.setData(model.index(0, 1), u"x"_s, Qt::ToolTipRole));
        QCOMPARE(textAt(model, 0, 1), u"b"_s);
    }

    void bookmarksReportTheirRowNotTheCount() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s, u"<!>BOOKMARK<#>"_s });

        QSignalSpy bookmark(&model, &TranslationTableModel::bookmarkChanged);
        QSignalSpy translated(&model, &TranslationTableModel::translatedChanged);

        QVERIFY(model.setData(model.index(1, 1), u"note"_s));

        QCOMPARE(textAt(model, 1, 1), u"note"_s);
        QCOMPARE(bookmark.size(), qsizetype{ 1 });
        QCOMPARE(bookmark.last().at(0).value<u32>(), u32{ 1 });
        QVERIFY(translated.isEmpty());
    }

    void headers() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b<#>c"_s });

        QSignalSpy changed(&model, &QAbstractItemModel::headerDataChanged);

        model.setHeaderLabels({ u"Source"_s, u"English"_s });

        QCOMPARE(changed.size(), 2);
        QCOMPARE(model.headerData(0, Qt::Horizontal).toString(), u"Source"_s);
        QCOMPARE(model.headerData(1, Qt::Horizontal).toString(), u"English"_s);
        // A column without a label falls back to the default
        QCOMPARE(model.headerData(2, Qt::Horizontal).toString(), u"3"_s);
        QCOMPARE(model.headerData(0, Qt::Vertical).toString(), u"1"_s);
    }

    void appendColumnAddsAnEditableColumn() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s, u"<!>FILE: x<#>"_s, u"c<#>d"_s });

        QSignalSpy inserted(&model, &QAbstractItemModel::columnsInserted);

        model.appendColumn({ u"A2"_s, u"ignored"_s, u"C2"_s });

        QCOMPARE(inserted.size(), 1);
        QCOMPARE(model.columnCount(), 3);
        QCOMPARE(textAt(model, 0, 2), u"A2"_s);
        QCOMPARE(textAt(model, 2, 2), u"C2"_s);
        QCOMPARE(textAt(model, 0, 1), u"b"_s);
        QVERIFY(editableAt(model, 0, 2));
        QVERIFY(editableAt(model, 2, 2));
        // Comments get no editable cell
        QVERIFY(!editableAt(model, 1, 2));
    }

    void appendColumnWithFewerCellsLeavesTheRestEmpty() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s, u"c<#>d"_s });

        model.appendColumn({ u"only"_s });

        QCOMPARE(textAt(model, 0, 2), u"only"_s);
        QCOMPARE(textAt(model, 1, 2), QString());
    }

    void appendColumnStopsAtTheLimit() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s });

        for (i32 idx = model.columnCount(); idx < TranslationTableModel::MAX_COLUMNS; idx++) {
            model.appendColumn({ u"x"_s });
        }
        QCOMPARE(model.columnCount(), TranslationTableModel::MAX_COLUMNS);

        QSignalSpy inserted(&model, &QAbstractItemModel::columnsInserted);
        model.appendColumn({ u"over"_s });

        QCOMPARE(model.columnCount(), TranslationTableModel::MAX_COLUMNS);
        QVERIFY(inserted.isEmpty());
        // Every column up to the last one stays editable: the mask holds a bit per column
        QVERIFY(editableAt(model, 0, TranslationTableModel::MAX_COLUMNS - 1));
    }

    void appendColumnToAnEmptyTable() {
        TranslationTableModel model;

        model.appendColumn({});

        QCOMPARE(model.columnCount(), 1);
        QCOMPARE(model.rowCount(), 0);
    }

    void insertRowPutsABookmarkInPlace() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s, u"c<#>d"_s });

        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);

        model.insertRow(1, { u"<!>BOOKMARK"_s, u"note"_s });

        QCOMPARE(inserted.size(), 1);
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(textAt(model, 0, 0), u"a"_s);
        QCOMPARE(textAt(model, 1, 0), u"<!>BOOKMARK"_s);
        QCOMPARE(textAt(model, 1, 1), u"note"_s);
        QCOMPARE(textAt(model, 2, 0), u"c"_s);
        QCOMPARE(textAt(model, 2, 1), u"d"_s);
        QVERIFY(editableAt(model, 1, 1));
        QVERIFY(!editableAt(model, 1, 0));

        // At either end
        model.insertRow(0, { u"<!>BOOKMARK"_s, u"first"_s });
        model.insertRow(4, { u"<!>BOOKMARK"_s, u"last"_s });
        QCOMPARE(textAt(model, 0, 1), u"first"_s);
        QCOMPARE(textAt(model, 4, 1), u"last"_s);
        QCOMPARE(textAt(model, 3, 0), u"c"_s);
    }

    void insertedBookmarksReportChanges() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s });
        model.insertRow(1, { u"<!>BOOKMARK"_s, QString() });

        QSignalSpy bookmark(&model, &TranslationTableModel::bookmarkChanged);
        QVERIFY(model.setData(model.index(1, 1), u"x"_s));

        QCOMPARE(bookmark.size(), qsizetype{ 1 });
    }

    void removeRows() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>1"_s, u"b<#>2"_s, u"c<#>3"_s, u"d<#>4"_s });

        QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);

        QVERIFY(model.removeRows(1, 2));
        QCOMPARE(removed.size(), 1);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(textAt(model, 0, 0), u"a"_s);
        QCOMPARE(textAt(model, 1, 0), u"d"_s);
        QCOMPARE(textAt(model, 1, 1), u"4"_s);
        // The editable flags travel with their rows
        QVERIFY(editableAt(model, 1, 1));
    }

    void removeRowsRefusesBadRanges() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>1"_s, u"b<#>2"_s });

        QVERIFY(!model.removeRows(0, 0));
        QVERIFY(!model.removeRows(-1, 1));
        QVERIFY(!model.removeRows(1, 2));
        QVERIFY(!model.removeRows(2, 1));
        QVERIFY(!model.removeRows(0, 1, model.index(0, 0)));
        QCOMPARE(model.rowCount(), 2);

        QVERIFY(model.removeRows(0, 2));
        QCOMPARE(model.rowCount(), 0);
    }

    void clearDropsEverythingButKeepsTheModelUsable() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b<#>c"_s });
        model.setHeaderLabels({ u"x"_s });

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        model.clear();

        QCOMPARE(reset.size(), 1);
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.columnCount(), 0);
        QCOMPARE(model.headerData(0, Qt::Horizontal).toString(), u"1"_s);

        fillModel(model, { u"d<#>e"_s });
        QCOMPARE(textAt(model, 0, 1), u"e"_s);
    }

    void itemPointersFollowTheCells() {
        TranslationTableModel model;
        fillModel(model, { u"a<#>b"_s, u"c<#>d"_s });

        QCOMPARE(*model.item(1, 1), u"d"_s);
        QCOMPARE(*model.itemFromIndex(model.index(0, 0)), u"a"_s);
        QVERIFY(model.item(0, 1) == model.itemFromIndex(model.index(0, 1)));
    }

    void tableModelObeysTheModelContract() {
        TranslationTableModel model;
        const QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        fillModel(model, { u"a<#>b"_s, u"<!>BOOKMARK<#>x"_s, u"c<#>d<#>e"_s });
        model.setHeaderLabels({ u"S"_s, u"T"_s });
        model.appendColumn({ u"1"_s, u"2"_s, u"3"_s });
        model.insertRow(1, { u"<!>BOOKMARK"_s, u"y"_s });
        QVERIFY(model.setData(model.index(0, 1), u"z"_s));
        QVERIFY(model.removeRows(0, 2));
        model.clear();
    }

    // TabListModel
    void tabListStartsEmpty() {
        TabListModel model;

        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.flags(QModelIndex()), Qt::NoItemFlags);
    }

    void tabListHoldsItsTabs() {
        TabListModel model;
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);

        vector<TabListItem> items;
        items.push_back(TabListItem{ u"Map001"_s, TabListItemProps(120, 45, false) });
        items.push_back(TabListItem{ u"Items"_s, TabListItemProps(8, 8, true) });
        model.setTabs(std::move(items));

        QCOMPARE(reset.size(), 1);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.tab(0).name, u"Map001"_s);
        QCOMPARE(model.tab(0).properties.get<"total">(), i32{ 120 });
        QCOMPARE(model.tab(0).properties.get<"translated">(), i32{ 45 });
        QVERIFY(!model.tab(0).properties.get<"completed">());
        QCOMPARE(model.tab(1).name, u"Items"_s);
        QVERIFY(model.tab(1).properties.get<"completed">());
    }

    // The tabs are edited in place, as the list shows counts that change as the user translates
    void tabListTabsAreEditableInPlace() {
        TabListModel model;

        vector<TabListItem> items;
        items.push_back(TabListItem{ u"Map001"_s, TabListItemProps(10, 0, false) });
        model.setTabs(std::move(items));

        model.tab(0).properties.set<"translated">(7);
        model.tab(0).properties.set<"completed">(true);

        QCOMPARE(model.tab(0).properties.get<"translated">(), i32{ 7 });
        QCOMPARE(model.tab(0).properties.get<"total">(), i32{ 10 });
        QVERIFY(model.tab(0).properties.get<"completed">());
    }

    void tabListSetTabsReplacesAndClearEmpties() {
        TabListModel model;

        vector<TabListItem> first;
        first.push_back(TabListItem{ u"a"_s, TabListItemProps(1, 1, true) });
        first.push_back(TabListItem{ u"b"_s, TabListItemProps(1, 1, true) });
        model.setTabs(std::move(first));

        vector<TabListItem> second;
        second.push_back(TabListItem{ u"c"_s, TabListItemProps(1, 0, false) });
        model.setTabs(std::move(second));

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.tab(0).name, u"c"_s);

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        model.clear();
        QCOMPARE(reset.size(), 1);
        QCOMPARE(model.rowCount(), 0);
    }

    void tabListRowsAreEnabledAndSelectable() {
        TabListModel model;

        vector<TabListItem> items;
        items.push_back(TabListItem{ u"a"_s, TabListItemProps(1, 0, false) });
        model.setTabs(std::move(items));

        const Qt::ItemFlags flags = model.flags(model.index(0));
        QVERIFY(flags.testFlag(Qt::ItemIsEnabled));
        QVERIFY(flags.testFlag(Qt::ItemIsSelectable));
        QVERIFY(!flags.testFlag(Qt::ItemIsEditable));
        // The delegate paints the rows, so the model gives no data of its own
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void tabListObeysTheModelContract() {
        TabListModel model;
        const QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        vector<TabListItem> items;
        items.push_back(TabListItem{ u"a"_s, TabListItemProps(1, 0, false) });
        items.push_back(TabListItem{ u"b"_s, TabListItemProps(2, 2, true) });
        model.setTabs(std::move(items));
        model.clear();
    }
};

QTEST_GUILESS_MAIN(TestModels)
#include "tst_models.moc"
