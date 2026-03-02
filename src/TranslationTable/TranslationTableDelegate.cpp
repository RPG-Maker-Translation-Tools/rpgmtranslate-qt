#include "TranslationTableDelegate.hpp"

#include "Aliases.hpp"
#include "SpellHighlighter.hpp"
#include "TranslationInput.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "WhitespaceHighlighter.hpp"

#include <QModelIndex>
#include <QTimer>

TranslationTableDelegate::TranslationTableDelegate(QObject* const parent) :
    QStyledItemDelegate(parent) {}

void TranslationTableDelegate::setText(const QString& text) {
    if (activeInput != nullptr) {
        activeInput->setPlainText(text);
    }
}

auto TranslationTableDelegate::createEditor(
    QWidget* const parent,
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& index
) const -> QWidget* {
    auto* const editor = new TranslationInput(&lengthHint, parent);
    new SpellHighlighter(&dictionary, &algorithm, editor->document());
    new WhitespaceHighlighter(
        &whitespaceHighlightingEnabled,
        editor->document()
    );

    activeRow = index.row();
    activeInput = editor;

    connect(
        editor,
        &TranslationInput::textChanged,
        this,
        [this, editor, index] -> void {
        auto* const that = const_cast<TranslationTableDelegate*>(this);
        auto* const tableView = as<TranslationTable*>(that->parent());
        emit that->sizeHintChanged(index);
        emit that->textChanged(editor->toPlainText());
        tableView->resizeRowToContents(index.row());
    }
    );

    connect(editor, &QObject::destroyed, this, [this] -> void {
        activeInput = nullptr;
    });

    return editor;
}

void TranslationTableDelegate::setEditorData(
    QWidget* const editor,
    const QModelIndex& index
) const {
    const QString value = index.model()->data(index, Qt::EditRole).toString();
    auto* const textEdit = as<TranslationInput*>(editor);
    textEdit->setPlainText(value);

    auto* const that = const_cast<TranslationTableDelegate*>(this);
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
    const auto* const textEdit = as<TranslationInput*>(editor);
    model->setData(index, textEdit->toPlainText(), Qt::EditRole);
}

auto TranslationTableDelegate::sizeHint(
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const -> QSize {
    const auto* const tableView = as<TranslationTable*>(
        as<const TranslationTableDelegate*>(this)->parent()
    );

    const QWidget* const editor = tableView->indexWidget(index);

    const auto* const textEdit = qobject_cast<const TranslationInput*>(editor);

    const QString text = textEdit != nullptr
                             ? textEdit->toPlainText()
                             : index.data(Qt::DisplayRole).toString();

    const auto fontMetrics = QFontMetrics(option.font);
    const u32 lines = text.split('\n').count();
    const u32 height = (fontMetrics.lineSpacing() * lines) + 10;
    return { fontMetrics.horizontalAdvance(text), i32(qMax(height, u32(30))) };
}

auto TranslationTableDelegate::eventFilter(
    QObject* const editor,
    QEvent* const event
) -> bool {
    if (event->type() != QEvent::KeyPress) {
        return QStyledItemDelegate::eventFilter(editor, event);
    }

    const auto* const keyEvent = as<QKeyEvent*>(event);

    if (keyEvent->matches(QKeySequence::Cancel)) {
        emit commitData(as<QWidget*>(editor));
        emit closeEditor(
            as<QWidget*>(editor),
            QStyledItemDelegate::SubmitModelCache
        );
        return true;
    }

    auto* const tableView = as<TranslationTable*>(
        const_cast<TranslationTableDelegate*>(this)->parent()
    );
    if (tableView == nullptr) {
        return QStyledItemDelegate::eventFilter(editor, event);
    }

    const TranslationTableModel* const model = tableView->model();
    if (model == nullptr) {
        return QStyledItemDelegate::eventFilter(editor, event);
    }

    const QModelIndex current = tableView->currentIndex();
    if (!current.isValid()) {
        return QStyledItemDelegate::eventFilter(editor, event);
    }

    const auto isEditable = [model](const QModelIndex& idx) -> bool {
        if (!idx.isValid()) {
            return false;
        }

        return (model->flags(idx) & Qt::ItemIsEditable);
    };

    const i32 rowCount = model->rowCount(current.parent());
    const u32 colCount = model->columnCount(current.parent());

    const auto commitAndClose = [this, editor] -> void {
        emit commitData(as<QWidget*>(editor));
        emit closeEditor(
            as<QWidget*>(editor),
            QStyledItemDelegate::SubmitModelCache
        );
    };

    const auto editIndexAsync = [tableView](const QModelIndex& idx) -> void {
        if (!idx.isValid()) {
            return;
        }

        QTimer::singleShot(0, tableView, [tableView, idx] -> void {
            tableView->setCurrentIndex(idx);
            tableView->scrollTo(idx);
            tableView->edit(idx);
        });
    };

    const Qt::KeyboardModifiers mods = keyEvent->modifiers();
    const i32 key = keyEvent->key();

    const bool ctrl = (mods & Qt::ControlModifier) != 0;
    const bool shift = !ctrl && ((mods & Qt::ShiftModifier) != 0);

    if (shift && (key == Qt::Key_Up || key == Qt::Key_Down ||
                  key == Qt::Key_Left || key == Qt::Key_Right)) {
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
                std::unreachable();
        }

        i32 row = current.row() + rowStep;
        i32 column = current.column() + colStep;

        QModelIndex next;
        while (row >= 0 && row < rowCount && column >= 0 && column < colCount) {
            const QModelIndex candidate =
                model->index(row, column, current.parent());

            if (isEditable(candidate)) {
                next = candidate;
                break;
            }

            row += rowStep;
            column += colStep;
        }

        commitAndClose();

        if (next.isValid()) {
            editIndexAsync(next);
        }

        return true;
    }

    if (ctrl && (key == Qt::Key_Up || key == Qt::Key_Down)) {
        const u8 col = current.column();
        QModelIndex target;

        if (key == Qt::Key_Up) {
            for (i32 row = 0; row < rowCount; row++) {
                const QModelIndex candidate =
                    model->index(row, col, current.parent());

                if (isEditable(candidate)) {
                    target = candidate;
                    break;
                }
            }
        } else {
            for (i32 row = rowCount - 1; row >= 0; row--) {
                const QModelIndex candidate =
                    model->index(row, col, current.parent());

                if (isEditable(candidate)) {
                    target = candidate;
                    break;
                }
            }
        }

        commitAndClose();

        if (target.isValid()) {
            editIndexAsync(target);
        }

        return true;
    }

    return QStyledItemDelegate::eventFilter(editor, event);
}

void TranslationTableDelegate::initializeDictionary() {
    // TODO: Delete unpacked dictionary

    switch (*algorithm) {
        case Algorithm::None:
            break;
        case Algorithm::Arabic:
            break;
        case Algorithm::Armenian:
            break;
        case Algorithm::Basque:
            break;
        case Algorithm::Catalan:
            break;
        case Algorithm::Danish:
            break;
        case Algorithm::Dutch:
        case Algorithm::DutchPorter:
            break;
        case Algorithm::English:
        case Algorithm::Porter:
        case Algorithm::Lovins:
            // TODO: Unpack and load `en_US`
            break;
        case Algorithm::Esperanto:
            break;
        case Algorithm::Estonian:
            break;
        case Algorithm::Finnish:
            break;
        case Algorithm::French:
            // TODO: Unpack and load `fr_FR`
            break;
        case Algorithm::German:
            // TODO: Unpack and load `de`
            break;
        case Algorithm::Greek:
            break;
        case Algorithm::Hindi:
            break;
        case Algorithm::Hungarian:
            break;
        case Algorithm::Indonesian:
            break;
        case Algorithm::Irish:
            break;
        case Algorithm::Italian:
            // TODO: Unpack and load `it_IT`
            break;
        case Algorithm::Lithuanian:
            break;
        case Algorithm::Nepali:
            break;
        case Algorithm::Norwegian:
            break;
        case Algorithm::Portuguese:
            // TODO: Unpack and load `pt_PT`
            break;
        case Algorithm::Romanian:
            break;
        case Algorithm::Russian:
            // TODO: Unpack and load `ru_RU`
            break;
        case Algorithm::Serbian:
            break;
        case Algorithm::Spanish:
            // TODO: Unpack and load `es`
            break;
        case Algorithm::Swedish:
            break;
        case Algorithm::Tamil:
            break;
        case Algorithm::Turkish:
            // TODO: Unpack and load `tr_TR`
            break;
        case Algorithm::Yiddish:
            break;
        case Algorithm::Japanese:
        case Algorithm::Chinese:
        case Algorithm::Korean:
        case Algorithm::Thai:
        case Algorithm::Burmese:
        case Algorithm::Lao:
        case Algorithm::Khmer:
            // TODO: Custom
            break;
    }
}