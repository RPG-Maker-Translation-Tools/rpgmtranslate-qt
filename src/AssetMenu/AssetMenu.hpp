#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"

class AssetMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit AssetMenu(QWidget* parent = nullptr);
    ~AssetMenu() override;

    void init(const ProjectSettings* projectSettings);
    void clear();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    static auto applyFilter(QTreeWidgetItem* item, const QString& lowerFilter) -> bool;

    void filterTree(const QString& text);
    void refresh();

    AssetPreviewWidget* const assetPreviewWidget;
    QPushButton* const refreshButton;
    QLineEdit* const searchInput;
    QTreeWidget* const tree;

    const ProjectSettings* projectSettings = nullptr;
};
