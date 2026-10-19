#include "SearchPanelDock.hpp"

#include "Constants.hpp"
#include "Notice.hpp"
#include "ProjectSettings.hpp"
#include "TabListModel.hpp"
#include "Utils.hpp"

#include <QComboBox>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

SearchPanelDock::SearchPanelDock(QWidget* const parent) : QDockWidget(parent) {};

SearchPanelDock::~SearchPanelDock() = default;

auto SearchPanelDock::generation() const -> u32 {
    return searchResultList->model()->generation();
}

void SearchPanelDock::itemChanged(const SearchResultListItem& item) const {
    // The height of the item can differ now
    searchResultList->delegate()->invalidateCache();
    searchResultList->model()->itemChanged(item);
}

void SearchPanelDock::showMatches(CellMatches matches, const ProjectSettings* const projectSettings) {
    clearList();

    // The old filter would hide nothing now, while the box still shows it
    fileSelect->setCurrentIndex(0);

    this->matches = std::move(matches);
    this->projectSettings = projectSettings;

    // The files in the order of the tabs, not in that of the hash map
    struct FileResults {
        QL1SV filename;
        FileMatches* fileMatches;
        i32 order;
    };

    vector<FileResults> files;
    files.reserve(this->matches.size());

    u32 itemsSize = 0;

    for (auto& [filenameArray, fileMatches] : this->matches) {
        const auto filename = QL1SV(filenameArray.data());

        files.push_back({ filename, &fileMatches, fileSelect->findText(QString::fromLatin1(filename)) });
        itemsSize += fileMatches.cells.size();
    }

    ranges::sort(files, {}, &FileResults::order);

    // Nothing is made for the items but the links: the texts are made when an item is shown
    vector<SearchResultListItem> items;
    items.reserve(itemsSize);

    for (const auto& [filename, fileMatches, order] : files) {
        for (auto& cellMatch : fileMatches->cells) {
            items.emplace_back(*fileMatches, cellMatch, filename, false, false);
        }
    }

    searchResultList->model()->setItems(std::move(items));
    show();
};

auto SearchPanelDock::subtitle(const SearchResultListItem& item) const -> QString {
    const CellMatch& cellMatch = item.cellMatch;

    QString subtitle = tr("File %1 / Row %2 / Column %3 (%4)")
                           .arg(
                               svtostr(item.filename),
                               qsv(qitos(cellMatch.rowIndex() + 1)),
                               qsv(qitos(cellMatch.colIndex() + 1)),
                               cellMatch.colIndex() == 0 ? tr("Source")
                               : cellMatch.colIndex() < scast<i32>(projectSettings->columns.size())
                                   ? projectSettings->columns[cellMatch.colIndex()].name
                                   : QString(qsv(qitos(cellMatch.colIndex() + 1)))
                           );

    if (const QStringView entry = item.file.entry(cellMatch); !entry.isEmpty()) {
        subtitle += tr(" / Entry %1").arg(entry);
    }

    return subtitle;
}

void SearchPanelDock::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange && fileSelect != nullptr && fileSelect->count() > 0) {
        fileSelect->setItemText(0, tr("- Filter by file -"));
    }

    QDockWidget::changeEvent(event);
}

void SearchPanelDock::clear() {
    clearList();
    fileSelect->clear();
    fileSelect->addItem(tr("- Filter by file -"));
}

void SearchPanelDock::init(
    QComboBox* const fileSelect,
    SearchResultList* const resultList,
    QPushButton* const clearButton
) {
    this->fileSelect = fileSelect;
    this->searchResultList = resultList;
    resultList->model()->setPanel(this);

    connect(fileSelect, &QComboBox::currentIndexChanged, this, [this](const i32 index) -> void {
        SearchResultListModel* const model = this->searchResultList->model();
        const QString filename = index > 0 ? this->fileSelect->itemText(index) : QString();

        for (i32 row = 0; row < model->rowCount(); row++) {
            SearchResultListItem& item = model->item(row);
            item.hidden = index > 0 && item.filename != filename;
        }

        this->searchResultList->proxy()->invalidateFilter();
    });

    connect(
        resultList,
        &SearchResultList::clicked,
        this,
        [this](const Qt::MouseButton button, const QModelIndex& index) -> void {
        const QModelIndex sourceIndex = this->searchResultList->proxy()->mapToSource(index);

        if (!sourceIndex.isValid()) {
            return;
        }

        SearchResultListItem& item = this->searchResultList->model()->item(sourceIndex.row());

        if (button == Qt::LeftButton) {
            emit actionRequested(
                Action::GoTo,
                item.filename,
                item.cellMatch.rowIndex(),
                item.cellMatch.colIndex(),
                {},
                item
            );
        } else if (button == Qt::MiddleButton) {
            // The menu and the dialog below run their own event loops, in which a finished task can refill the list and
            // free `item`
            const u32 shownGeneration = generation();

            if (item.cellMatch.colIndex() == 0) {
                auto menu = QMenu(this);

                for (const auto& [idx, column] : views::enumerate(views::drop(projectSettings->columns, 1))) {
                    const QAction* const action = menu.addAction(qsv(qitos(idx + 1)) % u" (" % column.name % u')');

                    connect(action, &QAction::triggered, this, [this, &item, idx, shownGeneration] -> void {
                        if (generation() != shownGeneration) {
                            return;
                        }

                        emit actionRequested(
                            Action::Put,
                            item.filename,
                            item.cellMatch.rowIndex(),
                            scast<u8>(idx + 1),
                            {},
                            item
                        );
                    });
                }

                menu.exec(QCursor::pos());
            } else {
                const auto selected = QMessageBox::question(
                    this,
                    tr("Confirmation"),
                    tr(
                        "Do you really want to put the text from the replace input in place of this text, effectively replacing it?"
                    )
                );

                if (selected != QMessageBox::Yes) {
                    qInfo().noquote() << u"Put-in-place replacement was rejected by user."_qsv;
                    return;
                }

                if (generation() != shownGeneration) {
                    return;
                }

                // Set before the request: the handler resets it if the task cannot start
                item.replaced = true;

                emit actionRequested(
                    Action::Put,
                    item.filename,
                    item.cellMatch.rowIndex(),
                    item.cellMatch.colIndex(),
                    {},
                    item
                );
            }
        } else if (button == Qt::RightButton) {
            if (item.cellMatch.colIndex() == 0) {
                present(this, NOTICE("Source column cannot be replaced.", Info, Status));
            } else {
                if (item.replaced) {
                    present(this, NOTICE("This result was already replaced.", Info, Status));
                    return;
                }

                // Set before the request: the handler resets it if the task cannot start
                item.replaced = true;

                emit actionRequested(
                    Action::Replace,
                    item.filename,
                    item.cellMatch.rowIndex(),
                    item.cellMatch.colIndex(),
                    item.file.matches(item.cellMatch),
                    item
                );
            }
        }
    }
    );

    connect(clearButton, &QPushButton::clicked, this, [this] -> void { clearList(); });
}

void SearchPanelDock::clearList() {
    searchResultList->clear();
    this->matches = {};
}

void SearchPanelDock::setFiles(const vector<TabListItem>& files) {
    for (const auto& file : files) {
        fileSelect->addItem(file.name);
    }
}
