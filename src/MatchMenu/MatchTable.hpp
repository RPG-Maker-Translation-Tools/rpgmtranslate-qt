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
    explicit MatchTableDelegate(
        MatchTableModel* model,
        MatchTable* view,
        QObject* parent = nullptr
    );

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& opt,
        const QModelIndex& index
    ) const override;

    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override;

   private:
    constexpr static u8 PAD_X = 4;
    constexpr static u8 PAD_Y = 4;

    MatchTableModel* model;
};

class MatchTableModel final : public QAbstractTableModel {
   public:
    enum Column : u8 {
        Filename,
        Line,
        TermOccurrences,
        TranslationOccurrences,
        SourceMatch,
        TranslationMatch,
        Info,
        ColumnCount
    };

    enum Role : u16 {
        TextRole = Qt::DisplayRole,
        SpansRole = Qt::UserRole + 1
    };

    struct Row {
        vector<Span> sourceSpans;
        vector<Span> translationSpans;

        QString filename;

        QString termOccurrences;
        QString translationOccurrences;

        QString sourceText;
        QString translationText;

        QString info;

        u32 lineNumber;
    };

    explicit MatchTableModel(QObject* const parent = nullptr) :
        QAbstractTableModel(parent) {}

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const
        -> i32 override {
        return i32(rows.size());
    }

    [[nodiscard]] auto columnCount(
        const QModelIndex& parent = QModelIndex()
    ) const -> i32 override;

    [[nodiscard]] auto flags(const QModelIndex& index) const
        -> Qt::ItemFlags override;

    [[nodiscard]] auto data(const QModelIndex& index, i32 role) const
        -> QVariant override;

    [[nodiscard]] auto
    headerData(int section, Qt::Orientation orientation, i32 role) const
        -> QVariant override;

    void appendRow(Row row);
    auto row(u32 row) -> const Row&;
    void clear();

   private:
    vector<Row> rows;
};

class MatchTable final : public QTableView {
   public:
    explicit MatchTable(QWidget* parent = nullptr);

    [[nodiscard]] auto model() const -> MatchTableModel*;

    void appendMatch(
        const QString& filename,
        u32 lineIndex,
        const QString& termSource,
        const QString& termTranslation,
        QStringView source,
        QStringView translation,
        ByteBuffer matches
    );

   private:
    MatchTableModel* model_;
    MatchTableDelegate* delegate;
};