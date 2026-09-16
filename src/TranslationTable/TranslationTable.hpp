#pragma once

#include "FWD.hpp"
#include "ProjectSettings.hpp"
#include "Settings.hpp"
#include "TranslationInput.hpp"

#include <QTableView>

enum RowFlags : u8 {
    CommentFlag = 1 << 0,
    BookmarkFlag = 1 << 1,
    TranslatedFlag = 1 << 2,
};

using RemovedRowInfo = PackedBits<Field<"row", u32, 24>, Field<"flags", RowFlags, 8>>;

class TranslationTable final : public QTableView {
    Q_OBJECT

   public:
    enum class MultilineAction : u8 {
        Cut,
        Copy,
        Paste
    };

    explicit TranslationTable(QWidget* parent = nullptr);

    void init(const Settings* settings, const ProjectSettings* projectSettings) const;

    void insertTranslation(const QString& translation);

    [[nodiscard]] constexpr auto model() const -> TranslationTableModel* { return model_; }

    [[nodiscard]] constexpr auto header() const -> TranslationTableHeader* { return header_; }

    void fill(const span<QStringView>& lines, const vector<ColumnInfo>& columns, const QString& filename);

    auto cut() -> u32;
    auto copy() -> u32;
    auto paste() -> u32;

    auto activeInput() -> TranslationInput*;

   signals:
    void bookmarked(u32 row);
    void translatedChanged(i8 count);
    void bookmarkChanged(u32 row);

    void columnAdded();
    void columnRenamed(i32 index, const QString& name);
    void columnResized(i32 index, u16 width);
    void rowRemoved(RemovedRowInfo info);

    void inputFocused();
    void textChanged();

    void multilineAction(MultilineAction action, u32 count);

   protected:
    void keyPressEvent(QKeyEvent* event) override;

   private:
    HashMap<i32, i8> selectedColumnPerRow;

    TranslationTableModel* model_;
    TranslationTableHeader* header_;
    TranslationTableDelegate* delegate;

    const ProjectSettings* projectSettings;
};
