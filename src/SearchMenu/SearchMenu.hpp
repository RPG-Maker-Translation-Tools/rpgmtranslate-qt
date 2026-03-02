#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
    class SearchMenu;
}  // namespace Ui

QT_END_NAMESPACE

class SearchMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    enum class Action : u8 {
        Search,
        Replace,
        Put,
    };

    explicit SearchMenu(QWidget* parent = nullptr);
    ~SearchMenu() override;

    void clear();
    void addFile(const QString& filename);
    void addColumn(const QString& name);
    void renameColumn(u8 index, const QString& name);
    [[nodiscard]] auto replaceText() const -> QString;

   signals:
    void actionRequested(
        Selected selected,
        Action action,
        const QString& searchText,
        const QString& replaceText,
        SearchLocation searchLocation,
        i8 searchColumn,
        SearchFlags searchFlags
    );

   protected:
    void changeEvent(QEvent* event) override;

   private:
    inline auto setupUi() -> Ui::SearchMenu*;

    inline void requestAction(bool replace);

    Ui::SearchMenu* const ui;

    FileSelectMenu* fileSelectMenu;
};