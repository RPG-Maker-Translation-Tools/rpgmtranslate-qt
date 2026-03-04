#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "SearchResultList.hpp"
#include "Types.hpp"

#include <QDockWidget>

class SearchPanelDock final : public QDockWidget {
    Q_OBJECT

   public:
    enum class Action : u8 {
        GoTo,
        Put,
        Replace
    };

    explicit SearchPanelDock(QWidget* parent = nullptr);
    ~SearchPanelDock() override;

    void showMatches(
        HashMap<FilenameArray, vector<CellMatch>> matches,
        HashMap<u16, QString>& mapSections,
        shared_ptr<ProjectSettings> projectSettings
    );

    void clear();
    void addFile(const QString& file);

    void init(
        QComboBox* fileSelect,
        SearchResultList* searchResultList,
        QPushButton* clearButton
    );

   signals:
    void actionRequested(
        SearchPanelDock::Action action,
        const QString& filename,
        u32 row,
        u8 column,
        span<const TextMatch> matches,
        SearchResultListItem& item
    );

   private:
    inline void clearList();

    HashMap<FilenameArray, vector<CellMatch>> matches;
    shared_ptr<ProjectSettings> projectSettings;

    QComboBox* fileSelect;
    SearchResultList* searchResultList;
};
