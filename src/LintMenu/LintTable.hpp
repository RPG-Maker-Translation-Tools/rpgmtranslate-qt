#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Types.hpp"

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QTableView>

class MatchTableDelegate final : public QStyledItemDelegate {
   public:
    explicit MatchTableDelegate(LintTableModel* model, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;

    [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize override;

   private:
    LintTableModel* model;
};

class LintTableModel final : public QAbstractTableModel {
    Q_OBJECT

   public:
    enum Column : u8 {
        Filename,
        Line,
        SourceMatch,
        TranslationMatch,
        Info,
        ColumnCount
    };

    enum Role : u16 {
        TextRole = Qt::DisplayRole,
        SpansRole = Qt::UserRole + 1
    };

    using Row = LintRow;

    explicit LintTableModel(QObject* const parent = nullptr) : QAbstractTableModel(parent) {}

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const -> i32 override {
        return scast<i32>(rows.size());
    }

    [[nodiscard]] auto columnCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;

    [[nodiscard]] auto flags(const QModelIndex& index) const -> Qt::ItemFlags override;

    [[nodiscard]] auto data(const QModelIndex& index, i32 role) const -> QVariant override;

    [[nodiscard]] auto headerData(i32 section, Qt::Orientation orientation, i32 role) const -> QVariant override;

    void appendRow(Row row);
    auto row(u32 row) -> const Row&;
    void clear();

   private:
    vector<Row> rows;
};

class LintTable final : public QTableView {
    Q_OBJECT

   public:
    explicit LintTable(QWidget* parent = nullptr);

    [[nodiscard]] auto model() const -> LintTableModel*;

    void appendRow(LintTableModel::Row row);
    void appendRows(vector<LintTableModel::Row> rows);

   private:
    LintTableModel* model_;
    MatchTableDelegate* delegate;
};
