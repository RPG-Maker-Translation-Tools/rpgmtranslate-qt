// The window is mostly wiring: a signal of one part leading to the reaction of another. These tests send the signal
// and look at what the window did, so a connection that is dropped shows.
#include "AboutWindow.hpp"
#include "HttpStub.hpp"
#include "MainWindowFixture.hpp"
#include "ModalCatcher.hpp"
#include "LintMenu.hpp"
#include "LintTable.hpp"
#include "FileSelectMenu.hpp"
#include "PurgeMenu.hpp"
#include "TranslationInput.hpp"
#include "TranslationsMenu.hpp"
#include "SerdeMenu.hpp"
#include "AssetMenu.hpp"
#include "BatchMenu.hpp"
#include "BookmarkList.hpp"
#include "BookmarkMenu.hpp"
#include "GlossaryMenu.hpp"
#include "ReadMenu.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "TabList.hpp"
#include "TabListModel.hpp"
#include "TaskPanel.hpp"
#include "TaskWorker.hpp"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStatusBar>
#include <QThreadPool>
#include <QToolButton>
#include <QtTest>

namespace {
constexpr auto ACTORS_DATA =
    R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])";

[[nodiscard]] auto actionNamed(const MainWindow& window, const QString& text) -> QAction* {
    for (QAction* const action : window.findChildren<QAction*>()) {
        // The docks have toggle actions of their own, named after their titles
        if (action->text() == text && action->parent() == &window) {
            return action;
        }
    }

    return nullptr;
}

// The actions the window makes itself have no names; their shortcuts are what tells them apart
[[nodiscard]] auto byShortcut(const MainWindow& window, const QString& keys) -> QAction* {
    for (QAction* const action : window.findChildren<QAction*>()) {
        if (action->shortcut() == QKeySequence(keys)) {
            return action;
        }
    }

    return nullptr;
}

// A file is purged when its box is ticked
// The menu is filled when the project has been read from the disk, a moment after the window is made
// The lists of files are siblings of their menus, so the file is ticked in every one of them: the other menus do not
// care.
auto tick(const MainWindow& window, const QString& name) -> bool {
    bool found = false;

    for (QCheckBox* const box : window.findChildren<QCheckBox*>()) {
        if (box->text() == name) {
            box->setChecked(true);
            found = true;
        }
    }

    return found;
}

// Everything the table of lint results shows
[[nodiscard]] auto lintTextOf(const LintTable* const table) -> QString {
    QString text;
    const auto* const model = table->model();

    for (i32 row = 0; row < model->rowCount(); row++) {
        for (i32 column = 0; column < model->columnCount(); column++) {
            text += model->data(model->index(row, column), Qt::DisplayRole).toString() + u'\n';
        }
    }

    return text;
}

void writeGame(const Env& env) {
    writeFile(env.project.path() + u"/data/Actors.json"_s, ACTORS_DATA);
    writeFile(env.project.path() + u"/data/MapInfos.json"_s, "[null]");
}

// Picks `text` in the menu that the "rvpacker" button opens, or closes it when there is no such entry
void chooseInOpenMenu(const QString& text) {
    QTimer::singleShot(100, [text] -> void {
        auto* const menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());

        if (menu == nullptr) {
            return;
        }

        for (QAction* const action : menu->actions()) {
            if (action->text() == text) {
                menu->setActiveAction(action);
                QTest::keyClick(menu, Qt::Key_Return);
                return;
            }
        }

        menu->close();
    });
}

// The labels of the translations menu, which has one for every endpoint
[[nodiscard]] auto translationsShown(const MainWindow& window) -> QStringList {
    QStringList texts;

    for (const QLabel* const label : window.findChild<TranslationsMenu*>()->findChildren<QLabel*>()) {
        texts.append(label->text());
    }

    return texts;
}

// Whether the action named `text` shows the panel and, triggered again, hides it
[[nodiscard]] auto toggles(const MainWindow& window, const QString& text, const QWidget* const panel) -> bool {
    QAction* const action = actionNamed(window, text);

    if (action == nullptr) {
        return false;
    }

    const bool before = panel->isHidden();
    action->trigger();
    const bool changed = panel->isHidden() != before;
    action->trigger();

    return changed && panel->isHidden() == before;
}
}  // namespace

class TestMainWindowConnections final : public QObject {
    Q_OBJECT

    unique_ptr<ModalCatcher> catcher;

   private slots:
    void initTestCase() { qputenv("RPGMTRANSLATE_MOCK_KEYRING", "1"); }

    void cleanup() {
        QThreadPool::globalInstance()->waitForDone();
        catcher.reset();
    }

    void theAboutActionShowsTheAboutWindow() {
        const Env env;
        writeGame(env);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QVERIFY(window.findChild<AboutWindow*>() == nullptr);

        actionNamed(window, u"About RPGMTranslate"_s)->trigger();

        auto* const about = window.findChild<AboutWindow*>();
        QVERIFY(about != nullptr);
        QVERIFY(about->isVisible());
    }

    void withoutLibgit2TheSourceControlActionSaysSo() {
        const Env env;
        writeGame(env);
        env.writeSettings();
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);
        QAction* const source = actionNamed(window, u"Source Control"_s);
        QVERIFY2(source != nullptr && source->isEnabled(), "the action is not there or not enabled");
        source->trigger();

#ifdef ENABLE_LIBGIT2
        QVERIFY(catcher->seen.isEmpty());
#else
        QTRY_VERIFY2_WITH_TIMEOUT(catcher->text.contains(u"without support for libgit2"_qsv), qPrintable(catcher->text + u"|"_s + catcher->seen.join(u";"_s)), 5000);
#endif
    }

    void aPurgeThatIsDeclinedChangesNothing() {
        const Env env;
        writeGame(env);
        const QByteArray original = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n";
        env.writeTranslation(u"actors"_s, original);
        env.writeSettings();
        catcher = make_unique<ModalCatcher>(QMessageBox::No);

        const MainWindow window;
        const WindowGuard guard(window);
        emit window.findChild<PurgeMenu*>()->accepted();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Purge removes"_qsv), 5000);
        QThreadPool::globalInstance()->waitForDone();
        QCOMPARE(env.readTranslation(u"actors"_s), original);
        catcher->text.clear();
    }

    void aPurgeThatIsConfirmedDropsTheEntriesNeverFilledIn() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\n");
        env.writeSettings();
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const menu = window.findChild<PurgeMenu*>();
        QTRY_VERIFY_WITH_TIMEOUT(tick(window, u"actors"_s), 10000);
        emit menu->accepted();

        QTRY_VERIFY_WITH_TIMEOUT(!env.readTranslation(u"actors"_s).contains("The Brave"), 15000);
        QVERIFY(env.readTranslation(u"actors"_s).contains("Гарольд"));
    }

    void exitingSavesWhatIsBeingEdited() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 2, 1, u"Гарольд"_s);

        actionNamed(window, u"Exit"_s)->trigger();

        QVERIFY(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд"));
        QVERIFY(QFileInfo::exists(env.settingsPath()));
    }

    void closingTheWindowSavesToo() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 2, 1, u"Гарольд"_s);

        // The event ignores the close and quits the program itself, once everything is written
        QCloseEvent event;
        QApplication::sendEvent(&window, &event);

        QVERIFY(!event.isAccepted());
        QVERIFY(env.readTranslation(u"actors"_s).contains("Harold<#>Гарольд"));
    }

    // Typing in a cell: the lint of that cell follows after a pause
    void typingInACellLintsIt() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        tableOf(window)->edit(tableOf(window)->currentIndex());
        QVERIFY(tableOf(window)->activeInput() != nullptr);

        auto* const lints = window.findChild<LintTable*>();
        tableOf(window)->activeInput()->setPlainText(u"  Гарольд  "_s);

        QTRY_VERIFY_WITH_TIMEOUT(lintTextOf(lints).contains(u"whitespace"_qsv, Qt::CaseInsensitive), 10000);

        // Typing more clears what was found before it
        tableOf(window)->activeInput()->setPlainText(u"Гарольд"_s);
        QTRY_VERIFY_WITH_TIMEOUT(!lintTextOf(lints).contains(u"whitespace"_qsv, Qt::CaseInsensitive), 10000);
    }

    // The shortcuts that run a script on the cell that has the focus
    void theSingleCellActionsRewriteTheCurrentCell() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        edit(window, 2, 1, u"  Гарольд  "_s);

        byShortcut(window, u"Ctrl+Shift+T"_s)->trigger();
        QCOMPARE(*tableOf(window)->model()->item(2, 1), u"Гарольд"_s);

        // Latinizing turns the punctuation of CJK text into its Latin form
        edit(window, 2, 1, u"Да。"_s);
        byShortcut(window, u"Ctrl+Shift+L"_s)->trigger();
        QCOMPARE(*tableOf(window)->model()->item(2, 1), u"Да."_s);
    }

    void theGlossaryLintSaysWhichLanguageIsMissing_data() {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("translation");
        QTest::addColumn<bool>("termFlag");
        QTest::addColumn<QString>("message");
        QTest::newRow("none set") << QString() << QString() << true << u"Set source and translation languages"_s;
        QTest::newRow("source missing") << QString() << u"en"_s << true << u"Set source language"_s;
        QTest::newRow("translation missing") << u"ja"_s << QString() << true << u"Set translation language"_s;
        QTest::newRow("both set") << u"ja"_s << u"en"_s << true << QString();
        QTest::newRow("flag off") << QString() << QString() << false << QString();
    }

    void theGlossaryLintSaysWhichLanguageIsMissing() {
        QFETCH(QString, source);
        QFETCH(QString, translation);
        QFETCH(bool, termFlag);
        QFETCH(QString, message);

        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        writeProjectSettings(env, [&](ProjectSettings& project) -> void {
            project.sourceLang = source;
            project.translationLang = translation;
        });
        env.writeSettings([termFlag](Settings& settings) -> void {
            if (!termFlag) {
                settings.translation.lintFlags = settings.translation.lintFlags & ~LintFlags::TermMismatch;
            }
        });

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        tableOf(window)->edit(tableOf(window)->currentIndex());
        auto* const lints = window.findChild<LintTable*>();
        tableOf(window)->activeInput()->setPlainText(u"Гарольд"_s);

        if (message.isEmpty()) {
            QTest::qWait(600);
            QVERIFY2(!lintTextOf(lints).contains(u"Settings > Project"_qsv), qPrintable(lintTextOf(lints)));
        } else {
            QTRY_VERIFY_WITH_TIMEOUT(lintTextOf(lints).contains(message), 10000);
        }
    }

    void withLintingOffTypingFindsNothing() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings([](Settings& settings) -> void { settings.translation.lintFlags = LintFlags::LintingDisabled; });

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        tableOf(window)->edit(tableOf(window)->currentIndex());
        tableOf(window)->activeInput()->setPlainText(u"  Гарольд  "_s);

        QTest::qWait(600);
        QCOMPARE(window.findChild<LintTable*>()->model()->rowCount(), 0);
    }

    void withoutLanguagesTheTranslationsMenuSaysWhatToSet() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings([](Settings& settings) -> void {
            // Nothing is asked of an endpoint while the languages are missing
            settings.translation.endpoints = { EndpointSettings{ .id = u"id1"_s,
                                                                  .name = u"Google"_s,
                                                                  .endpoint = TranslationEndpoint::Google } };
        });

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        tableOf(window)->edit(tableOf(window)->currentIndex());

        auto* const menu = window.findChild<TranslationsMenu*>();
        QTRY_VERIFY_WITH_TIMEOUT(
            [menu]() -> bool {
            for (const QLabel* const label : menu->findChildren<QLabel*>()) {
                if (label->text().contains(u"Settings > Project"_qsv)) {
                    return true;
                }
            }

            return false;
        }(),
            10000
        );
    }

    void theEndpointsTranslateTheCellThatGotTheFocus() {
        const HttpStub yandex([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("200 OK", R"({"translations":[{"text":"Hello there"}]})");
        });
        const HttpStub refusing([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("403 Forbidden", R"({"message":"bad key"})");
        });

        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        writeProjectSettings(env, [](ProjectSettings& project) -> void {
            project.sourceLang = u"ja"_s;
            project.translationLang = u"en"_s;
        });
        env.writeSettings([&](Settings& settings) -> void {
            const auto yandexAt = [](const QString& name, const QString& url) -> EndpointSettings {
                return EndpointSettings{ .id = name + u"-id"_s,
                                         .name = name,
                                         .apiKey = u"key"_s,
                                         .yandexFolderId = u"folder"_s,
                                         .yandexBaseUrl = url,
                                         .endpoint = TranslationEndpoint::Yandex };
            };
            settings.translation.endpoints = { yandexAt(u"Working"_s, yandex.url()),
                                               yandexAt(u"Refusing"_s, refusing.url()) };
        });

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        tableOf(window)->edit(tableOf(window)->currentIndex());

        QTRY_VERIFY_WITH_TIMEOUT(translationsShown(window).contains(u"Hello there"_s), 15000);
        const QStringList shown = translationsShown(window);
        QVERIFY(shown.contains(u"Working"_s));
        QVERIFY(shown.contains(u"Refusing"_s));
        // What the refusing endpoint answered is shown as its reason
        QVERIFY(ranges::any_of(shown, [](const QString& text) -> bool { return text.contains(u"403"_qsv); }));
    }

    void anEndpointLeftFromOlderVersionsIsDroppedOnStart() {
        const Env env;
        writeGame(env);
        env.writeSettings([](Settings& settings) -> void {
            // A script endpoint always has a script, so one without it is what older versions left behind
            settings.translation.endpoints = { EndpointSettings{ .id = u"id1"_s,
                                                                  .name = u"Leftover"_s,
                                                                  .endpoint = TranslationEndpoint::Script } };
        });

        const MainWindow window;
        const WindowGuard guard(window);
        actionNamed(window, u"Exit"_s)->trigger();

        QVERIFY(QFileInfo::exists(env.settingsPath()));
        QVERIFY(!readFile(env.settingsPath()).contains("Leftover"));
    }

    void anImportThatIsDeclinedIsNotStarted() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\n");
        env.writeSettings();
        catcher = make_unique<ModalCatcher>(QMessageBox::No);

        const MainWindow window;
        const WindowGuard guard(window);
        emit window.findChild<SerdeMenu*>()->importRequested();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"Import replaces"_qsv), 5000);
        QVERIFY(env.readTranslation(u"actors"_s).contains("Гарольд"));
        catcher->text.clear();
    }

    void aTabCanBeMarkedCompletedFromItsMenuAndUnmarkedAgain() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        {
            const MainWindow window;
            auto* const tabList = window.findChild<TabList*>();
            QVERIFY(tabList->model()->rowCount() > 0);
            const QModelIndex index = tabList->model()->index(0, 0);

            chooseInOpenMenu(u"Mark as Completed"_s);
            emit tabList->rightClicked(index, QPoint(0, 0));
            closeProject(window);
        }

        QVERIFY(savedProjectSettings(env)->completedFiles.contains(u"actors"_s));

        {
            const MainWindow window;
            auto* const tabList = window.findChild<TabList*>();

            chooseInOpenMenu(u"Unmark Completed"_s);
            emit tabList->rightClicked(tabList->model()->index(0, 0), QPoint(0, 0));
            closeProject(window);
        }

        QVERIFY(!savedProjectSettings(env)->completedFiles.contains(u"actors"_s));
    }

    void thePercentDisplayOfTheTabsCanBeToggledFromTheirMenu() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();

        {
            const MainWindow window;
            auto* const tabList = window.findChild<TabList*>();

            chooseInOpenMenu(u"Toggle Progress Display"_s);
            emit tabList->rightClicked(tabList->model()->index(0, 0), QPoint(0, 0));
            closeProject(window);
        }

        QVERIFY(readFile(env.settingsPath()).contains("\"displayPercents\":true"));
    }

    void theButtonsOfThePurgeMenuAskForThePurgeAndCloseIt() {
        const Env env;
        writeGame(env);
        env.writeSettings();
        // Accepting asks to confirm the purge, which is declined
        catcher = make_unique<ModalCatcher>(QMessageBox::No);

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const menu = window.findChild<PurgeMenu*>();
        QSignalSpy accepted(menu, &PurgeMenu::accepted);
        QSignalSpy rejected(menu, &PurgeMenu::rejected);

        menu->show();
        menu->findChild<QPushButton*>(u"applyButton"_s)->click();
        QCOMPARE(accepted.count(), 1);
        QVERIFY(menu->isHidden());
        QVERIFY(catcher->text.contains(u"Purge removes"_qsv));
        catcher->text.clear();

        menu->show();
        menu->findChild<QPushButton*>(u"closeButton"_s)->click();
        QCOMPARE(rejected.count(), 1);
        QVERIFY(menu->isHidden());
    }

    void theFileSelectButtonOfThePurgeMenuTogglesTheFileList() {
        const Env env;
        writeGame(env);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const menu = window.findChild<PurgeMenu*>();
        auto* const button = menu->findChild<QPushButton*>(u"fileSelectButton"_s);
        menu->show();

        // The list is one of the file menus of the window, and the only one that this button changes
        const auto shown = [&window]() -> i32 {
            i32 count = 0;

            for (const FileSelectMenu* const list : window.findChildren<FileSelectMenu*>()) {
                count += !list->isHidden();
            }

            return count;
        };

        const i32 before = shown();
        button->click();
        QCOMPARE(shown(), before + 1);
        button->click();
        QCOMPARE(shown(), before);
    }

    void theLogOfTheRustCoreNamesTheFileATaskIsOn() {
        const Env env;
        writeGame(env);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        const auto task = window.findChild<TaskWorker*>()->startTask(TaskKind::Read);
        QVERIFY(task.has_value());

        const auto taskLabelShown = [&window](const QString& file) -> bool {
            const QString suffix = u": "_qsv % file;

            for (const QLabel* const label : window.findChild<TaskPanel*>()->findChildren<QLabel*>()) {
                if (label->text().endsWith(suffix)) {
                    return true;
                }
            }

            return false;
        };

        // Every level is passed on to the log of the program; only a line that ends a file of the running task names it
        FFILogger& logger = FFILogger::instance();
        emit logger.logReceived(0, u"[core] critical line"_s);
        emit logger.logReceived(1, u"[core] warning line"_s);
        emit logger.logReceived(3, u"[core] debug line"_s);
        emit logger.logReceived(2, u"[core] info line"_s);
        emit logger.logReceived(2, u"nospaces:read."_s);
        QVERIFY(!taskLabelShown(u"actors"_s));

        emit logger.logReceived(2, u"[core] actors: 3 read."_s);
        QVERIFY(taskLabelShown(u"actors"_s));

        // A task of another kind is not the one that read
        emit logger.logReceived(2, u"[core] items: 3 written."_s);
        QVERIFY(!taskLabelShown(u"items"_s));

        window.findChild<TaskWorker*>()->finishTask(*task);
    }

    void aBookmarkClickedInTheMenuOpensItsTab() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        QVERIFY(tabsOf(window)->currentTabName().isEmpty());

        emit window.findChild<BookmarkMenu*>()->bookmarkClicked(QL1SV("actors"), 2);

        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);
        // The row is scrolled into view a moment later
        QTest::qWait(50);
    }

    // The entry of a bookmark is that of the nearest `<!>NAME` row above it, and the `<!>ID` row starting an entry ends the
    // search. Rows: 0 ID, 1 NAME, 2 text, 3 ID, 4 text, 5 NAME, 6 text.
    void aBookmarkRemembersTheEntryItIsIn_data() {
        QTest::addColumn<u32>("row");
        QTest::addColumn<QString>("entry");

        QTest::newRow("under the name") << u32{ 2 } << u"First"_s;
        QTest::newRow("on the name row") << u32{ 1 } << QString();
        QTest::newRow("id row stops the search") << u32{ 4 } << QString();
        QTest::newRow("second name") << u32{ 6 } << u"Second"_s;
        QTest::newRow("first row") << u32{ 0 } << QString();
    }

    void aBookmarkRemembersTheEntryItIsIn() {
        QFETCH(const u32, row);
        QFETCH(const QString, entry);

        const Env env;
        writeGame(env);
        env.writeTranslation(
            u"actors"_s,
            "<!>ID<#>1\n<!>NAME<#>First\nHarold<#>\n<!>ID<#>2\nSomeone<#>\n<!>NAME<#>Second\nBrave<#>\n"
        );
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        emit window.findChild<BookmarkMenu*>()->bookmarkClicked(QL1SV("actors"), 0);
        QCOMPARE(tabsOf(window)->currentTabName(), u"actors"_s);

        emit tableOf(window)->bookmarked(row);

        const auto* const list = window.findChild<BookmarkMenu*>()->findChild<BookmarkList*>();
        QCOMPARE(list->rowCount(), 1);
        QCOMPARE(list->bookmark(0).row, row);
        QCOMPARE(list->bookmark(0).entry, entry);
        QCOMPARE(QL1SV(list->bookmark(0).filename.data()), u"actors"_qsv);
    }

    void aChangedBookmarkTakesTheTextOfItsRow() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        emit window.findChild<BookmarkMenu*>()->bookmarkClicked(QL1SV("actors"), 0);
        emit tableOf(window)->bookmarked(3);
        edit(window, 3, 1, u"Der Tapfere"_s);

        emit tableOf(window)->bookmarkChanged(3);

        const auto* const list = window.findChild<BookmarkMenu*>()->findChild<BookmarkList*>();
        QCOMPARE(list->bookmark(0).description, u"Der Tapfere"_s);
    }

    void cuttingCopyingAndPastingRowsIsReported() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const table = tableOf(window);

        emit table->multilineAction(TranslationTable::MultilineAction::Cut, 3);
        QVERIFY(window.statusBar()->currentMessage().contains(u"3 row(s) cut"_qsv));

        emit table->multilineAction(TranslationTable::MultilineAction::Copy, 2);
        QVERIFY(window.statusBar()->currentMessage().contains(u"2 row(s) copied"_qsv));

        emit table->multilineAction(TranslationTable::MultilineAction::Paste, 1);
        QVERIFY(window.statusBar()->currentMessage().contains(u"1 row(s) pasted"_qsv));
    }

    void everyMenuActionShowsAndHidesItsMenu() {
        const Env env;
        writeGame(env);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);

        QVERIFY(toggles(window, u"Search"_s, window.findChild<SearchMenu*>()));
        QVERIFY(toggles(window, u"Batch Menu"_s, window.findChild<BatchMenu*>()));
        QVERIFY(toggles(window, u"Glossary Menu"_s, window.findChild<GlossaryMenu*>()));
        QVERIFY(toggles(window, u"Translations Menu"_s, window.findChild<TranslationsMenu*>()));
        QVERIFY(toggles(window, u"Bookmark Menu"_s, window.findChild<BookmarkMenu*>()));
        QVERIFY(toggles(window, u"Search Panel"_s, window.findChild<SearchPanelDock*>()));
        QVERIFY(toggles(window, u"Assets"_s, window.findChild<AssetMenu*>()));
        QVERIFY(toggles(window, u"Lint Menu"_s, window.findChild<LintMenu*>()));
        QVERIFY(toggles(window, u"Tab Panel"_s, tabsOf(window)));
    }

    void savingIsRefusedWhileTheFilesAreBeingRewritten() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        edit(window, 2, 1, u"Гарольд"_s);

        const auto task = window.findChild<TaskWorker*>()->startTask(TaskKind::Replace);
        QVERIFY(task.has_value());

        // The action is greyed out meanwhile, a menu opened before can still send it
        emit actionNamed(window, u"Save"_s)->triggered();

        QVERIFY(window.statusBar()->currentMessage().contains(u"Cannot save while"_qsv));
        QVERIFY(!env.readTranslation(u"actors"_s).contains("Гарольд"));

        window.findChild<TaskWorker*>()->finishTask(*task);
    }

    void whatIsDoneToTheTableIsRememberedWithTheProject() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        {
            const MainWindow window;
            tabsOf(window)->changeTab(u"actors"_s);

            emit tabsOf(window)->completedToggled(u"actors"_s, true);
            emit tableOf(window)->columnAdded();
            emit tableOf(window)->columnRenamed(1, u"Русский"_s);
            emit tableOf(window)->columnResized(1, 321);
            // A column the project does not have is ignored
            emit tableOf(window)->columnRenamed(40, u"Nobody"_s);
            emit tableOf(window)->columnResized(40, 10);

            closeProject(window);
        }

        const auto saved = savedProjectSettings(env);
        QVERIFY(saved.has_value());
        QVERIFY(saved->completedFiles.contains(u"actors"_s));
        QVERIFY(saved->columns.size() >= 3);
        QCOMPARE(saved->columns[1].name, u"Русский"_s);
        QCOMPARE(saved->columns[1].width, u16{ 321 });

        // Turned back, the tab is no longer completed
        {
            const MainWindow window;
            emit tabsOf(window)->completedToggled(u"actors"_s, false);
            closeProject(window);
        }

        QVERIFY(!savedProjectSettings(env)->completedFiles.contains(u"actors"_s));
    }

    void aTranslationClickedInTheMenuIsInsertedIntoTheCell() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>\n");
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        tabsOf(window)->changeTab(u"actors"_s);
        tableOf(window)->setCurrentIndex(tableOf(window)->model()->index(2, 1));
        tableOf(window)->edit(tableOf(window)->currentIndex());
        QVERIFY(tableOf(window)->activeInput() != nullptr);

        emit window.findChild<TranslationsMenu*>()->translationClicked(u"Гарольд"_s);

        QCOMPARE(tableOf(window)->activeInput()->toPlainText(), u"Гарольд"_s);
    }

    void theTaskPanelOpensOnlyWhileThereAreTasks() {
        const Env env;
        writeGame(env);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const panel = window.findChild<TaskPanel*>();
        auto* const button = window.findChild<QToolButton*>(u"taskButton"_s);

        emit button->pressed();
        QVERIFY(panel->isHidden());

        const auto task = window.findChild<TaskWorker*>()->startTask(TaskKind::Replace);
        QVERIFY(task.has_value());
        QCOMPARE(panel->taskCount(), 1);

        emit button->pressed();
        QVERIFY(!panel->isHidden());

        // With the last task gone it hides itself
        window.findChild<TaskWorker*>()->finishTask(*task);
        QVERIFY(panel->isHidden());
    }

    void theRvpackerMenuOpensTheMenuThatWasChosen() {
        const Env env;
        writeGame(env);
        env.writeSettings();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const button = window.findChild<QToolButton*>(u"rvpackerButton"_s);
        QVERIFY(button != nullptr);

        chooseInOpenMenu(u"Purge"_s);
        button->click();
        QVERIFY(!window.findChild<PurgeMenu*>()->isHidden());
        window.findChild<PurgeMenu*>()->hide();

        chooseInOpenMenu(u"Export/Import"_s);
        button->click();
        QVERIFY(!window.findChild<SerdeMenu*>()->isHidden());
        window.findChild<SerdeMenu*>()->hide();

        chooseInOpenMenu(u"Read"_s);
        button->click();
        QVERIFY(!window.findChild<ReadMenu*>()->isHidden());
        window.findChild<ReadMenu*>()->hide();

        chooseInOpenMenu(u"nothing"_s);
        button->click();
        QVERIFY(window.findChild<PurgeMenu*>()->isHidden());
        QVERIFY(window.findChild<SerdeMenu*>()->isHidden());
    }

    void aFolderChosenInTheDialogIsOpenedAsANewProject() {
        const Env env;
        writeGame(env);
        forgetProgramFolder(env);
        env.writeSettings([](Settings& settings) -> void { settings.core.projectPath.clear(); });
        catcher = make_unique<ModalCatcher>();
        catcher->file = env.project.path();

        const MainWindow window;
        const WindowGuard guard(window);
        answerFirstRead(window, true);

        actionNamed(window, u"Open Folder"_s)->trigger();

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QTRY_VERIFY_WITH_TIMEOUT(env.readTranslation(u"actors"_s).contains("Harold"), 15000);
    }

    void aDialogThatIsCancelledOpensNothing() {
        const Env env;
        writeGame(env);
        env.writeSettings([](Settings& settings) -> void { settings.core.projectPath.clear(); });
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);

        actionNamed(window, u"Open Folder"_s)->trigger();

        QCOMPARE(catcher->dialogs.size(), qsizetype{ 1 });
        QCOMPARE(tabsOf(window)->tabCount(), 0);
    }

    void checkingTheSourcesOnRequestListsTheFilesThatWereNeverRead() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);
        window.findChild<QAction*>(u"actionCheckSourceUpdates"_s)->trigger();

        QCOMPARE(catcher->dialogs, QStringList{ u"Source files have been updated"_s });
        QVERIFY(catcher->text.contains(u"New files:"_qsv));
        catcher->text.clear();
    }

    void checkingTheSourcesOfAGameWhoseFolderIsGoneSaysSo() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "Harold<#>\n");
        env.writeSettings();
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);
        QVERIFY(QDir(env.project.path() + u"/data"_s).removeRecursively());
        window.findChild<QAction*>(u"actionCheckSourceUpdates"_s)->trigger();

        QVERIFY(catcher->text.contains(u"does not exist"_qsv));
        catcher->text.clear();
    }

    void anExportWritesTheTranslationToTheChosenFolder() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\n");
        env.writeSettings();
        const QTemporaryDir target;

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const menu = window.findChild<SerdeMenu*>();
        QTRY_VERIFY_WITH_TIMEOUT(tick(window, u"actors"_s), 10000);
        menu->findChild<QLineEdit*>(u"directoryInput"_s)->setText(target.path());

        emit menu->exportRequested();
        QThreadPool::globalInstance()->waitForDone();

        QTRY_VERIFY_WITH_TIMEOUT(!QDir(target.path()).isEmpty(), 10000);
    }

    void anExportThatCannotBeWrittenNamesTheFilesItSkipped() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\n");
        env.writeSettings();
        const QTemporaryDir target;
        // A file stands where the folder should be
        const QString blocked = target.path() + u"/blocked"_s;
        writeFile(blocked, "x");
        catcher = make_unique<ModalCatcher>();

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const menu = window.findChild<SerdeMenu*>();
        QTRY_VERIFY_WITH_TIMEOUT(tick(window, u"actors"_s), 10000);
        menu->findChild<QLineEdit*>(u"directoryInput"_s)->setText(blocked);

        emit menu->exportRequested();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->text.contains(u"unable to open the following files"_qsv), 10000);
        QVERIFY(catcher->text.contains(u"actors"_qsv));
        catcher->text.clear();
    }

    // The result of a task that the closing of the project waited for is for nobody: no box over a closed project
    void aSerdeTaskThatIsStoppedByClosingTheProjectReportsNothing_data() {
        QTest::addColumn<bool>("isImport");
        QTest::newRow("export") << false;
        QTest::newRow("import") << true;
    }

    void aSerdeTaskThatIsStoppedByClosingTheProjectReportsNothing() {
        QFETCH(bool, isImport);

        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\n");
        env.writeSettings();
        const QTemporaryDir target;
        // A file stands where the folder should be: left running, the task would name the files it skipped
        const QString blocked = target.path() + u"/blocked"_s;
        writeFile(blocked, "x");
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        auto* const menu = window.findChild<SerdeMenu*>();
        QTRY_VERIFY_WITH_TIMEOUT(tick(window, u"actors"_s), 10000);
        menu->findChild<QLineEdit*>(u"directoryInput"_s)->setText(blocked);

        if (isImport) {
            emit menu->importRequested();
        } else {
            emit menu->exportRequested();
        }

        // The result of the task is delivered by the event loop, which closing the project runs while it waits
        closeProject(window);
        QThreadPool::globalInstance()->waitForDone();
        QTRY_VERIFY(window.findChild<TaskWorker*>()->idle());

        QVERIFY2(!catcher->text.contains(u"unable to open"_qsv), qPrintable(catcher->text));
        // What the import asks first is a box of its own
        catcher->text.clear();
        catcher->seen.clear();
    }

    void anImportFromAFolderWithoutTheFilesNamesThemAndKeepsTheTranslation() {
        const Env env;
        writeGame(env);
        env.writeTranslation(u"actors"_s, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\n");
        env.writeSettings();
        const QTemporaryDir source;
        catcher = make_unique<ModalCatcher>(QMessageBox::Yes);

        const MainWindow window;
        const WindowGuard guard(window);
        auto* const menu = window.findChild<SerdeMenu*>();
        QTRY_VERIFY_WITH_TIMEOUT(tick(window, u"actors"_s), 10000);
        menu->findChild<QLineEdit*>(u"directoryInput"_s)->setText(source.path());

        emit menu->importRequested();

        QTRY_VERIFY_WITH_TIMEOUT(catcher->seen.size() >= 2, 10000);
        QVERIFY2(catcher->seen.last().contains(u"unable to open the following files"_qsv), qPrintable(catcher->seen.join(u'|')));
        QVERIFY(catcher->seen.last().contains(u"actors"_qsv));
        QVERIFY(env.readTranslation(u"actors"_s).contains("Гарольд"));
        catcher->text.clear();
    }
};

QTEST_MAIN(TestMainWindowConnections)
#include "tst_mainwindow_connections.moc"
