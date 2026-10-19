#pragma once

#include "Aliases.hpp"

#include <QAbstractTableModel>

class TranslationTableModel final : public QAbstractTableModel {
    Q_OBJECT

   public:
    static constexpr i32 MAX_COLUMNS = 16;

    explicit TranslationTableModel(QObject* parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;
    [[nodiscard]] auto columnCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;
    [[nodiscard]] auto data(const QModelIndex& index, i32 role = Qt::DisplayRole) const -> QVariant override;
    [[nodiscard]] auto flags(const QModelIndex& index) const -> Qt::ItemFlags override;
    auto setData(const QModelIndex& index, const QVariant& value, i32 role = Qt::EditRole) -> bool override;
    [[nodiscard]] auto headerData(i32 section, Qt::Orientation orientation, i32 role = Qt::DisplayRole) const
        -> QVariant override;
    auto setHeaderData(i32 section, Qt::Orientation orientation, const QVariant& value, i32 role = Qt::DisplayRole)
        -> bool override;

    void fill(std::span<QStringView> lines, const QString& filename);
    void clear();

    void setHeaderLabels(QStringList labels);
    void appendColumn(QStringList cells);
    void insertRow(u32 row, QStringList cells);
    auto removeRows(i32 row, i32 count, const QModelIndex& parent = QModelIndex()) -> bool override;

    // Read-only: every write goes through `setData`, `fill` and the row/column methods above
    [[nodiscard]] auto item(u32 row, u8 column) const -> const QString*;
    [[nodiscard]] auto itemFromIndex(const QModelIndex& index) const -> const QString*;

   signals:
    void bookmarkChanged(u32 row);
    void translatedChanged(i8 count);

   private:
    // The cell of a row, which is `colCount` cells long in `texts`
    [[nodiscard]] auto cell(this auto&& self, const isize row, const isize column) -> decltype(auto) {
        return (self.texts[(row * self.colCount) + column]);
    }

    [[nodiscard]] auto isEditable(const isize row, const u8 column) const -> bool {
        return ((editableColumns[row] >> column) & 1) != 0;
    }

    // Changes the length of every row, keeping the cells that fit
    void restride(u8 newCount);

    // Row-major cells of all rows, so that a row costs only the columns the table has, not `MAX_COLUMNS`
    QList<QString> texts;
    // Bit N of a row is set if the column N of the row is editable
    QList<u16> editableColumns;
    QStringList headers;
    u8 colCount = 0;
};
