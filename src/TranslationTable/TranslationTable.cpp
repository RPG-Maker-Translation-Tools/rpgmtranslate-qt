#include "TranslationTable.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "TranslationTableDelegate.hpp"
#include "TranslationTableHeader.hpp"
#include "TranslationTableModel.hpp"
#include "Utils.hpp"

#include <QClipboard>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>

// TODO: Syntax highlighting for some Yanfly Message Core sequences

TranslationTable::TranslationTable(QWidget* const parent) :
    QTableView(parent),

    model_(new TranslationTableModel(this)),
    header_(new TranslationTableHeader(this)),
    delegate(new TranslationTableDelegate(this)) {
    setModel(model_);
    setHorizontalHeader(header_);
    setItemDelegate(delegate);

    setVerticalScrollMode(QTableView::ScrollPerPixel);
    setEditTriggers(QTableView::DoubleClicked | QTableView::AnyKeyPressed);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSortingEnabled(false);

    setSelectionBehavior(QTableView::SelectItems);
    setSelectionMode(QTableView::ContiguousSelection);

    verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
    verticalHeader()->setSectionsClickable(false);
    verticalHeader()->setHighlightSections(true);
    verticalHeader()->setSectionResizeMode(
        QHeaderView::ResizeMode::ResizeToContents
    );

    connect(
        model_,
        &TranslationTableModel::translatedChanged,
        this,
        &TranslationTable::translatedChanged
    );

    connect(
        model_,
        &TranslationTableModel::bookmarkChanged,
        this,
        &TranslationTable::bookmarkChanged
    );

    connect(
        selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex&) -> void {
        selectedColumn = i8(current.isValid() ? current.column() : -1);
    }
    );

    connect(
        selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](
            const QItemSelection& selected,
            const QItemSelection& /* prev */
        ) -> void {
        if (selectedColumn < 0) {
            const QModelIndex current = currentIndex();

            if (current.isValid()) {
                selectedColumn = i8(current.column());
            } else {
                const auto indexes = selectionModel()->selectedIndexes();

                if (indexes.isEmpty()) {
                    return;
                }

                selectedColumn = i8(indexes.first().column());
            }
        }

        QItemSelection toDeselect;

        for (const auto& range : selectionModel()->selection()) {
            if (range.left() != selectedColumn ||
                range.right() != selectedColumn) {
                toDeselect.append(range);
            }
        }

        if (!toDeselect.isEmpty()) {
            selectionModel()->blockSignals(true);
            selectionModel()->select(toDeselect, QItemSelectionModel::Deselect);
            selectionModel()->blockSignals(false);
        }

        if (selectionModel()->selectedIndexes().isEmpty()) {
            selectedColumn = -1;
        }
    }
    );

    connect(
        this,
        &TranslationTable::pressed,
        this,
        [this](const QModelIndex& index) -> void {
        if (qApp->mouseButtons() != Qt::RightButton) {
            return;
        }

        if (!index.isValid()) {
            return;
        }

        if (!model_->item(index.row(), 1)->isEditable()) {
            return;
        }

        auto* const menu = new QMenu(this);

        const QAction* const removeRowAction =
            menu->addAction(tr("Remove Row"));
        const QAction* const bookmarkRowAction =
            menu->addAction(tr("Bookmark Row"));

        const QAction* const selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == removeRowAction) {
            model_->removeRow(index.row());
        } else if (selectedAction == bookmarkRowAction) {
            auto* const sourceItem =
                new QStandardItem(BOOKMARK_COMMENT.toString());
            sourceItem->setEditable(false);

            model_->insertRow(index.row(), { sourceItem, new QStandardItem() });
            emit bookmarked(index.row() + 1);
        }
    }
    );

    connect(
        header_,
        &TranslationTableHeader::addButtonClicked,
        this,
        [this] -> void {
        model_->appendColumn({});

        emit columnAdded();

        model_->setHeaderData(
            model_->columnCount() - 1,
            Qt::Horizontal,
            tr("Translation")
        );
        header_->resizeSection(model_->columnCount() - 1, DEFAULT_COLUMN_WIDTH);
    }
    );

    connect(
        model_,
        &QStandardItemModel::headerDataChanged,
        this,
        [this](
            const Qt::Orientation orientation,
            const u8 logicalFirst,
            const u8 logicalLast
        ) -> void {
        emit columnRenamed(
            logicalFirst,
            model_->headerData(logicalFirst, Qt::Horizontal).toString()
        );
    }
    );

    connect(
        header_,
        &QHeaderView::sectionResized,
        this,
        [this](
            const u8 logicalIndex,
            const u16 /* oldSize */,
            const u16 newSize
        ) -> void { emit columnResized(logicalIndex, newSize); }
    );

    connect(
        model_,
        &TranslationTableModel::rowsAboutToBeRemoved,
        this,
        [this](
            const QModelIndex& /* parent*/,
            const int first,
            const int /* last */
        ) -> void {
        const QString sourceText = model_->item(first, 0)->text();
        auto flags = RowFlags(0);

        if (sourceText.startsWith(COMMENT_PREFIX)) {
            flags |= RowFlags::CommentFlag;

            if (sourceText == BOOKMARK_COMMENT) {
                flags |= RowFlags::BookmarkFlag;
            }
        } else {
            for (const u8 column : range<u8>(1, model_->columnCount())) {
                if (model_->item(first, column)->text().isEmpty()) {
                    flags |= RowFlags::TranslatedFlag;
                    break;
                }
            }
        }

        emit rowRemoved(RemovedRowInfo(first, flags));
    }
    );

    connect(
        delegate,
        &TranslationTableDelegate::inputFocused,
        this,
        &TranslationTable::inputFocused
    );

    connect(
        delegate,
        &TranslationTableDelegate::textChanged,
        this,
        &TranslationTable::textChanged
    );
}

void TranslationTable::insertTranslation(const QString& translation) {
    const QModelIndex index = currentIndex();

    if (!index.isValid()) {
        return;
    }

    auto* const item = model_->itemFromIndex(index);

    if (!item->text().isEmpty()) {
        const auto selected = QMessageBox::question(
            this,
            tr("Cell is not empty"),
            tr(
                "Selected cell is not empty. Overwrite its contents with the translation?"
            )
        );

        if (selected != QMessageBox::Yes) {
            return;
        }
    }

    if (state() == QTableView::EditingState) {
        delegate->setText(translation);
        return;
    }

    item->setText(translation);
};

void TranslationTable::init(
    const Algorithm* const algorithm,
    const u16* const hint,
    const bool* const enabled
) const {
    delegate->init(algorithm, hint, enabled);

#ifdef ENABLE_HUNSPELL
    delegate->initializeDictionary();
#endif
}

#ifdef ENABLE_HUNSPELL
void TranslationTable::initializeDictionary() const {
    delegate->initializeDictionary();
};
#endif

void TranslationTable::addRow(
    const QStringView source,
    const QSVList& translations
) {
    QList<QStandardItem*> items(1 + translations.size());
    auto* const sourceItem =
        new QStandardItem(qsvReplace(source, NEW_LINE, LINE_FEED));
    sourceItem->setEditable(false);
    sourceItem->setData(true, Qt::UserRole);
    items[0] = sourceItem;

    for (const auto [column, translation] : views::enumerate(translations)) {
        items[column + 1] =
            new QStandardItem(qsvReplace(translation, NEW_LINE, LINE_FEED));
    }

    model_->appendRow(items);
}

void TranslationTable::addCommentRow(const QStringView comment) {
    const bool editable = comment.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX) ||
                          comment.startsWith(BOOKMARK_COMMENT);

    auto* const commentItem = new QStandardItem(
        editable ? comment.sliced(0, comment.indexOf(SEPARATORL1)).toString()
                 : comment.toString()
    );
    commentItem->setEditable(false);

    auto* const counterpartItem = new QStandardItem();
    counterpartItem->setEditable(editable);

    if (editable) {
        const u32 start = comment.indexOf(SEPARATORL1) + SEPARATORL1.size();
        const isize end = comment.indexOf(SEPARATORL1, start);

        counterpartItem->setText((end == -1
                                      ? comment.sliced(start)
                                      : comment.sliced(start, end - start))
                                     .toString());
    }

    model_->appendRow({ commentItem, counterpartItem });
}

void TranslationTable::fill(
    const span<QStringView>& lines,
    const vector<ColumnInfo>& columns,
    const QString& filename
) {
    blockSignals(true);
    setUpdatesEnabled(false);

    model_->clear();

    for (const auto [row, line] : views::enumerate(lines)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const auto parts = lineParts(line, row, filename);

        if (parts.isEmpty()) {
            continue;
        }

        const QStringView source = getSource(parts);
        const QSVList translations = getTranslations(parts);

        if (source.startsWith(COMMENT_PREFIX)) {
            addCommentRow(line);
            continue;
        }

        addRow(source, translations);
    }

    auto headerLabels = QStringList(model_->columnCount());

    for (const u8 column : range<u8>(0, model_->columnCount())) {
        headerLabels[column] = columns[column].name;
        header_->resizeSection(column, columns[column].width);
    }

    model_->setHorizontalHeaderLabels(headerLabels);

    blockSignals(false);
    setUpdatesEnabled(true);

    viewport()->update();
}

void TranslationTable::keyPressEvent(QKeyEvent* const event) {
    if (event->matches(QKeySequence::Copy)) {
        const u32 count = copy();
        emit multilineAction(MultilineAction::Copy, count);
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Cut)) {
        const u32 count = cut();
        emit multilineAction(MultilineAction::Cut, count);
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Paste)) {
        const u32 count = paste();
        emit multilineAction(MultilineAction::Paste, count);
        event->accept();
        return;
    }

    QTableView::keyPressEvent(event);
}

auto TranslationTable::copy() -> u32 {
    QModelIndexList indexes = selectionModel()->selectedIndexes();

    if (indexes.isEmpty()) {
        return 0;
    }

    ranges::sort(
        indexes,
        [](const QModelIndex& aIndex, const QModelIndex& bIndex) -> bool {
        return aIndex.row() < bIndex.row();
    }
    );

    QStringList rowData;

    for (const QModelIndex& index : indexes) {
        QString cellData = model()->data(index, Qt::EditRole).toString();
        cellData.replace("\\"_L1, "\\\\"_L1);
        cellData.replace("\n"_L1, "\\n"_L1);

        rowData.append(cellData);
    }

    const QString clipboardText = rowData.join("\n");

    qApp->clipboard()->setText(clipboardText);

    return rowData.size();
}

auto TranslationTable::cut() -> u32 {
    QModelIndexList indexes = selectionModel()->selectedIndexes();

    if (indexes.isEmpty()) {
        return 0;
    }

    copy();

    u32 cut = 0;

    for (const QModelIndex& index : indexes) {
        if (!model_->data(index, Qt::EditRole).toString().isEmpty()) {
            cut++;
        }

        model_->setData(index, QString(), Qt::EditRole);
    }

    return cut;
}

auto TranslationTable::paste() -> u32 {
    const QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) {
        return 0;
    }

    const QString clipboardText = qApp->clipboard()->text();
    if (clipboardText.isEmpty()) {
        return 0;
    }

    QStringList rows = clipboardText.split('\n');

    for (QString& row : rows) {
        row.replace("\\n"_L1, "\n"_L1);
        row.replace("\\\\"_L1, "\\"_L1);
    }

    const u8 column = indexes.front().column();

    u32 firstRow = indexes.front().row();
    for (const QModelIndex& idx : indexes) {
        firstRow = std::min(firstRow, u32(idx.row()));
    }

    const u32 maxRow = model_->rowCount() - 1;
    const u32 available = maxRow - firstRow + 1;
    const u32 count = std::min(u32(rows.size()), available);

    u32 pasted = 0;

    for (u32 i = 0; i < count; i++) {
        const u32 row = firstRow + i;
        const QModelIndex dst = model_->index(i32(row), column);

        if (dst.isValid() && ((model_->flags(dst) & Qt::ItemIsEditable) != 0)) {
            if (model_->setData(dst, rows[i], Qt::EditRole)) {
                pasted++;
            }
        }
    }

    return pasted;
}