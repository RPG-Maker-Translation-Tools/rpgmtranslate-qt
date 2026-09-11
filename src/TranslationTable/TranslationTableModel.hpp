#pragma once

#include "Aliases.hpp"
#include "Utils.hpp"

#include <QAbstractItemModel>

class TranslationTableModel final : public QAbstractItemModel {
    Q_OBJECT

   public:
    static constexpr i32 MAX_COLUMNS = 16;

    struct Cell {
       private:
        QString* text;

       public:
        Cell(const QString* const text, const bool editable) {
            setText(text);
            setEditable(editable);
        }

        void setEditable(const bool editable) { text = rcast<QString*>(set_tag(text, scast<u8>(editable))); }

        [[nodiscard]] auto getEditable() const -> bool { get_tag(rcast<u64>(text)); }

        void setText(const QString* text) {
            this->text = rcast<QString*>(replace_addr(rcast<u64>(this->text), ccast<QString*>(text)));
        }

        [[nodiscard]] auto getText() const -> QString* { return rcast<QString*>(strip_tag(rcast<u64>(text))); }
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
    QList<pair<array<QString, MAX_COLUMNS>, bitset<MAX_COLUMNS>>> rows;
    QStringList headers;
    u8 colCount = 0;
};
