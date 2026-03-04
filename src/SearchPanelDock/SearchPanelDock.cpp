
#include "SearchPanelDock.hpp"

#include "Constants.hpp"
#include "ProjectSettings.hpp"
#include "SearchResultList.hpp"
#include "Utils.hpp"

#include <QComboBox>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

SearchPanelDock::SearchPanelDock(QWidget* const parent) :
    QDockWidget(parent) {};

SearchPanelDock::~SearchPanelDock() {
    clearList();
}

void SearchPanelDock::showMatches(
    HashMap<FilenameArray, vector<CellMatch>> matches,
    HashMap<u16, QString>& mapSections,
    shared_ptr<ProjectSettings> projectSettings
) {
    clearList();

    this->matches = std::move(matches);
    this->projectSettings = std::move(projectSettings);

    u32 itemsSize = 0;

    for (const auto& matches : views::values(this->matches)) {
        itemsSize += matches.size();
    }

    vector<SearchResultListItem> items;
    items.reserve(itemsSize);

    for (const auto& [filenameArray, cellMatches] : this->matches) {
        QString content;
        QSVList lines;

        const auto filename = QLatin1StringView(filenameArray.data());

        if (filename.startsWith("map"_L1)) {
            const u16 mapNumber = filename.sliced(3).toUInt();
            lines = QStringView(mapSections.find(mapNumber)->second)
                        .split('\n', Qt::SkipEmptyParts);
        } else {
            const QString path = this->projectSettings->translationPath() +
                                 '/' + filename + u".txt";
            auto file = QFile(path);

            if (!file.open(QFile::ReadOnly)) {
                qWarning() << u"Failed to open file %1: %2"_qssv.arg(path).arg(
                    file.errorString()
                );
                continue;
            }

            content = file.readAll();
            lines = QStringView(content).split('\n', Qt::SkipEmptyParts);
        }

        for (const auto& cellMatch : cellMatches) {
            const QString sourceOwned =
                qsvReplace(lines.at(cellMatch.rowIndex()), NEW_LINE, LINE_FEED);

            const QSVList parts =
                lineParts(sourceOwned, cellMatch.rowIndex(), filename);

            if (parts.empty()) {
                continue;
            }

            items.emplace_back(
                qsvReplace(
                    parts.at(cellMatch.colIndex()),
                    '\n',
                    LINE_SEPARATOR
                ),
                tr("File %1 / Row %2 / Column %3 (%4)")
                    .arg(filename)
                    .arg(cellMatch.rowIndex() + 1)
                    .arg(cellMatch.colIndex() + 1)
                    .arg(
                        cellMatch.colIndex() == 0
                            ? tr("Source")
                            : this->projectSettings
                                  ->columns[cellMatch.colIndex()]
                                  .name
                    ),
                filename,
                cellMatch,
                false,
                false
            );
        }
    }

    searchResultList->model()->setItems(std::move(items));
    show();
};

void SearchPanelDock::clear() {
    clearList();
    fileSelect->clear();
    fileSelect->addItem(tr("- Filter by file -"));
}

void SearchPanelDock::addFile(const QString& file) {
    fileSelect->addItem(file);
}

void SearchPanelDock::init(
    QComboBox* const fileSelect,
    SearchResultList* const searchResultList,
    QPushButton* const clearButton
) {
    this->fileSelect = fileSelect;
    this->searchResultList = searchResultList;

    connect(
        fileSelect,
        &QComboBox::currentIndexChanged,
        this,
        [this](const u16 index) -> void {
        if (index == 0) {
            for (const i32 row :
                 range(0, this->searchResultList->model()->rowCount())) {
                this->searchResultList->model()->item(row).hidden = false;
            }
        } else {
            for (const i32 row :
                 range(0, this->searchResultList->model()->rowCount())) {
                this->searchResultList->model()->item(row).hidden =
                    this->searchResultList->model()->item(row).filename !=
                    this->fileSelect->itemText(index);
            }
        }

        this->searchResultList->proxy()->invalidate();
    }
    );

    connect(
        searchResultList,
        &SearchResultList::clicked,
        this,
        [this](const Qt::MouseButton button, const QModelIndex& index) -> void {
        SearchResultListItem& item =
            this->searchResultList->model()->item(index.row());

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
            // TODO: Not really clear
            if (item.cellMatch.colIndex() == 0) {
                auto menu = QMenu(this);

                for (const auto& [idx, column] : views::enumerate(
                         views::drop(projectSettings->columns, 1)
                     )) {
                    const QAction* const action = menu.addAction(
                        QString::number(idx + 1) + u" (" + column.name + ')'
                    );

                    connect(
                        action,
                        &QAction::triggered,
                        this,
                        [this, &item, idx] -> void {
                        emit actionRequested(
                            Action::Put,
                            item.filename,
                            item.cellMatch.rowIndex(),
                            idx + 1,
                            {},
                            item
                        );
                    }
                    );
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
                    return;
                }

                emit actionRequested(
                    Action::Put,
                    item.filename,
                    item.cellMatch.rowIndex(),
                    item.cellMatch.colIndex(),
                    {},
                    item
                );

                item.replaced = true;
            }
        } else if (button == Qt::RightButton) {
            if (item.cellMatch.colIndex() == 0) {
                QMessageBox::information(
                    this,
                    tr("Cannot be replaced"),
                    tr("Source column cannot be replaced.")
                );
            } else {
                if (item.replaced) {
                    QMessageBox::information(
                        this,
                        tr("Already replaced"),
                        tr("This result was already replaced.")
                    );
                    return;
                }

                emit actionRequested(
                    Action::Replace,
                    item.filename,
                    item.cellMatch.rowIndex(),
                    item.cellMatch.colIndex(),
                    span(item.cellMatch.matches, item.cellMatch.matchesCount),
                    item
                );

                item.replaced = true;
            }
        }
    }
    );

    connect(clearButton, &QPushButton::pressed, this, [this] -> void {
        clearList();
    });
}

void SearchPanelDock::clearList() {
    for (const auto& [key, matches] : this->matches) {
        for (const auto cmatch : matches) {
            delete[] cmatch.matches;
        }
    }

    this->matches = {};
    searchResultList->clear();
}