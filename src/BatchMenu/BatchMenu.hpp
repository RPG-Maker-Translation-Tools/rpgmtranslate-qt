#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"
#include "Settings.hpp"
#include "Types.hpp"

QT_BEGIN_NAMESPACE

namespace Ui {
class BatchMenu;
}  // namespace Ui

QT_END_NAMESPACE

class BatchMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit BatchMenu(QWidget* parent = nullptr);
    ~BatchMenu() override;

    void clear();
    void init(const vector<TabListItem>& files, const ProjectSettings* projectSettings);
    void addColumn(const QString& name);
    void renameColumn(u8 index, const QString& name);
    void setEndpoints(const vector<EndpointSettings>& endpoints);
    [[nodiscard]] auto save() -> bool;

   signals:
    void actionRequested(JSScript script, Selected selected, u8 columnIndex, BatchVariant variant);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    [[nodiscard]] inline auto setupUi() -> Ui::BatchMenu*;

    HashSet<QCheckBox*> dragTouched;
    vector<QString> scripts;

    Ui::BatchMenu* const ui;

    const ProjectSettings* projectSettings;
    FileSelectMenu* fileSelectMenu;

    i32 prevScript = 0;

    bool dragging = false;
    bool dragSetValue = false;
};
