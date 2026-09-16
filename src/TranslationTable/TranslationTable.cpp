#include "TranslationTable.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Notice.hpp"
#include "TranslationTableDelegate.hpp"
#include "TranslationTableHeader.hpp"
#include "TranslationTableModel.hpp"

#include <QClipboard>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>

TranslationTable::TranslationTable(QWidget* const parent) :
    QTableView(parent),

    model_(new TranslationTableModel(this)),
    header_(new TranslationTableHeader(this)),
    delegate(new TranslationTableDelegate(this)) {
    setModel(model_);
    setHorizontalHeader(header_);
    setItemDelegate(delegate);

    setVerticalScrollMode(QTableView::ScrollPerPixel);
    setHorizontalScrollMode(QTableView::ScrollPerPixel);

    setEditTriggers(QTableView::DoubleClicked | QTableView::AnyKeyPressed);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSortingEnabled(false);

    setSelectionBehavior(QTableView::SelectItems);
    setSelectionMode(QTableView::ExtendedSelection);

    verticalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignTop);
    verticalHeader()->setSectionsClickable(false);
    verticalHeader()->setHighlightSections(true);
    verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

    connect(model_, &TranslationTableModel::translatedChanged, this, &TranslationTable::translatedChanged);

    connect(model_, &TranslationTableModel::bookmarkChanged, this, &TranslationTable::bookmarkChanged);

    connect(
        selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        [this](const QModelIndex& current, const QModelIndex& /* prev */) -> void {
        if (!current.isValid()) {
            return;
        }

        selectedColumnPerRow.try_emplace(current.row(), scast<i8>(current.column()));
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
        // Seed an anchor column for any row newly touched that doesn't
        // have one yet.
        for (const auto& range : selected) {
            for (i32 row = range.top(); row <= range.bottom(); row++) {
                selectedColumnPerRow.try_emplace(row, scast<i8>(range.left()));
            }
        }

        QItemSelection toDeselect;

        for (const auto& range : selectionModel()->selection()) {
            for (i32 row = range.top(); row <= range.bottom(); row++) {
                const i8 keepColumn = selectedColumnPerRow.try_emplace(row, scast<i8>(range.left())).first->second;

                for (i32 col = range.left(); col <= range.right(); col++) {
                    if (col != keepColumn) {
                        const QModelIndex idx = model()->index(row, col);
                        toDeselect.select(idx, idx);
                    }
                }
            }
        }

        if (!toDeselect.isEmpty()) {
            selectionModel()->blockSignals(true);
            selectionModel()->select(toDeselect, QItemSelectionModel::Deselect);
            selectionModel()->blockSignals(false);
        }

        for (auto iter = selectedColumnPerRow.begin(); iter != selectedColumnPerRow.end();) {
            if (selectionModel()->rowIntersectsSelection(iter->first, rootIndex())) {
                iter++;
            } else {
                iter = selectedColumnPerRow.erase(iter);
            }
        }
    }
    );

    connect(this, &TranslationTable::pressed, this, [this](const QModelIndex& index) -> void {
        if (qApp->mouseButtons() != Qt::RightButton) {
            if (index.column() == 0 && (model_->index(index.row(), 1).flags() & Qt::ItemIsEditable) != 0) {
                qApp->clipboard()->setText(index.data(Qt::EditRole).toString());
                present(
                    this,
                    NOTICE("Source text from row %1 is copied to clipboard.", Info, Status, itos(index.row() + 1).qsv())
                );
            }
            return;
        }

        if (!index.isValid()) {
            return;
        }

        if ((model_->index(index.row(), 1).flags() & Qt::ItemIsEditable) == 0) {
            return;
        }

        auto menu = QMenu(this);

        const QAction* const removeRowAction = menu.addAction(tr("Remove Row"));
        const QAction* const bookmarkRowAction = menu.addAction(tr("Bookmark Row"));

        const QAction* const selectedAction = menu.exec(QCursor::pos());

        if (selectedAction == removeRowAction) {
            model_->removeRow(index.row());
        } else if (selectedAction == bookmarkRowAction) {
            model_->insertRow(index.row(), { BOOKMARK_COMMENT.toString() });
            emit bookmarked(index.row() + 1);
        }
    });

    connect(header_, &TranslationTableHeader::addButtonClicked, this, [this] -> void {
        model_->appendColumn({});

        emit columnAdded();

        model_->setHeaderData(model_->columnCount() - 1, Qt::Horizontal, tr("Translation"));
        header_->resizeSection(model_->columnCount() - 1, DEFAULT_COLUMN_WIDTH);
    });

    connect(
        model_,
        &TranslationTableModel::headerDataChanged,
        this,
        [this](const Qt::Orientation /* orientation */, const i32 logicalFirst, const i32 /* logicalLast */) -> void {
        if (logicalFirst == 0) {
            return;  // source column
        }

        emit columnRenamed(logicalFirst, model_->headerData(logicalFirst, Qt::Horizontal).toString());
    }
    );

    connect(
        header_,
        &QHeaderView::sectionResized,
        this,
        [this](const i32 logicalIndex, const i32 /* oldSize */, const i32 newSize) -> void {
        emit columnResized(logicalIndex, newSize);
    }
    );

    connect(
        model_,
        &TranslationTableModel::rowsAboutToBeRemoved,
        this,
        [this](
            const QModelIndex& /* parent*/,
            const i32 first,
            const i32 /* last */
        ) -> void {
        const QString* const sourceText = model_->item(first, 0);
        auto flags = RowFlags(0);

        if (sourceText->startsWith(COMMENT_PREFIX)) {
            flags |= RowFlags::CommentFlag;

            if (*sourceText == BOOKMARK_COMMENT) {
                flags |= RowFlags::BookmarkFlag;
            }
        } else {
            for (i32 column = 1; column < model_->columnCount(); column++) {
                if (model_->item(first, column)->isEmpty()) {
                    flags |= RowFlags::TranslatedFlag;
                    break;
                }
            }
        }

        emit rowRemoved(RemovedRowInfo(first, flags));
    }
    );

    connect(delegate, &TranslationTableDelegate::inputFocused, this, &TranslationTable::inputFocused);

    connect(delegate, &TranslationTableDelegate::textChanged, this, &TranslationTable::textChanged);
}

void TranslationTable::insertTranslation(const QString& translation) {
    const QModelIndex index = currentIndex();

    if (!index.isValid()) {
        return;
    }

    QString* const item = model_->itemFromIndex(index);

    if (!item->isEmpty()) {
        const auto selected = QMessageBox::question(
            this,
            tr("Cell is not empty"),
            tr("Selected cell is not empty. Overwrite its contents with the translation?")
        );

        if (selected != QMessageBox::Yes) {
            qInfo().noquote() << u"Overwriting a non-empty cell was rejected by user."_qsv;
            return;
        }
    }

    if (state() == QTableView::EditingState) {
        delegate->setText(translation);
        return;
    }

    *item = translation;
};

void TranslationTable::init(const Settings* const settings, const ProjectSettings* const projectSettings) const {
    delegate->init(projectSettings, settings);
}

void TranslationTable::fill(
    const span<QStringView>& lines,
    const vector<ColumnInfo>& columns,
    const QString& filename
) {
    model_->fill(lines, filename);

    auto headerLabels = QStringList(model_->columnCount());

    for (i32 column = 0; column < model_->columnCount(); column++) {
        headerLabels[column] = columns[column].name;
        header_->resizeSection(column, columns[column].width);
    }

    model_->setHeaderLabels(std::move(headerLabels));

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

    ranges::sort(indexes, [](const QModelIndex& aIndex, const QModelIndex& bIndex) -> bool {
        return aIndex.row() < bIndex.row();
    });

    QStringList rowData;

    for (const QModelIndex& index : indexes) {
        QString cellData = index.data(Qt::EditRole).toString();

        if (cellData.isEmpty() || (index.flags() & Qt::ItemIsEditable) == 0) {
            continue;
        }

        cellData.replace(u'\n', u'\1');

        rowData.append(cellData);
    }

    const QString clipboardText = rowData.join(u'\n');

    qApp->clipboard()->setText(clipboardText);

    return rowData.size();
}

auto TranslationTable::cut() -> u32 {
    const QModelIndexList indexes = selectionModel()->selectedIndexes();

    if (indexes.isEmpty()) {
        return 0;
    }

    copy();

    u32 cut = 0;

    for (const QModelIndex& index : indexes) {
        if (index.data(Qt::EditRole).toString().isEmpty() || (index.flags() & Qt::ItemIsEditable) != 0) {
            cut++;
            model_->setData(index, QString(), Qt::EditRole);
            resizeRowToContents(index.row());
        }
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

    QStringList rows = clipboardText.split(u'\n');

    for (QString& row : rows) {
        row.replace(u'\1', u'\n');
    }

    const i32 column = indexes.front().column();

    vector<i32> selectedRows;
    selectedRows.reserve(scast<usize>(indexes.size()));

    for (const QModelIndex& idx : indexes) {
        selectedRows.push_back(idx.row());
    }

    ranges::sort(selectedRows);
    selectedRows.erase(ranges::unique(selectedRows).begin(), selectedRows.end());

    u32 pasted = 0;

    if (selectedRows.size() <= 1) {
        const i32 firstRow = selectedRows.front();
        const i32 maxRow = model_->rowCount() - 1;

        i32 row = firstRow;
        i32 idx = 0;

        while (idx < rows.size() && row <= maxRow) {
            const QModelIndex dst = model_->index(row, column);

            if (dst.isValid() && ((dst.flags() & Qt::ItemIsEditable) != 0)) {
                if (model_->setData(dst, rows[idx], Qt::EditRole)) {
                    pasted++;
                    resizeRowToContents(row);
                }
                idx++;
            }

            row++;
        }
    } else {
        const i32 count = min(scast<i32>(rows.size()), scast<i32>(selectedRows.size()));

        for (i32 idx = 0; idx < count; idx++) {
            const i32 row = selectedRows[scast<usize>(idx)];
            const QModelIndex dst = model_->index(row, column);

            if (dst.isValid() && ((dst.flags() & Qt::ItemIsEditable) != 0)) {
                if (model_->setData(dst, rows[idx], Qt::EditRole)) {
                    pasted++;
                }
            }
        }
    }

    return pasted;
}

auto TranslationTable::activeInput() -> TranslationInput* {
    return delegate->getActiveInput();
}