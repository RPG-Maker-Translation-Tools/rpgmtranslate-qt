#include "TranslationTable.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
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
    setSelectionMode(QTableView::ContiguousSelection);

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
        [this](const QModelIndex& current, const QModelIndex&) -> void {
        selectedColumn = scast<i8>(current.isValid() ? current.column() : -1);
    }
    );

    connect(
        selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](
            const QItemSelection& /* selected */,
            const QItemSelection& /* prev */
        ) -> void {
        if (selectedColumn < 0) {
            const QModelIndex current = currentIndex();

            if (current.isValid()) {
                selectedColumn = scast<i8>(current.column());
            } else {
                const auto indexes = selectionModel()->selectedIndexes();

                if (indexes.isEmpty()) {
                    return;
                }

                selectedColumn = scast<i8>(indexes.first().column());
            }
        }

        QItemSelection toDeselect;

        for (const auto& range : selectionModel()->selection()) {
            if (range.left() != selectedColumn || range.right() != selectedColumn) {
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

    connect(this, &TranslationTable::pressed, this, [this](const QModelIndex& index) -> void {
        if (qApp->mouseButtons() != Qt::RightButton) {
            return;
        }

        if (!index.isValid()) {
            return;
        }

        if (!model_->item(index.row(), 1).editable()) {
            return;
        }

        auto* const menu = new QMenu(this);

        const QAction* const removeRowAction = menu->addAction(tr("Remove Row"));
        const QAction* const bookmarkRowAction = menu->addAction(tr("Bookmark Row"));

        const QAction* const selectedAction = menu->exec(QCursor::pos());
        delete menu;

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
        const QString* const sourceText = model_->item(first, 0).text();
        auto flags = RowFlags(0);

        if (sourceText->startsWith(COMMENT_PREFIX)) {
            flags |= RowFlags::CommentFlag;

            if (*sourceText == BOOKMARK_COMMENT) {
                flags |= RowFlags::BookmarkFlag;
            }
        } else {
            for (const auto column : range(1, model_->columnCount())) {
                if (model_->item(first, column).text()->isEmpty()) {
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

    auto item = model_->itemFromIndex(index);

    if (!item.text()->isEmpty()) {
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

    *item.text() = translation;
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

    for (const auto column : range(0, model_->columnCount())) {
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
        QString cellData = model()->data(index, Qt::EditRole).toString();
        cellData.replace("\\"_L1, "\\\\"_L1);
        cellData.replace("\n"_L1, "\\n"_L1);

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

    QStringList rows = clipboardText.split(u'\n');

    for (QString& row : rows) {
        row.replace("\\n"_L1, "\n"_L1);
        row.replace("\\\\"_L1, "\\"_L1);
    }

    const i32 column = indexes.front().column();

    i32 firstRow = indexes.front().row();
    for (const QModelIndex& idx : indexes) {
        firstRow = std::min(firstRow, idx.row());
    }

    const u32 maxRow = model_->rowCount() - 1;
    const u32 available = maxRow - firstRow + 1;
    const u32 count = std::min(scast<u32>(rows.size()), available);

    u32 pasted = 0;

    for (const auto idx : range(0, count)) {
        const i32 row = firstRow + idx;
        const QModelIndex dst = model_->index(row, column);

        if (dst.isValid() && ((model_->flags(dst) & Qt::ItemIsEditable) != 0)) {
            if (model_->setData(dst, rows[idx], Qt::EditRole)) {
                pasted++;
            }
        }
    }

    return pasted;
}

auto TranslationTable::activeInput() -> TranslationInput* {
    return delegate->getActiveInput();
}