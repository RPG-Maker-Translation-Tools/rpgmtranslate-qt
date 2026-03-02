#pragma once

#include "Enums.hpp"
#include "FWD.hpp"
#include "ProjectSettings.hpp"

#include <QTableView>

enum RowFlags : u8 {
    CommentFlag = 1 << 0,
    BookmarkFlag = 1 << 1,
    TranslatedFlag = 1 << 2,
};

struct RemovedRowInfo {
    u32 bits;

    constexpr explicit RemovedRowInfo() = default;

    constexpr explicit RemovedRowInfo(const u32 row, const RowFlags flags) {
        setRow(row);
        setFlags(flags);
    };

    static constexpr u32 ROW_MASK = 0xFF00'0000U;
    static constexpr u32 FLAGS_SHIFT = 24;
    static constexpr u32 FLAGS_MASK = 0x00FF'FFFFU;

    [[nodiscard]] constexpr auto row() const -> u32 {
        return bits & FLAGS_MASK;
    }

    [[nodiscard]] constexpr auto flags() const -> RowFlags {
        return RowFlags(bits >> FLAGS_SHIFT);
    }

    constexpr void setRow(const u32 row) {
        bits = (bits & ROW_MASK) | (row & FLAGS_MASK);
    }

    constexpr void setFlags(const RowFlags flags) {
        bits = (bits & FLAGS_MASK) | (u32(flags) << FLAGS_SHIFT);
    }
};

class TranslationTable final : public QTableView {
    Q_OBJECT

   public:
    enum class MultilineAction : u8 {
        Cut,
        Copy,
        Paste
    };

    explicit TranslationTable(QWidget* parent = nullptr);

    void init(
        const Algorithm* algorithm,
        const u16* hint,
        const bool* enabled
    ) const;

    void initializeDictionary() const;
    void insertTranslation(const QString& translation);

    [[nodiscard]] constexpr auto model() const -> TranslationTableModel* {
        return model_;
    }

    [[nodiscard]] constexpr auto header() const -> TranslationTableHeader* {
        return header_;
    }

    void fill(
        const span<QStringView>& lines,
        const vector<ColumnInfo>& columns,
        const QString& filename
    );

    auto cut() -> u32;
    auto copy() -> u32;
    auto paste() -> u32;

   signals:
    void bookmarked(u32 row);
    void translatedChanged(i8 count);
    void bookmarkChanged(u32 row);

    void columnAdded();
    void columnRenamed(u8 index, const QString& name);
    void columnResized(u8 index, u16 width);
    void rowRemoved(RemovedRowInfo info);

    void inputFocused();
    void textChanged(const QString& text);

    void multilineAction(MultilineAction action, u32 count);

   protected:
    void keyPressEvent(QKeyEvent* event) override;

   private:
    inline void addRow(QStringView source, const QSVList& translations);
    inline void addCommentRow(QStringView comment);

    TranslationTableModel* model_;
    TranslationTableHeader* header_;
    TranslationTableDelegate* delegate;

    i8 selectedColumn = -1;
};