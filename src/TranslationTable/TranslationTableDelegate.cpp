#include "TranslationTableDelegate.hpp"

#include "Aliases.hpp"
#include "TranslationInput.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"

#include <QApplication>
#include <QModelIndex>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>

namespace {
constexpr i32 MIN_ROW_HEIGHT = 30;
constexpr i32 PAD_X = 4;
constexpr i32 PAD_Y = 4;
}  // namespace

TranslationTableDelegate::TranslationTableDelegate(QObject* const parent) : QStyledItemDelegate(parent) {}

auto TranslationTableDelegate::table() const -> TranslationTable* {
    return scast<TranslationTable*>(parent());
}

void TranslationTableDelegate::commitAndClose(QWidget* const editor) {
    emit commitData(editor);
    emit closeEditor(editor, QStyledItemDelegate::SubmitModelCache);
}

void TranslationTableDelegate::editLater(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    TranslationTable* const tableView = table();

    QTimer::singleShot(0, tableView, [tableView, index] -> void {
        tableView->setCurrentIndex(index);
        tableView->scrollTo(index);
        tableView->edit(index);
    });
}

auto TranslationTableDelegate::moveEdit(QWidget* const editor, const i8 rowStep, const i8 colStep) -> bool {
    const TranslationTable* const tableView = table();
    const QModelIndex current = tableView->currentIndex();

    if (!current.isValid()) {
        return false;
    }

    const TranslationTableModel* const model = tableView->model();
    const i32 rowCount = model->rowCount();
    const i32 colCount = model->columnCount();

    i32 row = current.row() + rowStep;
    i32 column = current.column() + colStep;

    QModelIndex next;

    // Invalid indexes have no flags, so out-of-range cells are never editable
    while (row >= 0 && row < rowCount && column >= 0 && column < colCount) {
        const QModelIndex candidate = model->index(row, column);

        if ((candidate.flags() & Qt::ItemIsEditable) != 0) {
            next = candidate;
            break;
        }

        row += rowStep;
        column += colStep;
    }

    commitAndClose(editor);
    editLater(next);
    return true;
}

auto TranslationTableDelegate::jumpEdit(QWidget* const editor, const bool toFirst) -> bool {
    const TranslationTable* const tableView = table();
    const QModelIndex current = tableView->currentIndex();

    if (!current.isValid()) {
        return false;
    }

    const TranslationTableModel* const model = tableView->model();
    const i32 rowCount = model->rowCount();

    QModelIndex target;

    for (i32 step = 0; step < rowCount; step++) {
        const QModelIndex candidate = model->index(toFirst ? step : rowCount - 1 - step, current.column());

        if ((candidate.flags() & Qt::ItemIsEditable) != 0) {
            target = candidate;
            break;
        }
    }

    commitAndClose(editor);
    editLater(target);
    return true;
}

auto TranslationTableDelegate::createEditor(
    QWidget* const parent,
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& index
) const -> QWidget* {
    auto* const editor = new TranslationInput(projectSettings, settings, parent);

    activeInput = editor;

    // The row can move while the editor is open (a row is removed or a bookmark is inserted above it)
    const auto editedIndex = QPersistentModelIndex(index);

    // Signals can't be emitted through a const `this`
    auto* const that = ccast<TranslationTableDelegate*>(this);

    connect(editor, &TranslationInput::textChanged, this, [that, editedIndex] -> void {
        emit that->textChanged();

        if (editedIndex.isValid()) {
            emit that->sizeHintChanged(editedIndex);
            that->table()->resizeRowToContents(editedIndex.row());
        }
    });

    // Lines the text wraps into are counted when the editor has laid them out, which is after the text changed
    connect(editor, &TranslationInput::contentHeightChanged, this, [that, editedIndex] -> void {
        if (editedIndex.isValid()) {
            that->table()->resizeRowToContents(editedIndex.row());
        }
    });

    connect(editor, &TranslationInput::focusNextInput, this, [that, editor] -> void { that->moveEdit(editor, 1, 0); });

    // The next editor can be created before this one is deleted
    connect(editor, &QObject::destroyed, this, [this, editor] -> void {
        if (activeInput == editor) {
            activeInput = nullptr;
        }
    });

    QTimer::singleShot(0, editor, [editor] -> void { editor->verticalScrollBar()->setValue(0); });

    return editor;
}

void TranslationTableDelegate::setEditorData(QWidget* const editor, const QModelIndex& index) const {
    const QString value = index.model()->data(index, Qt::EditRole).toString();
    auto* const textEdit = scast<TranslationInput*>(editor);
    textEdit->setPlainText(value);

    auto* const that = ccast<TranslationTableDelegate*>(this);
    emit that->inputFocused();

    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    textEdit->setTextCursor(cursor);
}

void TranslationTableDelegate::setModelData(
    QWidget* const editor,
    QAbstractItemModel* const model,
    const QModelIndex& index
) const {
    const auto* const textEdit = scast<TranslationInput*>(editor);
    model->setData(index, textEdit->toPlainText(), Qt::EditRole);
}

auto TranslationTableDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize {
    const QWidget* const editor = table()->indexWidget(index);

    const auto* const textEdit = qobject_cast<const TranslationInput*>(editor);

    // The width is that of the model's text (it is used when a column is fitted to its contents)
    const QString text = index.data(Qt::DisplayRole).toString();

    const auto fontMetrics = QFontMetrics(option.font);

    // The editor wraps its lines, so it counts the lines it has; a cell that is not edited is drawn in one line per
    // line feed
    const i32 lines =
        textEdit != nullptr ? max(1, textEdit->document()->lineCount()) : scast<i32>(text.count(u'\n') + 1);
    const i32 height = (fontMetrics.lineSpacing() * lines) + 10;

    return { fontMetrics.horizontalAdvance(text), max(height, MIN_ROW_HEIGHT) };
}

void TranslationTableDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();

    const QStyle* const style = (opt.widget != nullptr) ? opt.widget->style() : qApp->style();

    if ((opt.state & QStyle::State_Selected) != 0) {
        painter->fillRect(opt.rect, opt.palette.highlight());
    } else {
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    }

    if (!opt.text.isEmpty()) {
        const QRect paddedRect = opt.rect.adjusted(PAD_X, PAD_Y, -PAD_X, -PAD_Y);
        const QPalette::ColorRole textRole =
            ((opt.state & QStyle::State_Selected) != 0) ? QPalette::HighlightedText : QPalette::Text;

        painter->setFont(opt.font);
        style->drawItemText(
            painter,
            paddedRect,
            scast<i32>(opt.displayAlignment),
            opt.palette,
            (opt.state & QStyle::State_Enabled) != 0,
            opt.text,
            textRole
        );
    }

    painter->restore();
}

auto TranslationTableDelegate::eventFilter(QObject* const editor, QEvent* const event) -> bool {
    if (event->type() != QEvent::KeyPress) {
        return QStyledItemDelegate::eventFilter(editor, event);
    }

    const auto* const keyEvent = scast<QKeyEvent*>(event);
    auto* const widget = scast<QWidget*>(editor);

    if (keyEvent->matches(QKeySequence::Cancel)) {
        commitAndClose(widget);
        return true;
    }

    const Qt::KeyboardModifiers mods = keyEvent->modifiers();
    const i32 key = keyEvent->key();

    const bool ctrl = (mods & Qt::ControlModifier) != 0;
    const bool shift = !ctrl && ((mods & Qt::ShiftModifier) != 0);

    if (shift) {
        i8 rowStep = 0;
        i8 colStep = 0;

        switch (key) {
            case Qt::Key_Up:
                rowStep = -1;
                break;
            case Qt::Key_Down:
                rowStep = 1;
                break;
            case Qt::Key_Left:
                colStep = -1;
                break;
            case Qt::Key_Right:
                colStep = 1;
                break;
            default:
                return QStyledItemDelegate::eventFilter(editor, event);
        }

        return moveEdit(widget, rowStep, colStep) || QStyledItemDelegate::eventFilter(editor, event);
    }

    if (ctrl && (key == Qt::Key_Up || key == Qt::Key_Down)) {
        return jumpEdit(widget, key == Qt::Key_Up) || QStyledItemDelegate::eventFilter(editor, event);
    }

    return QStyledItemDelegate::eventFilter(editor, event);
}
