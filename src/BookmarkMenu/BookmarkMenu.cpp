#include "BookmarkMenu.hpp"

#include "BookmarkList.hpp"
#include "TabListModel.hpp"
#include "Utils.hpp"

#include <QComboBox>
#include <QVBoxLayout>

BookmarkMenu::BookmarkMenu(QWidget* const parent) :
    QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
    layout_(new QVBoxLayout(this)),
    fileSelect(new QComboBox(this)),
    bookmarkList(new BookmarkList(this)) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(u"BookmarkMenu { background-color: palette(window) }"_s);

    fileSelect->addItem(tr("- Filter by file -"));

    connect(fileSelect, &QComboBox::currentIndexChanged, this, [this](const u32 index) -> void {
        if (index == 0) {
            for (i32 row = 0; row < bookmarkList->rowCount(); row++) {
                bookmarkList->setRowHidden(row, false);
            }
        } else {
            const QString filename = fileSelect->currentText();

            for (i32 row = 0; row < bookmarkList->rowCount(); row++) {
                const Bookmark& bookmark = bookmarkList->bookmark(row);
                bookmarkList->setRowHidden(row, QL1SV(bookmark.filename.data()) != filename);
            }
        }

        bookmarkList->refilter();
    });

    connect(bookmarkList, &BookmarkList::bookmarkClicked, this, [this](const QL1SV file, const u32 row) -> void {
        emit bookmarkClicked(file, row);
    });

    layout_->addWidget(fileSelect);
    layout_->addWidget(bookmarkList);
    adjustSize();
    hide();
}

void BookmarkMenu::addBookmark(const QStringView description, const QStringView file, const u32 row) {
    shiftIndices(file, row, true);
    bookmarkList->appendRow(description, file, row);
}

void BookmarkMenu::updateBookmark(const u32 targetRow, const QString& text) {
    for (i32 row = 0; row < bookmarkList->rowCount(); row++) {
        Bookmark& bookmark = bookmarkList->bookmark(row);

        if (bookmark.row == targetRow) {
            bookmark.description = text;
        }
    }
};

void BookmarkMenu::removeBookmark(const u32 targetRow) {
    for (i32 row = 0; row < bookmarkList->rowCount(); row++) {
        const Bookmark& bookmark = bookmarkList->bookmark(row);

        if (bookmark.row == targetRow) {
            bookmarkList->removeRow(row);
        }
    }
}

void BookmarkMenu::shiftIndices(const QStringView file, const u32 row, const bool rowAdded) {
    if (rowAdded) {
        for (i32 idx = 0; idx < bookmarkList->rowCount(); idx++) {
            Bookmark& bookmark = bookmarkList->bookmark(idx);

            if (QL1SV(bookmark.filename.data()) == file && bookmark.row >= row) {
                bookmark.row += 1;
            }
        }
    } else {
        for (i32 idx = 0; idx < bookmarkList->rowCount(); idx++) {
            Bookmark& bookmark = bookmarkList->bookmark(idx);

            if (QL1SV(bookmark.filename.data()) == file && bookmark.row > row) {
                bookmark.row -= 1;
            }
        }
    }
};

void BookmarkMenu::clear() {
    bookmarkList->clear();
    fileSelect->clear();
    fileSelect->addItem(tr("- Filter by file -"));
}

void BookmarkMenu::init(const vector<TabListItem>& files) {
    for (const auto& file : files) {
        fileSelect->addItem(file.name);
    }
}