#pragma once

#include "Enums.hpp"
#include "FWD.hpp"

#ifdef ENABLE_NUSPELL
#include <nuspell/dictionary.hxx>
#endif

#include <QStyledItemDelegate>

class TranslationTableDelegate final : public QStyledItemDelegate {
    Q_OBJECT

   public:
    explicit TranslationTableDelegate(QObject* parent = nullptr);

#ifdef ENABLE_NUSPELL
    void initializeDictionary();
#endif

    void init(
        const Algorithm* const algorithm,
        const u16* const hint,
        const bool* const enabled
    ) {
        this->algorithm = algorithm;
        lengthHint = hint;
        this->whitespaceHighlightingEnabled = enabled;
    }

    void setText(const QString& text);
    auto createEditor(
        QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QWidget* override;
    void
    setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(
        QWidget* editor,
        QAbstractItemModel* model,
        const QModelIndex& index
    ) const override;
    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override;
    auto eventFilter(QObject* editor, QEvent* event) -> bool override;

   signals:
    void inputFocused();
    void textChanged(const QString& text);

   private:
#ifdef ENABLE_NUSPELL
    mutable nuspell::Dictionary dictionary;
#endif

    const Algorithm* algorithm;
    const u16* lengthHint;
    const bool* whitespaceHighlightingEnabled;

    mutable QPlainTextEdit* activeInput;
    mutable u32 activeRow;
};