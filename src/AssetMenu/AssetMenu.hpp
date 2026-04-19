#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "PersistentMenu.hpp"

class AssetMenu final : public PersistentMenu {
    Q_OBJECT

   public:
    explicit AssetMenu(QWidget* parent = nullptr);

    ~AssetMenu() override {}

    void init(shared_ptr<ProjectSettings> projectSettings);
    void clear();

   private:
    static auto applyFilter(QTreeWidgetItem* item, const QString& lowerFilter)
        -> bool;

    void filterTree(const QString& text);
    void refresh();

    AssetPreviewWidget* const assetPreviewWidget;
    QPushButton* const refreshButton;
    QWidget* const topWidget;
    QHBoxLayout* const topLayout;
    QVBoxLayout* const layout;
    QLineEdit* const searchInput;
    QTreeWidget* const tree;
    QGraphicsScene* const graphicsScene;

    shared_ptr<ProjectSettings> projectSettings;
};
