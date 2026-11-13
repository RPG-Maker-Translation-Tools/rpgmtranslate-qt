// The asset menu lists the files of a game by kind, wherever its engine keeps them, and filters that list.
#include "AssetMenu.hpp"
#include "AssetPreviewWidget.hpp"
#include "Constants.hpp"
#include "MarkingTranslator.hpp"
#include "ProjectSettings.hpp"
#include "TestFiles.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>

namespace {
// The first item below `parent` (or at the top) with this text, at any depth
[[nodiscard]] auto find(QTreeWidgetItem* const parent, const QString& text) -> QTreeWidgetItem* {
    if (parent->text(0) == text) {
        return parent;
    }

    for (i32 idx = 0; idx < parent->childCount(); idx++) {
        if (auto* const found = find(parent->child(idx), text)) {
            return found;
        }
    }

    return nullptr;
}

[[nodiscard]] auto category(const QTreeWidget* const tree, const QString& text) -> QTreeWidgetItem* {
    for (i32 idx = 0; idx < tree->topLevelItemCount(); idx++) {
        if (tree->topLevelItem(idx)->text(0) == text) {
            return tree->topLevelItem(idx);
        }
    }

    return nullptr;
}

// The names of the files below `item`, in the order the menu lists them
void filesBelow(const QTreeWidgetItem* const item, QStringList& names) {
    if (item->childCount() == 0) {
        names.append(item->text(0));
        return;
    }

    for (i32 idx = 0; idx < item->childCount(); idx++) {
        filesBelow(item->child(idx), names);
    }
}

[[nodiscard]] auto filesOf(const QTreeWidget* const tree, const QString& categoryName) -> QStringList {
    QStringList names;
    const QTreeWidgetItem* const item = category(tree, categoryName);

    if (item != nullptr && item->childCount() > 0) {
        filesBelow(item, names);
    }

    return names;
}

void touch(const QTemporaryDir& dir, const QString& relative, const QByteArray& content = "x") {
    const QString path = dir.path() + u'/' + relative;
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    writeFile(path, content);
}

[[nodiscard]] auto previewShown() -> bool {
    for (const QWidget* const widget : QApplication::allWidgets()) {
        if (qobject_cast<const AssetPreviewWidget*>(widget) != nullptr && widget->isVisible()) {
            return true;
        }
    }

    return false;
}
}  // namespace

class TestAssetMenu final : public QObject {
    Q_OBJECT

    QTemporaryDir dir;
    ProjectSettings project;

    [[nodiscard]] auto treeOf(const AssetMenu& menu) const -> QTreeWidget* { return menu.findChild<QTreeWidget*>(); }

   private slots:
    void init() {
        for (const QString& name : QDir(dir.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden)) {
            const QString path = dir.path() + u'/' + name;
            QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
        }

        project = ProjectSettings();
        project.projectPath = dir.path();
    }

    void anMvGameKeepsItsAssetsInTheFoldersOfTheEngine() {
        touch(dir, u"audio/bgm/Theme.ogg"_s);
        touch(dir, u"data/Actors.json"_s);
        touch(dir, u"img/pictures/Hero.png"_s);
        touch(dir, u"icon/icon.png"_s);
        touch(dir, u"fonts/mplus.ttf"_s);
        touch(dir, u"movies/intro.webm"_s);
        touch(dir, u"js/plugins.js"_s);
        touch(dir, u"save/file1.rpgsave"_s);
        touch(dir, u"data/notes.txt"_s);
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);
        const QTreeWidget* const tree = treeOf(menu);

        QCOMPARE(filesOf(tree, u"Audio"_s), QStringList{ u"Theme.ogg"_s });
        // What is not an asset is not listed
        QCOMPARE(filesOf(tree, u"Data"_s), QStringList{ u"Actors.json"_s });
        QCOMPARE(filesOf(tree, u"Images"_s), QStringList{ u"Hero.png"_s });
        QCOMPARE(filesOf(tree, u"Icons"_s), QStringList{ u"icon.png"_s });
        QCOMPARE(filesOf(tree, u"Fonts"_s), QStringList{ u"mplus.ttf"_s });
        QCOMPARE(filesOf(tree, u"Movies"_s), QStringList{ u"intro.webm"_s });
        QCOMPARE(filesOf(tree, u"JS"_s), QStringList{ u"plugins.js"_s });
        QCOMPARE(filesOf(tree, u"Saves"_s), QStringList{ u"file1.rpgsave"_s });

        // Subfolders are branches
        QVERIFY(find(category(tree, u"Audio"_s), u"bgm"_s) != nullptr);
    }

    void aGameInAWwwFolderIsLookedForThere() {
        touch(dir, u"www/audio/bgm/Theme.ogg"_s);
        touch(dir, u"www/save/file1.rpgsave"_s);
        touch(dir, u"audio/bgm/Outside.ogg"_s);
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);

        QCOMPARE(filesOf(treeOf(menu), u"Audio"_s), QStringList{ u"Theme.ogg"_s });
        QCOMPARE(filesOf(treeOf(menu), u"Saves"_s), QStringList{ u"file1.rpgsave"_s });
    }

    void aVxAceGameKeepsItsAssetsInTheFoldersOfTheEngine() {
        touch(dir, u"Audio/BGM/Theme.ogg"_s);
        touch(dir, u"Data/System.rvdata2"_s);
        touch(dir, u"Graphics/Pictures/Hero.png"_s);
        touch(dir, u"Fonts/mplus.ttf"_s);
        touch(dir, u"Movies/intro.ogv"_s);
        touch(dir, u"Save01.rvdata2"_s);
        project.engineType = EngineType::VXAce;

        AssetMenu menu;
        menu.init(&project);
        const QTreeWidget* const tree = treeOf(menu);

        QCOMPARE(filesOf(tree, u"Audio"_s), QStringList{ u"Theme.ogg"_s });
        QCOMPARE(filesOf(tree, u"Data"_s), QStringList{ u"System.rvdata2"_s });
        QCOMPARE(filesOf(tree, u"Images"_s), QStringList{ u"Hero.png"_s });
        QCOMPARE(filesOf(tree, u"Fonts"_s), QStringList{ u"mplus.ttf"_s });
        QCOMPARE(filesOf(tree, u"Movies"_s), QStringList{ u"intro.ogv"_s });
        QCOMPARE(filesOf(tree, u"Saves"_s), QStringList{ u"Save01.rvdata2"_s });
    }

    void anRm2kGameHasNoFoldersOfItsOwnAndTheMapsComeInOrder() {
        touch(dir, u"Music/Song.mid"_s);
        touch(dir, u"Picture/Face.png"_s);
        touch(dir, u"Movie/clip.avi"_s);
        touch(dir, u"RPG_RT.ldb"_s);
        touch(dir, u"Map0010.lmu"_s);
        touch(dir, u"Map0002.lmu"_s);
        touch(dir, u"Map0001.lmu"_s);
        touch(dir, u"Save01.lsd"_s);
        // The tool's own copies of the game are not part of it
        touch(dir, u".rpgmtranslate/baseline-data/data/Map0003.lmu"_s);
        touch(dir, u".rpgmtranslate/translation/Skipped.png"_s);
        touch(dir, u"translation/Skipped2.png"_s);
        project.engineType = EngineType::RM2K;
        project.sourceDirectory = SourceDirectory::Root;

        AssetMenu menu;
        menu.init(&project);
        const QTreeWidget* const tree = treeOf(menu);

        QCOMPARE(filesOf(tree, u"Data"_s), (QStringList{ u"Map0001.lmu"_s, u"Map0002.lmu"_s, u"Map0010.lmu"_s, u"RPG_RT.ldb"_s }));
        QCOMPARE(filesOf(tree, u"Audio"_s), QStringList{ u"Song.mid"_s });
        QCOMPARE(filesOf(tree, u"Images"_s), QStringList{ u"Face.png"_s });
        QCOMPARE(filesOf(tree, u"Movies"_s), QStringList{ u"clip.avi"_s });
        QCOMPARE(filesOf(tree, u"Saves"_s), QStringList{ u"Save01.lsd"_s });
    }

    void aGameWithoutItsFoldersHasEmptyCategories() {
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);
        const QTreeWidget* const tree = treeOf(menu);

        QVERIFY(tree->topLevelItemCount() > 0);
        QVERIFY(filesOf(tree, u"Audio"_s).isEmpty());
        QVERIFY(filesOf(tree, u"Data"_s).isEmpty());
    }

    void withoutAProjectThereIsNothingToList() {
        AssetMenu menu;
        menu.init(&project);
        QVERIFY(treeOf(menu)->topLevelItemCount() > 0);

        menu.init(nullptr);

        QCOMPARE(treeOf(menu)->topLevelItemCount(), 0);
    }

    void theFilterKeepsTheFilesWhoseNamesHaveTheText() {
        touch(dir, u"audio/bgm/Theme.ogg"_s);
        touch(dir, u"audio/se/Theme2.ogg"_s);
        touch(dir, u"audio/se/Cursor.ogg"_s);
        touch(dir, u"data/Actors.json"_s);
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);
        const QTreeWidget* const tree = treeOf(menu);
        auto* const search = menu.findChild<QLineEdit*>();

        // The case does not matter
        search->setText(u"THEME"_s);

        QVERIFY(!find(category(tree, u"Audio"_s), u"Theme.ogg"_s)->isHidden());
        QVERIFY(!find(category(tree, u"Audio"_s), u"Theme2.ogg"_s)->isHidden());
        QVERIFY(find(category(tree, u"Audio"_s), u"Cursor.ogg"_s)->isHidden());
        QVERIFY(category(tree, u"Data"_s)->isHidden());
        // A category with a match is opened, a branch without one is hidden
        QVERIFY(category(tree, u"Audio"_s)->isExpanded());

        search->setText(u"cursor"_s);
        QVERIFY(find(category(tree, u"Audio"_s), u"bgm"_s)->isHidden());
        QVERIFY(!find(category(tree, u"Audio"_s), u"se"_s)->isHidden());

        search->clear();
        QVERIFY(!category(tree, u"Data"_s)->isHidden());
        QVERIFY(!find(category(tree, u"Audio"_s), u"Cursor.ogg"_s)->isHidden());
    }

    void refreshingPicksUpNewFilesAndClearsTheFilter() {
        touch(dir, u"data/Actors.json"_s);
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);
        menu.findChild<QLineEdit*>()->setText(u"zzz"_s);
        touch(dir, u"data/Items.json"_s);

        menu.findChild<QPushButton*>()->click();

        QVERIFY(menu.findChild<QLineEdit*>()->text().isEmpty());
        QCOMPARE(filesOf(treeOf(menu), u"Data"_s), (QStringList{ u"Actors.json"_s, u"Items.json"_s }));
    }

    void clickingAFileShowsItAndClosingTheProjectHidesIt() {
        touch(dir, u"data/Actors.json"_s, "[null]");
        touch(dir, u"audio/bgm/Theme.ogg"_s);
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);
        QTreeWidget* const tree = treeOf(menu);

        // A branch has no file behind it
        QTreeWidgetItem* const branch = find(category(tree, u"Audio"_s), u"bgm"_s);
        emit tree->itemClicked(branch, 0);
        QVERIFY(!previewShown());

        emit tree->itemClicked(find(category(tree, u"Data"_s), u"Actors.json"_s), 0);
        QVERIFY(previewShown());

        menu.clear();

        QVERIFY(!previewShown());
        QCOMPARE(tree->topLevelItemCount(), 0);
    }

    void theCategoriesAreRenamedWhenTheLanguageChanges() {
        touch(dir, u"data/Actors.json"_s);
        project.engineType = EngineType::MVMZ;

        AssetMenu menu;
        menu.init(&project);

        const MarkingTranslator translator;
        translator.retranslate(&menu);

        QVERIFY(category(treeOf(menu), MarkingTranslator::MARK + u"Audio"_s) != nullptr);
        QVERIFY(menu.findChild<QLineEdit*>()->placeholderText().startsWith(MarkingTranslator::MARK));
    }
};

QTEST_MAIN(TestAssetMenu)
#include "tst_assetmenu.moc"
