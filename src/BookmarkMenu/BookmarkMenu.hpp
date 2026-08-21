#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QWidget>

class BookmarkMenu final : public QWidget {
    Q_OBJECT

   public:
    explicit BookmarkMenu(QWidget* parent = nullptr);

    void init(const vector<TabListItem>& files);
    void addBookmark(QStringView description, QStringView file, u32 row);
    void updateBookmark(u32 row, const QString& text);
    void removeBookmark(u32 row);
    void shiftIndices(QStringView file, u32 row, bool rowAdded = false);

    void clear();

   signals:
    void bookmarkClicked(QL1SV file, u32 row);

   private:
    QVBoxLayout* layout_;
    QComboBox* fileSelect;
    BookmarkList* bookmarkList;
};
