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

    void showMatches(CellMatches matches, const ProjectSettings* projectSettings);

    void clear();
    void clearList();
    void setFiles(const vector<TabListItem>& files);

    // Changes whenever the list is cleared or refilled: the items handed out before are gone then, even if new ones
    // lie at the same addresses.
    [[nodiscard]] auto generation() const -> u32;
    void itemChanged(const SearchResultListItem& item) const;

    [[nodiscard]] auto subtitle(const SearchResultListItem& item) const -> QString;

    void init(QComboBox* fileSelect, SearchResultList* searchResultList, QPushButton* clearButton);

   signals:
    void actionRequested(
        SearchPanelDock::Action action,
        const QString& filename,
        u32 row,
        u8 column,
        span<const TextMatch> matches,
        SearchResultListItem& item
    );

   protected:
    void changeEvent(QEvent* event) override;

   private:
    CellMatches matches;
    const ProjectSettings* projectSettings = nullptr;

    QComboBox* fileSelect = nullptr;
    SearchResultList* searchResultList = nullptr;
};
