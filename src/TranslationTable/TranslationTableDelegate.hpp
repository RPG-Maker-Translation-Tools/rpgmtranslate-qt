#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "TranslationInput.hpp"

#include <QStyledItemDelegate>

class TranslationTableDelegate final : public QStyledItemDelegate {
    Q_OBJECT

   public:
    explicit TranslationTableDelegate(QObject* parent = nullptr);

    void init(const ProjectSettings* const projectSettings, const Settings* const settings) {
        this->projectSettings = projectSettings;
        this->settings = settings;
    }

    void setText(const QString& text);
    auto createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
        -> QWidget* override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    auto eventFilter(QObject* editor, QEvent* event) -> bool override;

    auto getActiveInput() const -> TranslationInput* { return activeInput; }

   signals:
    void inputFocused();
    void textChanged();

   private:
    const Settings* settings;
    const ProjectSettings* projectSettings;

    mutable TranslationInput* activeInput = nullptr;
    mutable u32 activeRow;
};
