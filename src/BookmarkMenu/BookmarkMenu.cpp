#include "BookmarkMenu.hpp"

#include "BookmarkList.hpp"
#include "TabListModel.hpp"

#include <QComboBox>
#include <QEvent>
#include <QVBoxLayout>

BookmarkMenu::BookmarkMenu(QWidget* const parent) :
    QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
    layout_(new QVBoxLayout(this)),
    fileSelect(new QComboBox(this)),
    bookmarkList(new BookmarkList(this)) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(u"BookmarkMenu { background-color: palette(window) }"_s);

    fileSelect->addItem(tr("- Filter by file -"));

    connect(fileSelect, &QComboBox::currentIndexChanged, this, [this] -> void { applyFilter(); });

    connect(bookmarkList, &BookmarkList::bookmarkClicked, this, [this](const QL1SV file, const u32 row) -> void {
        emit bookmarkClicked(file, row);
    });

    layout_->addWidget(fileSelect);
    layout_->addWidget(bookmarkList);
    adjustSize();
    hide();
}

void BookmarkMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        fileSelect->setItemText(0, tr("- Filter by file -"));
        // The rows are drawn from the bookmarks, with the translated "Row / File / Entry" text
        bookmarkList->viewport()->update();
    }

    QWidget::changeEvent(event);
}

void BookmarkMenu::addBookmark(
    const QStringView description,
    const QStringView file,
    const u32 row,
    const QStringView entry
) {
    shiftIndices(file, row, true);
    bookmarkList->appendRow(description, file, row, entry);

    // A new bookmark of a file the list is not filtered to stays out of it. With no filter nothing is hidden, which
    // is every call while the project loads.
    if (fileSelect->currentIndex() > 0) {
        applyFilter();
    }
}

void BookmarkMenu::applyFilter() const {
    // Index 0 is the "all files" item; none (-1) is the combo box being cleared
    const QString filename = fileSelect->currentIndex() > 0 ? fileSelect->currentText() : QString();

    for (i32 row = 0; row < bookmarkList->rowCount(); row++) {
        const Bookmark& bookmark = bookmarkList->bookmark(row);
        bookmarkList->setRowHidden(row, !filename.isEmpty() && QL1SV(bookmark.filename.data()) != filename);
    }

    bookmarkList->refilter();
}

void BookmarkMenu::setShowEntryNames(const bool show) const {
    bookmarkList->setShowEntryNames(show);
    bookmarkList->viewport()->update();
}

void BookmarkMenu::updateBookmark(const QStringView file, const u32 targetRow, const QString& text) {
    for (i32 row = 0; row < bookmarkList->rowCount(); row++) {
        Bookmark& bookmark = bookmarkList->bookmark(row);

        // Rows are numbered per file: another file can have a bookmark on the same number
        if (bookmark.row == targetRow && QL1SV(bookmark.filename.data()) == file) {
            bookmark.description = text;
        }
    }

    bookmarkList->viewport()->update();
}

void BookmarkMenu::removeBookmark(const QStringView file, const u32 targetRow) {
    // From the end: a removed row moves the ones after it
    for (i32 row = bookmarkList->rowCount() - 1; row >= 0; row--) {
        const Bookmark& bookmark = bookmarkList->bookmark(row);

        if (bookmark.row == targetRow && QL1SV(bookmark.filename.data()) == file) {
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
