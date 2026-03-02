#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QWidget>

class BookmarkMenu final : public QWidget {
    Q_OBJECT

   public:
    explicit BookmarkMenu(QWidget* parent = nullptr);

    void addFile(const QString& file);
    void addBookmark(QStringView description, QStringView file, u32 row);
    void updateBookmark(u32 row, const QString& text);
    void removeBookmark(u32 row);
    void shiftIndices(const QString& file, u32 removedRow);

    void clear();

   signals:
    void bookmarkClicked(QLatin1StringView file, u32 row);

   private:
    QVBoxLayout* layout_;
    QComboBox* fileSelect;
    BookmarkList* bookmarkList;
};