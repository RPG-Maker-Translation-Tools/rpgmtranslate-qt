#include "AssetMenu.hpp"

#include "AssetPreviewWidget.hpp"
#include "ProjectSettings.hpp"
#include "rpgmtranslate_rs.h"

#include <QDirListing>
#include <QGraphicsPixmapItem>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>

AssetMenu::AssetMenu(QWidget* const parent) :
    PersistentMenu(parent),
    assetPreviewWidget(new AssetPreviewWidget()),
    refreshButton(new QPushButton(QPushButton(QIcon(u":/icons/refresh.svg"_s), QString(), this))),
    topWidget(new QWidget(this)),
    topLayout(new QHBoxLayout(topWidget)),
    layout(new QVBoxLayout(this)),
    searchInput(new QLineEdit(this)),
    tree(new QTreeWidget(this)),
    graphicsScene(new QGraphicsScene(this)) {
    tree->setUniformRowHeights(true);
    tree->setHeaderHidden(true);
    tree->setSelectionMode(QTreeWidget::SingleSelection);
    tree->setEditTriggers(QTreeWidget::NoEditTriggers);
    tree->setDragDropMode(QTreeWidget::NoDragDrop);
    tree->setAcceptDrops(false);

    searchInput->setPlaceholderText(tr("Search file..."));

    topLayout->addWidget(searchInput);
    topLayout->addWidget(refreshButton);

    layout->addWidget(topWidget);
    layout->addWidget(tree);

    connect(
        tree,
        &QTreeWidget::itemClicked,
        this,
        [this](const QTreeWidgetItem* const item, const i32 /* column */) -> void {
        const QString path = item->data(0, Qt::UserRole).toString();

        if (path.isEmpty()) {
            return;
        }

        assetPreviewWidget->showAsset(path);
    }
    );

    connect(refreshButton, &QPushButton::pressed, this, [this] -> void { refresh(); });

    connect(searchInput, &QLineEdit::textChanged, this, &AssetMenu::filterTree);
}

void AssetMenu::init(shared_ptr<ProjectSettings> projectSettings) {
    this->projectSettings = std::move(projectSettings);
    refresh();
}

void AssetMenu::clear() {
    tree->clear();
    graphicsScene->clear();
}

auto AssetMenu::applyFilter(QTreeWidgetItem* const item, const QString& lowerFilter) -> bool {
    const i32 childCount = item->childCount();

    if (childCount == 0) {
        const bool match = lowerFilter.isEmpty() || item->text(0).toLower().contains(lowerFilter);
        item->setHidden(!match);
        return match;
    }

    bool anyVisible = false;
    for (const auto idx : range(0, childCount)) {
        if (applyFilter(item->child(idx), lowerFilter)) {
            anyVisible = true;
        }
    }

    item->setHidden(!anyVisible);
    if (!lowerFilter.isEmpty()) {
        item->setExpanded(anyVisible);
    }

    return anyVisible;
}

void AssetMenu::filterTree(const QString& text) {
    const QString lower = text.toLower();
    const i32 categoryCount = tree->topLevelItemCount();
    for (const auto idx : range(0, categoryCount)) {
        applyFilter(tree->topLevelItem(idx), lower);
    }
}

void AssetMenu::refresh() {
    clear();
    searchInput->clear();

    if (!projectSettings) {
        return;
    }

    auto* const audioItem = new QTreeWidgetItem(tree, { tr("Audio") });
    auto* const dataItem = new QTreeWidgetItem(tree, { tr("Data") });
    auto* const imagesItem = new QTreeWidgetItem(tree, { tr("Images") });
    auto* const iconsItem = new QTreeWidgetItem(tree, { tr("Icons") });
    auto* const fontsItem = new QTreeWidgetItem(tree, { tr("Fonts") });
    auto* const moviesItem = new QTreeWidgetItem(tree, { tr("Movies") });
    auto* const jsItem = new QTreeWidgetItem(tree, { tr("JS") });

    const auto populate =
        [](QTreeWidgetItem* const parent, const QString& dirPath, const QStringList& filters) -> void {
        if (!QFile::exists(dirPath)) {
            qWarning().noquote() << u"Path %1 does not exist."_qsv.arg(dirPath);
            return;
        }

        const auto listing =
            QDirListing(dirPath, filters, QDirListing::IteratorFlag::Recursive | QDirListing::IteratorFlag::FilesOnly);

        for (const auto& entry : listing) {
            auto* const item = new QTreeWidgetItem(parent, { entry.fileName() });
            item->setData(0, Qt::UserRole, entry.filePath());
        }
    };

    const QString& base = projectSettings->projectPath;

    if (projectSettings->engineType == EngineType::New) {
        const bool wwwExists = QFile::exists(base + u"/www");

        populate(
            audioItem,
            base + (wwwExists ? u"/www/audio" : u"/audio"),
            { u"*.ogg_"_s, u"*.m4a_"_s, u"*.rpgmvo"_s, u"*.rpgmvm"_s, u"*.ogg"_s, u"*.m4a"_s }
        );
        populate(dataItem, base + (wwwExists ? u"/www/data" : u"/data"), { u"*.json"_s });
        populate(
            imagesItem,
            base + (wwwExists ? u"/www/img" : u"/img"),
            { u"*.png_"_s, u"*.rpgmvp"_s, u"*.png"_s, u"*.jpg"_s }
        );
        populate(
            iconsItem,
            base + (wwwExists ? u"/www/icon" : u"/icon"),
            { u"*.png_"_s, u"*.rpgmvp"_s, u"*.png"_s, u"*.jpg"_s }
        );
        populate(moviesItem, base + (wwwExists ? u"/www/movies" : u"/movies"), { u"*.webm"_s, u"*.mp4"_s });
        populate(fontsItem, base + (wwwExists ? u"/www/fonts" : u"/fonts"), { u"*.ttf"_s, u"*.otf"_s });
        populate(jsItem, base + (wwwExists ? u"/www/js" : u"/js"), { u"*.js"_s });
    } else {
        populate(audioItem, base + u"/Audio", { u"*.ogg"_s, u"*.m4a"_s });
        populate(dataItem, base + u"/Data", { u"*.rxdata"_s, u"*.rvdata"_s, u"*.rvdata2"_s });
        populate(imagesItem, base + u"/Graphics", { u"*.png"_s, u"*.jpg"_s });
        populate(fontsItem, base + u"/Fonts", { u"*.ttf"_s, u"*.otf"_s });
        populate(moviesItem, base + u"/Movies", { u"*.webm"_s, u"*.mp4"_s });
    }
}
