#pragma once

#include "Aliases.hpp"

#include <QAbstractItemModel>

class TranslationTableModel final : public QAbstractItemModel {
    Q_OBJECT

   public:
    static constexpr i32 MAX_COLUMNS = 16;

    struct Cell {
        static constexpr usize EDITABLE_BIT = 1;
        static constexpr usize PTR_MASK = ~EDITABLE_BIT;

        usize bits;

        Cell(QString* const ptr, const bool editable) { set(ptr, editable); }

        void set(QString* const ptr, const bool editable) {
            const auto val = rcast<usize>(ptr);
            bits = val | (editable ? EDITABLE_BIT : 0);
        }

        [[nodiscard]] auto text() const -> QString* { return rcast<QString*>(bits & PTR_MASK); }

        [[nodiscard]] auto editable() const -> bool { return (bits & EDITABLE_BIT) != 0; }

        void setEditable(const bool editable) { bits = (bits & PTR_MASK) | (editable ? EDITABLE_BIT : 0); }
    };

    explicit TranslationTableModel(QObject* parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;
    [[nodiscard]] auto columnCount(const QModelIndex& parent = QModelIndex()) const -> i32 override;
    [[nodiscard]] auto index(i32 row, i32 col, const QModelIndex& parent = QModelIndex()) const -> QModelIndex override;
    [[nodiscard]] auto parent(const QModelIndex& /* index */ = QModelIndex()) const -> QModelIndex override;
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

    [[nodiscard]] auto item(u32 row, u8 column) -> Cell;
    [[nodiscard]] auto itemFromIndex(const QModelIndex& index) -> Cell;

   signals:
    void bookmarkChanged(u32 row);
    void translatedChanged(i8 count);

   private:
    QList<array<QString, MAX_COLUMNS>> rows;
    QList<bitset<MAX_COLUMNS>> editableFlags;
    QStringList headers;
    u8 colCount = 0;
};
