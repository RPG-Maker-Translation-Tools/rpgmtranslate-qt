#include "AssetMenu.hpp"

#include "AssetKind.hpp"
#include "AssetPreviewWidget.hpp"
#include "Constants.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"

#include <QDir>
#include <QDirListing>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>

namespace {
// Directories at the root of an RM2K project that hold the tool's own copies of the game's files
constexpr array<QStringView, 2> RM2K_EXCLUDED_DIRECTORIES = { PROGRAM_DATA_DIRECTORY, TRANSLATION_DIRECTORY };
}  // namespace

AssetMenu::AssetMenu(QWidget* const parent) :
    PersistentMenu(parent),
    assetPreviewWidget(new AssetPreviewWidget()),
    refreshButton(new QPushButton(QIcon(u":/icons/refresh.svg"_s), QString(), this)),
    searchInput(new QLineEdit(this)),
    tree(new QTreeWidget(this)) {
    auto* const topWidget = new QWidget(this);
    auto* const topLayout = new QHBoxLayout(topWidget);
    auto* const layout = new QVBoxLayout(this);

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

    connect(refreshButton, &QPushButton::clicked, this, [this] -> void { refresh(); });

    connect(searchInput, &QLineEdit::textChanged, this, &AssetMenu::filterTree);
}

AssetMenu::~AssetMenu() {
    delete assetPreviewWidget;
}

void AssetMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        searchInput->setPlaceholderText(tr("Search file..."));

        // The top-level items are the categories in the order `refresh()` makes them (JS, the last, has no translation)
        const QStringList categories = { tr("Audio"), tr("Data"),  tr("Images"), tr("Icons"),
                                         tr("Fonts"), tr("Saves"), tr("Movies") };

        for (i32 idx = 0; idx < min<i32>(categories.size(), tree->topLevelItemCount()); idx++) {
            tree->topLevelItem(idx)->setText(0, categories[idx]);
        }
    }

    PersistentMenu::changeEvent(event);
}

void AssetMenu::init(const ProjectSettings* const projectSettings) {
    this->projectSettings = projectSettings;
    refresh();
}

// The project is closed: the window can't keep showing its asset, and `projectSettings` is about to be freed
void AssetMenu::clear() {
    projectSettings = nullptr;

    assetPreviewWidget->hide();
    assetPreviewWidget->clear();

    tree->clear();
    searchInput->clear();
}

auto AssetMenu::applyFilter(QTreeWidgetItem* const item, const QString& lowerFilter) -> bool {
    const i32 childCount = item->childCount();

    if (childCount == 0) {
        const bool match = lowerFilter.isEmpty() || item->text(0).toLower().contains(lowerFilter);
        item->setHidden(!match);
        return match;
    }

    bool anyVisible = false;
    for (i32 idx = 0; idx < childCount; idx++) {
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
    for (i32 idx = 0; idx < categoryCount; idx++) {
        applyFilter(tree->topLevelItem(idx), lower);
    }
}

void AssetMenu::refresh() {
    tree->clear();
    searchInput->clear();

    if (projectSettings == nullptr) {
        return;
    }

    auto* const audioItem = new QTreeWidgetItem(tree, { tr("Audio") });
    auto* const dataItem = new QTreeWidgetItem(tree, { tr("Data") });
    auto* const imagesItem = new QTreeWidgetItem(tree, { tr("Images") });
    auto* const iconsItem = new QTreeWidgetItem(tree, { tr("Icons") });
    auto* const fontsItem = new QTreeWidgetItem(tree, { tr("Fonts") });
    auto* const savesItem = new QTreeWidgetItem(tree, { tr("Saves") });
    auto* const moviesItem = new QTreeWidgetItem(tree, { tr("Movies") });
    auto* const jsItem = new QTreeWidgetItem(tree, { u"JS"_s });

    // Where the files of a kind go, and which names are looked for. With several targets a file goes to the one of its
    // extension's kind, so the directory is walked once for all of them.
    struct Target {
        QTreeWidgetItem* parent;
        QStringList filters;
        AssetKind kind;
    };

    // `excludedDirs` are top-level subdirectories of `dirPath` ("/name") that are not walked at all.
    const auto populate = [](const span<const Target> targets,
                             const QString& dirPath,
                             const bool recursive = true,
                             const span<const QStringView> excludedDirs = {}) -> void {
        if (!QFile::exists(dirPath)) {
            qWarning().noquote() << u"Path %1 does not exist."_qsv.arg(dirPath);
            return;
        }

        QStringList filters;

        for (const Target& target : targets) {
            filters.append(target.filters);
        }

        struct Entry {
            QString path;
            QString name;
            // Subdirectory relative to `dirPath`, '/'-separated; empty for files directly inside it.
            QString dir;
            // Index into `targets`
            usize target;
        };

        QList<Entry> list;
        list.reserve(2048);

        const isize dirStart = dirPath.size() + 1;

        const auto targetOf = [targets](const QString& name) -> usize {
            if (targets.size() == 1) {
                return 0;
            }

            const AssetKind kind = assetKind(name.sliced(name.lastIndexOf(u'.') + 1).toLower());
            const auto found = ranges::find(targets, kind, &Target::kind);

            return found == targets.end() ? 0 : scast<usize>(found - targets.begin());
        };

        const auto collect = [&](const QString& root, const QDirListing::IteratorFlags flags) -> void {
            for (const auto entry : QDirListing(root, filters, flags)) {
                QString path = entry.filePath();
                const isize nameStart = path.lastIndexOf(u'/') + 1;
                QString dir = nameStart > dirStart ? path.sliced(dirStart, nameStart - 1 - dirStart) : QString();
                QString name = entry.fileName();
                const usize target = targetOf(name);

                list.append(
                    { .path = std::move(path), .name = std::move(name), .dir = std::move(dir), .target = target }
                );
            }
        };

        if (excludedDirs.empty()) {
            collect(
                dirPath,
                recursive ? QDirListing::IteratorFlag::Recursive | QDirListing::IteratorFlag::FilesOnly
                          : QDirListing::IteratorFlag::FilesOnly
            );
        } else {
            // QDirListing can't prune a subtree, so the root's files and each allowed subdirectory are listed apart.
            collect(dirPath, QDirListing::IteratorFlag::FilesOnly);

            for (const auto subdir : QDirListing(dirPath, QDirListing::IteratorFlag::DirsOnly)) {
                const QString subdirPath = subdir.filePath();

                if (!ranges::contains(excludedDirs, QStringView(subdirPath).sliced(dirPath.size()))) {
                    collect(subdirPath, QDirListing::IteratorFlag::Recursive | QDirListing::IteratorFlag::FilesOnly);
                }
            }
        }

        const auto extractMapNumber = [](const QStringView name) -> u16 {
            if (!name.startsWith(u"Map"_qsv)) {
                return UINT16_MAX;
            }

            const isize start = 3;
            isize end = start;

            while (end < name.size() && name.at(end).isDigit()) {
                end++;
            }

            return stoa<u16>(name.sliced(start, end - start)).value_or(0);
        };

        ranges::sort(list, [&extractMapNumber](const Entry& lhs, const Entry& rhs) -> bool {
            if (lhs.target != rhs.target) {
                return lhs.target < rhs.target;
            }

            if (lhs.dir != rhs.dir) {
                return lhs.dir < rhs.dir;
            }

            const auto lhsNum = extractMapNumber(lhs.name);
            const auto rhsNum = extractMapNumber(rhs.name);

            if (lhsNum != UINT16_MAX && rhsNum != UINT16_MAX) {
                return lhsNum < rhsNum;
            }

            return lhs.name < rhs.name;
        });

        // Every subdirectory becomes a branch of the tree, created on first use.
        HashMap<QString, QTreeWidgetItem*> branches;

        const auto branchFor = [&branches](this auto&& self, const QString& dir) -> QTreeWidgetItem* {
            if (const auto found = branches.find(dir); found != branches.end()) {
                return found->second;
            }

            const isize split = dir.lastIndexOf(u'/');
            auto* const item =
                new QTreeWidgetItem(self(split == -1 ? QString() : dir.first(split)), { dir.sliced(split + 1) });

            branches.emplace(dir, item);
            return item;
        };

        // The entries of one target are together, so its branches are made when the target starts
        usize current = UINT64_MAX;

        for (const auto& entry : list) {
            if (entry.target != current) {
                current = entry.target;
                branches.clear();
                branches.emplace(QString(), targets[current].parent);
            }

            auto* const item = new QTreeWidgetItem(branchFor(entry.dir), { entry.name });
            item->setData(0, Qt::UserRole, entry.path);
        }
    };

    const auto populateOne =
        [&populate](QTreeWidgetItem* const parent, const QString& dirPath, QStringList filters) -> void {
        const Target target = { .parent = parent, .filters = std::move(filters), .kind = AssetKind::Unknown };
        populate(span(&target, 1), dirPath);
    };
    const QString& base = projectSettings->projectPath;

    // Engines differ only in where assets live; which extensions count as an asset is the same everywhere.
    if (projectSettings->engineType == EngineType::MVMZ) {
        const QString root = base + (QFile::exists(base + u"/www") ? u"/www" : u"");

        const QStringList imageFilters = assetFilters(AssetKind::Image);

        populateOne(audioItem, root + u"/audio", assetFilters(AssetKind::Audio));
        populateOne(dataItem, root + u"/data", assetFilters(AssetKind::Data));
        populateOne(imagesItem, root + u"/img", imageFilters);
        populateOne(iconsItem, root + u"/icon", imageFilters);
        populateOne(moviesItem, root + u"/movies", assetFilters(AssetKind::Video));
        populateOne(fontsItem, root + u"/fonts", assetFilters(AssetKind::Font));
        populateOne(jsItem, root + u"/js", assetFilters(AssetKind::Script));
    } else if (projectSettings->engineType == EngineType::RM2K) {
        // No dedicated directories: assets are anywhere under the root, apart from the tool's own directories.
        const array<Target, 4> targets = {
            Target{ .parent = dataItem, .filters = assetFilters(AssetKind::Data), .kind = AssetKind::Data },
            { .parent = audioItem, .filters = assetFilters(AssetKind::Audio), .kind = AssetKind::Audio },
            { .parent = imagesItem, .filters = assetFilters(AssetKind::Image), .kind = AssetKind::Image },
            { .parent = moviesItem, .filters = assetFilters(AssetKind::Video), .kind = AssetKind::Video },
        };

        populate(targets, base, true, RM2K_EXCLUDED_DIRECTORIES);
    } else {
        populateOne(audioItem, base + u"/Audio", assetFilters(AssetKind::Audio));
        populateOne(dataItem, base + u"/Data", assetFilters(AssetKind::Data));
        populateOne(imagesItem, base + u"/Graphics", assetFilters(AssetKind::Image));
        populateOne(fontsItem, base + u"/Fonts", assetFilters(AssetKind::Font));
        populateOne(moviesItem, base + u"/Movies", assetFilters(AssetKind::Video));
    }

    const auto [saveDirectory, saveFilter, saveRecursive] = saveLocation(projectSettings);
    const Target saves = { .parent = savesItem, .filters = { saveFilter }, .kind = AssetKind::Save };
    populate(span(&saves, 1), saveDirectory, saveRecursive);
}
