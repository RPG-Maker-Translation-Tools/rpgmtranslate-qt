#include "TranslationTableDelegate.hpp"

#include "Aliases.hpp"
#include "Settings.hpp"
#include "TranslationHighlighter.hpp"
#include "TranslationInput.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"

#include <QApplication>
#include <QModelIndex>
#include <QPainter>
#include <QSyntaxHighlighter>
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
    auto* const editor = new TranslationInput(projectSettings, parent);

    new TranslationHighlighter(
        &settings->translation,
#ifdef ENABLE_NUSPELL
        &dictionary,
        &dictionaryReady,
#endif
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
    const u32 lines = text.count(u'\n') + 1;
    const u32 height = (fontMetrics.lineSpacing() * lines) + 10;
    return { fontMetrics.horizontalAdvance(text), i32(max(height, u32(30))) };
}

void TranslationTableDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle* const style =
        (opt.widget != nullptr) ? opt.widget->style() : qApp->style();

    style->drawPrimitive(
        QStyle::PE_PanelItemViewItem,
        &opt,
        painter,
        opt.widget
    );

    if (!opt.text.isEmpty()) {
        const QRect paddedRect =
            opt.rect.adjusted(PAD_X, PAD_Y, -PAD_X, -PAD_Y);

        const QPalette::ColorRole textRole =
            ((opt.state & QStyle::State_Selected) != 0)
                ? QPalette::HighlightedText
                : QPalette::Text;

        painter->save();
        painter->setFont(opt.font);
        style->drawItemText(
            painter,
            paddedRect,
            i32(opt.displayAlignment),
            opt.palette,
            (opt.state & QStyle::State_Enabled) != 0,
            opt.text,
            textRole
        );
        painter->restore();
    }
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
            for (const auto row : range(0, rowCount)) {
                const QModelIndex candidate =
                    model->index(row, col, current.parent());

                if (isEditable(candidate)) {
                    target = candidate;
                    break;
                }
            }
        } else {
            for (const auto row : range<-1>(rowCount - 1, -1)) {
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

#ifdef ENABLE_NUSPELL
auto TranslationTableDelegate::initializeDictionary() -> result<void, QString> {
    const QString path = qApp->property("data-location").toString() +
                         u"/dictionaries" +
                         projectSettings->spellcheckDictionary;

    if (projectSettings->spellcheckDictionary.isEmpty() ||
        !QFile::exists(path)) {
        dictionary = nuspell::Dictionary();
        dictionaryReady = false;
    } else {
        try {
            dictionary.load_aff_dic(path.toStdString());
            dictionaryReady = true;
        } catch (nuspell::Dictionary_Loading_Error& err) {
            return Err(QString::fromUtf8(err.what()));
        }
    }

    if (activeInput != nullptr) {
        const auto highlighters =
            activeInput->document()->findChildren<QSyntaxHighlighter*>();

        for (QSyntaxHighlighter* const highlighter : highlighters) {
            highlighter->rehighlight();
        }
    }

    return {};
}
#endif
