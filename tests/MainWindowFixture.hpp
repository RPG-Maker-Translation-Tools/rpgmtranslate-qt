#pragma once

#include "Constants.hpp"
#include "MainWindow.hpp"
#include "ProjectSettings.hpp"
#include "ReadMenu.hpp"
#include "Settings.hpp"
#include "TabPanel.hpp"
#include "glazemeta.hpp"
#include "TestFiles.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"

#include <QAction>
#include <QDir>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QToolButton>
#include <QtTest>
#include <functional>

// What the tests that drive the whole main window share
namespace {
// One project and the application data directory it is opened from
class Env {
   public:
    Env() {
        qApp->setProperty("data-location", data.path());

        QDir().mkpath(translationPath());
        QDir().mkpath(project.path() + u"/data"_s);
        writeFile(project.path() + u"/data/System.json"_s, "{}");
        // An MV project: its plugin list is part of what is copied as the baseline
        QDir().mkpath(project.path() + u"/js"_s);
        writeFile(project.path() + u"/js/plugins.js"_s, "var $plugins = [];");
    }

    [[nodiscard]] auto translationPath() const -> QString {
        return project.path() + PROGRAM_DATA_DIRECTORY.toString() + TRANSLATION_DIRECTORY.toString();
    }

    [[nodiscard]] auto translationFile(const QString& name) const -> QString {
        return translationPath() + u'/' + name + u".txt"_qsv;
    }

    [[nodiscard]] auto settingsPath() const -> QString { return data.path() + u"/settings.json"_s; }

    void writeTranslation(const QString& name, const QByteArray& content) const {
        writeFile(translationFile(name), content);
    }

    [[nodiscard]] auto readTranslation(const QString& name) const -> QByteArray {
        return readFile(translationFile(name));
    }

    // The settings that reopen the project, without the network and the timers that would disturb a test
    void writeSettings(const std::function<void(Settings&)>& tweak = {}) const {
        Settings settings;
        settings.core.projectPath = project.path();
        settings.core.checkForAppUpdates = false;
        settings.core.checkForSourceUpdates = false;
        settings.core.backup.enabled = false;
        settings.appearance.language = QLocale::English;

        if (tweak) {
            tweak(settings);
        }

        const auto json = glz::write_json(settings);
        QVERIFY(json.has_value());
        writeFile(settingsPath(), QByteArray(json->data(), static_cast<qsizetype>(json->size())));
    }

    QTemporaryDir data;
    QTemporaryDir project;
};

[[nodiscard]] inline auto tabsOf(const MainWindow& window) -> TabPanel* {
    return window.findChild<TabPanel*>();
}

[[nodiscard]] inline auto tableOf(const MainWindow& window) -> TranslationTable* {
    return window.findChild<TranslationTable*>();
}

[[nodiscard]] inline auto titleInputOf(const MainWindow& window) -> QLineEdit* {
    return window.findChild<QLineEdit*>(u"gameTitleInput"_s);
}

inline void closeTab(const MainWindow& window) {
    window.findChild<QAction*>(u"actionCloseTab"_s)->trigger();
}

inline void closeProject(const MainWindow& window) {
    window.findChild<QAction*>(u"actionCloseProject"_s)->trigger();
}

inline void save(const MainWindow& window) {
    window.findChild<QToolButton*>(u"saveButton"_s)->click();
}

inline void edit(const MainWindow& window, const i32 row, const i32 column, const QString& text) {
    auto* const model = tableOf(window)->model();
    QVERIFY(model->setData(model->index(row, column), text));
}

[[nodiscard]] inline auto programFile(const Env& env, const QString& name) -> QString {
    return env.project.path() + PROGRAM_DATA_DIRECTORY.toString() + name;
}

// What closing the project left in its settings file
[[nodiscard]] inline auto savedProjectSettings(const Env& env) -> optional<ProjectSettings> {
    return readJsonFile<ProjectSettings>(programFile(env, PROJECT_SETTINGS_FILE.toString()));
}

// The settings of a project that was opened and closed before, without the ones the window works out itself
inline void writeProjectSettings(const Env& env, const std::function<void(ProjectSettings&)>& tweak) {
    ProjectSettings project;
    project.projectPath = env.project.path();
    project.sourceDirectory = SourceDirectory::LowercaseData;
    tweak(project);

    const auto json = glz::write_json(project);
    QVERIFY(json.has_value());
    writeFile(programFile(env, PROJECT_SETTINGS_FILE.toString()), QByteArray(json->data(), static_cast<qsizetype>(json->size())));
}

// The project is opened the way a project that was never opened is: from the list of recent projects, with no folder of
// the program in it
inline void forgetProgramFolder(const Env& env) {
    QVERIFY(QDir(env.project.path() + PROGRAM_DATA_DIRECTORY.toString()).removeRecursively());
}

inline void writeRecentSettings(const Env& env) {
    env.writeSettings([&env](Settings& settings) -> void {
        settings.core.projectPath.clear();
        settings.core.recentProjects = { env.project.path() };
    });
}

inline void openFromRecent(const MainWindow& window, const QString& folder) {
    for (QAction* const action : window.findChildren<QAction*>()) {
        if (action->text() == folder) {
            action->trigger();
            return;
        }
    }

    QFAIL("the project is not in the list of recent projects");
}

// Answers the question of the first read, which waits for the read menu
inline void answerFirstRead(const MainWindow& window, const bool accept) {
    QTimer::singleShot(100, &window, [&window, accept] -> void {
        auto* const menu = window.findChild<ReadMenu*>();

        if (accept) {
            emit menu->accepted();
        } else {
            emit menu->rejected();
        }
    });
}

// Closes the project when it goes out of scope. Destroying the window does not stop the tasks it started, which are
// still using its worker; closing the project waits for them.
class WindowGuard {
   public:
    explicit WindowGuard(const MainWindow& window) : window(window) {}

    WindowGuard(const WindowGuard&) = delete;
    auto operator=(const WindowGuard&) -> WindowGuard& = delete;

    ~WindowGuard() {
        QThreadPool::globalInstance()->waitForDone();
        // The continuations of the tasks run in the event loop
        QTest::qWait(50);
        window.findChild<QAction*>(u"actionCloseProject"_s)->trigger();
    }

   private:
    const MainWindow& window;
};
}  // namespace
