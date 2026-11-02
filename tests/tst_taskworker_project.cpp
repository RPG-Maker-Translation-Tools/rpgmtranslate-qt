#include "TaskWorker.hpp"

#include "ProjectSettings.hpp"
#include "TestFiles.hpp"

#include <QDir>
#include <QDirIterator>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
auto filenameOf(const string_view name) -> FilenameArray {
    FilenameArray filename{};
    std::copy_n(name.data(), name.size(), filename.data());
    return filename;
}

auto selectionOf(const FileFlags flags) -> Selected {
    Selected selected;
    selected.flags = flags;
    return selected;
}

// Reading, writing and purging take the files to leave out, not the ones to process
auto onlyFiles(const FileFlags flags) -> Selected {
    static constexpr FileFlags everything = (1 << 14) - 1;
    return selectionOf(static_cast<FileFlags>(everything & ~flags));
}

// What a failed FFI result says, or nothing for a good one. QVERIFY2 evaluates its message even when it passes, and
// `error()` of a value is not defined.
template <typename Result>
auto errorOf(const Result& result) -> QByteArray {
    return result.has_value() ? QByteArray() : ffitostr(result.error()).toString().toUtf8();
}

auto searchFlags(const u8 flags = 0) -> SearchFlags {
    return static_cast<SearchFlags>(flags);
}

// A project directory with the translation files of a game and a worker over it. No game data is needed: searching,
// replacing, linting and scripts all work on the translation files.
class Project {
   public:
    Project() {
        projectSettings.projectPath = dir.path();
        projectSettings.sourceDirectory = SourceDirectory::UppercaseData;
        projectSettings.engineType = EngineType::MVMZ;

        QDir().mkpath(projectSettings.translationPath());
        worker.init(&settings, &projectSettings, &mapSections);
    }

    [[nodiscard]] auto path(const QString& name) const -> QString {
        return projectSettings.translationPath() % u'/' % name % u".txt"_qsv;
    }

    void write(const QString& name, const QString& text) const { writeFile(path(name), text.toUtf8()); }

    [[nodiscard]] auto read(const QString& name) const -> QString { return QString::fromUtf8(readFile(path(name))); }

    // A task of `kind` that has been started. The ones asked for before are finished, as the worker allows one running
    // task of a kind.
    [[nodiscard]] auto task(const TaskKind kind) -> TaskToken {
        for (const TaskToken& previous : tasks) {
            worker.finishTask(previous);
        }
        tasks.clear();

        const auto started = worker.startTask(kind);

        if (!started.has_value()) {
            return nullptr;
        }

        tasks.push_back(*started);
        return *started;
    }

    // The data of a game with one actor
    void writeActorsData() const {
        QVERIFY(QDir().mkpath(projectSettings.sourcePath()));
        writeFile(
            projectSettings.sourcePath() + u"/Actors.json"_s,
            R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}])"
        );
    }

    // Reads the actors of the game into the translation files
    [[nodiscard]] auto readActors(const ReadMode mode = ReadMode::Default) -> ReadResult {
        return worker.read(
            projectSettings.sourcePath(),
            projectSettings.translationPath(),
            mode,
            EngineType::MVMZ,
            DuplicateMode::Allow,
            onlyFiles(FileFlags_Actors),
            static_cast<BaseFlags>(0),
            false,
            {},
            u"Test Game"_s,
            QString()
        );
    }

    QTemporaryDir dir;
    Settings settings;
    ProjectSettings projectSettings;
    HashMap<u16, QString> mapSections;
    TaskWorker worker;

   private:
    vector<TaskToken> tasks;
};

auto withText(QString text, const QString& from, const QString& with) -> QString {
    return text.replace(from, with);
}

// Comments, a line break, an empty translation and a second translation column
const QString SERDE_ACTORS =
    u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nLine1\\#Line2<#>Строка1\\#Строка2\nBob<#>\nTwo<#>Один<#>Два"_s;

const QString ACTORS = u"Harold<#>Гарольд\nAlice<#>Алиса\nLine1\\#Line2<#>Строка1\\#Строка2\nBob<#>"_s;
}  // namespace

class TestTaskWorkerProject final : public QObject {
    Q_OBJECT

   private slots:
    // Search
    void searchFindsTheSource() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"harold"_s,
            SearchLocation::Source,
            -1,
            searchFlags(),
            1
        );

        QCOMPARE(result.skippedCount, u32{ 0 });
        QCOMPARE(result.filenames, (vector<FilenameArray>{ filenameOf("actors") }));
        QCOMPARE(result.matches.size(), usize{ 1 });

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 1 });

        const CellMatch& cell = file.cells[0];
        QCOMPARE(cell.rowIndex(), 0);
        QCOMPARE(cell.colIndex(), 0);
        QCOMPARE(file.text(cell).toString(), u"Harold"_s);

        const auto spans = file.matches(cell);
        QCOMPARE(spans.size(), usize{ 1 });
        QCOMPARE(spans[0].get<"start">(), u32{ 0 });
        QCOMPARE(spans[0].get<"len">(), u32{ 6 });
        QVERIFY(!spans[0].get<"capture">());
    }

    void searchFindsTranslationsInEveryColumn() {
        Project project;
        project.write(u"actors"_s, u"A<#>first match<#>second match"_s);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"match"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 2 });
        QCOMPARE(file.cells[0].colIndex(), 1);
        QCOMPARE(file.cells[1].colIndex(), 2);
    }

    void searchColumnSelectsWhereToLook() {
        Project project;
        project.write(u"actors"_s, u"A<#>first match<#>second match"_s);

        const auto searchColumn = [&](const i8 column) -> usize {
            const auto result = project.worker.search(
                project.task(TaskKind::Search),
                SearchAction::Search,
                selectionOf(FileFlags_Actors),
                u"match"_s,
                SearchLocation::Translation,
                column,
                searchFlags(),
                1
            );

            return result.matches.empty() ? 0 : result.matches.begin()->second.cells.size();
        };

        QCOMPARE(searchColumn(1), usize{ 1 });
        QCOMPARE(searchColumn(2), usize{ 1 });
        // A row can have fewer columns than the one searched
        QCOMPARE(searchColumn(3), usize{ 0 });
        // Zero is the last column that holds a translation
        QCOMPARE(searchColumn(0), usize{ 1 });
    }

    void searchLocationsSelectSourceAndTranslation() {
        Project project;
        project.write(u"actors"_s, u"same<#>same"_s);

        const auto count = [&](const SearchLocation location) -> usize {
            const auto result = project.worker.search(
                project.task(TaskKind::Search),
                SearchAction::Search,
                selectionOf(FileFlags_Actors),
                u"same"_s,
                location,
                -1,
                searchFlags(),
                1
            );

            return result.matches.empty() ? 0 : result.matches.begin()->second.cells.size();
        };

        QCOMPARE(count(SearchLocation::Source), usize{ 1 });
        QCOMPARE(count(SearchLocation::Translation), usize{ 1 });
        QCOMPARE(count(static_cast<SearchLocation>(SearchLocation::Source | SearchLocation::Translation)), usize{ 2 });
    }

    void searchRespectsCaseAndWholeWords() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto count = [&](const QString& text, const u8 flags) -> usize {
            const auto result = project.worker.search(
                project.task(TaskKind::Search),
                SearchAction::Search,
                selectionOf(FileFlags_Actors),
                text,
                SearchLocation::Source,
                -1,
                searchFlags(flags),
                1
            );

            return result.matches.empty() ? 0 : result.matches.begin()->second.cells.size();
        };

        QCOMPARE(count(u"harold"_s, 0), usize{ 1 });
        QCOMPARE(count(u"harold"_s, SearchFlags::CaseSensitive), usize{ 0 });
        QCOMPARE(count(u"Harold"_s, SearchFlags::CaseSensitive), usize{ 1 });
        QCOMPARE(count(u"Ali"_s, 0), usize{ 1 });
        QCOMPARE(count(u"Ali"_s, SearchFlags::WholeWord), usize{ 0 });
        QCOMPARE(count(u"Alice"_s, SearchFlags::WholeWord), usize{ 1 });
        // Not a regular expression unless asked: the dot is a dot
        QCOMPARE(count(u"Har.ld"_s, 0), usize{ 0 });
        QCOMPARE(count(u"Har.ld"_s, SearchFlags::RegExp), usize{ 1 });
        // Cyrillic is case-folded too
        QCOMPARE(count(u"ГАРОЛЬД"_s, 0), usize{ 0 });
    }

    void searchWholeWordWrapsAlternations() {
        Project project;
        project.write(u"actors"_s, u"cat<#>x\ncatalog<#>x\ndog<#>x"_s);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"cat|dog"_s,
            SearchLocation::Source,
            -1,
            searchFlags(SearchFlags::RegExp | SearchFlags::WholeWord),
            1
        );

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 2 });
        QCOMPARE(file.cells[0].rowIndex(), 0);
        QCOMPARE(file.cells[1].rowIndex(), 2);
    }

    void searchRecordsCaptureGroups() {
        Project project;
        project.write(u"actors"_s, u"Harold<#>x"_s);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"(H)ar(x)?old"_s,
            SearchLocation::Source,
            -1,
            searchFlags(SearchFlags::RegExp),
            1
        );

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        const auto spans = file.matches(file.cells.at(0));

        QCOMPARE(spans.size(), usize{ 3 });
        QVERIFY(!spans[0].get<"capture">());
        QCOMPARE(spans[0].get<"len">(), u32{ 6 });
        QVERIFY(spans[1].get<"capture">());
        QCOMPARE(spans[1].get<"start">(), u32{ 0 });
        QCOMPARE(spans[1].get<"len">(), u32{ 1 });
        // A group that took no part is kept empty, so the numbers of the groups after it hold
        QVERIFY(spans[2].get<"capture">());
        QCOMPARE(spans[2].get<"len">(), u32{ 0 });
    }

    // Replacing an empty match would insert text between characters
    void searchIgnoresEmptyMatches() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"(?=H)"_s,
            SearchLocation::Source,
            -1,
            searchFlags(SearchFlags::RegExp),
            1
        );

        QVERIFY(result.matches.empty());
    }

    void searchSeesLineBreaksAsLineFeeds() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"Line1\nLine2"_s,
            SearchLocation::Source,
            -1,
            searchFlags(),
            1
        );

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 1 });
        QCOMPARE(file.cells[0].rowIndex(), 2);
        // The panel shows a line break as a separator character
        QCOMPARE(file.text(file.cells[0]).toString(), u"Line1 Line2"_s);
    }

    void searchLooksIntoCommentsOnRequest() {
        Project project;
        project.write(u"actors"_s, u"<!>FILE: actors<#>\n<!>BOOKMARK<#>remember the FILE\nA<#>b"_s);

        const auto count = [&](const QString& text, const SearchLocation location, const u8 flags) -> usize {
            const auto result = project.worker.search(
                project.task(TaskKind::Search),
                SearchAction::Search,
                selectionOf(FileFlags_Actors),
                text,
                location,
                -1,
                searchFlags(flags),
                1
            );

            return result.matches.empty() ? 0 : result.matches.begin()->second.cells.size();
        };

        QCOMPARE(count(u"FILE"_s, SearchLocation::Source, 0), usize{ 0 });
        QCOMPARE(count(u"FILE"_s, SearchLocation::Source, SearchFlags::Comment), usize{ 1 });
        // Bookmarks are text of the user: found without asking
        QCOMPARE(count(u"remember"_s, SearchLocation::Translation, 0), usize{ 1 });
    }

    void searchNamesTheEntriesOfResults() {
        Project project;
        project.settings.appearance.displayEntryNames = true;
        project.write(
            u"actors"_s,
            u"<!>ID<#>1\n<!>NAME<#>Hero<#>\nHarold<#>H\n<!>ID<#>2\n<!>NAME<#>Mage<#>\nAlice<#>A"_s
        );

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"l"_s,
            SearchLocation::Source,
            -1,
            searchFlags(),
            1
        );

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 2 });
        QCOMPARE(file.entry(file.cells[0]).toString(), u"Hero"_s);
        QCOMPARE(file.entry(file.cells[1]).toString(), u"Mage"_s);
    }

    void searchSkipsFilesThatCannotBeRead() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors | FileFlags_Items),
            u"Harold"_s,
            SearchLocation::Source,
            -1,
            searchFlags(),
            2
        );

        QCOMPARE(result.skippedCount, u32{ 1 });
        // The skipped ones are moved to the front
        QCOMPARE(result.filenames.front(), filenameOf("items"));
        QCOMPARE(result.matches.size(), usize{ 1 });
    }

    void searchStopsWhenTheTaskIsInterrupted() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const TaskToken task = project.task(TaskKind::Search);
        task->interrupt();

        const auto result = project.worker.search(
            task,
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"Harold"_s,
            SearchLocation::Source,
            -1,
            searchFlags(),
            1
        );

        QVERIFY(result.matches.empty());
    }

    void searchReadsTheMapsHeldInMemory() {
        Project project;
        project.mapSections.emplace(1, u"Hello<#>Salut\nBye<#>"_s);

        Selected selected;
        selected.mapCount = 1;
        selected.validIndices[1] = true;
        selected.mapIndices[0] = true;

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selected,
            u"salut"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        QCOMPARE(result.filenames, (vector<FilenameArray>{ filenameOf("map1") }));
        QCOMPARE(result.matches.size(), usize{ 1 });
    }

    // Replace
    void replaceChangesTheTranslation() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"Алиса"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"Элис"_s);

        QVERIFY(replaced.skipped.empty());
        QCOMPARE(replaced.deltas.size(), usize{ 1 });
        QCOMPARE(replaced.deltas[0].filename, filenameOf("actors"));
        QCOMPARE(replaced.deltas[0].delta, 0);
        QCOMPARE(project.read(u"actors"_s), withText(ACTORS, u"Алиса"_s, u"Элис"_s));
    }

    void replacingWithNothingCountsAsUntranslating() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"Алиса"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, QString());

        QCOMPARE(replaced.deltas.at(0).delta, -1);
        QVERIFY(project.read(u"actors"_s).contains(u"\nAlice<#>\n"_qsv));
    }

    void replaceKeepsLineBreaksAndOtherRows() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"Строка1"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        // The replacement has a line break of its own
        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"Новая\nстрока"_s);

        QVERIFY(replaced.skipped.empty());
        QCOMPARE(
            project.read(u"actors"_s),
            u"Harold<#>Гарольд\nAlice<#>Алиса\nLine1\\#Line2<#>Новая\\#строка\\#Строка2\nBob<#>"_s
        );
    }

    // A group that did not take part is empty, also the last one
    void replaceTreatsAnOptionalGroupThatDidNotMatchAsEmpty() {
        Project project;
        project.write(u"actors"_s, u"x<#>ab\ny<#>a"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"(a)(b)?"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(SearchFlags::RegExp),
            1
        );

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"[\\1|\\2]"_s);

        QCOMPARE(project.read(u"actors"_s), u"x<#>[a|b]\ny<#>[a|]"_s);
    }

    void replaceUsesBackReferences() {
        Project project;
        project.write(u"actors"_s, u"x<#>John Smith\ny<#>Mary Jones"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"(\\w+) (\\w+)"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(SearchFlags::RegExp),
            1
        );

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"\\2, \\1 (\\+) [\\\\]"_s);

        QVERIFY(replaced.skipped.empty());
        QCOMPARE(project.read(u"actors"_s), u"x<#>Smith, John (Smith) [\\]\ny<#>Jones, Mary (Jones) [\\]"_s);
    }

    void replaceAppliesSeveralMatchesOfACell() {
        Project project;
        project.write(u"actors"_s, u"x<#>ab ab ab"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"ab"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"longer"_s);

        QCOMPARE(project.read(u"actors"_s), u"x<#>longer longer longer"_s);
    }

    void replaceNeverTouchesComments() {
        Project project;
        const QString text = u"<!>FILE: actors<#>\nA<#>FILE"_s;
        project.write(u"actors"_s, text);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"FILE"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(SearchFlags::Comment),
            1
        );

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"X"_s);

        QCOMPARE(project.read(u"actors"_s), u"<!>FILE: actors<#>\nA<#>X"_s);
    }

    void replaceInMapsChangesTheSectionNotAFile() {
        Project project;
        project.mapSections.emplace(1, u"Hello<#>Salut\nBye<#>"_s);

        Selected selected;
        selected.mapCount = 1;
        selected.validIndices[1] = true;
        selected.mapIndices[0] = true;

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selected,
            u"Salut"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"Allo"_s);

        QCOMPARE(project.mapSections[1], u"Hello<#>Allo\nBye<#>"_s);
        QVERIFY(!QFile::exists(project.path(u"map1"_s)));
    }

    void replaceReportsFilesThatCannotBeChanged() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"Алиса"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        // The file is gone by the time it is replaced in
        QVERIFY(QFile::remove(project.path(u"actors"_s)));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"x"_s);

        QCOMPARE(replaced.skipped, (vector<FilenameArray>{ filenameOf("actors") }));
        QVERIFY(replaced.deltas.empty());
    }

    void putFillsEmptyCells() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Put,
            selectionOf(FileFlags_Actors),
            u"Bob"_s,
            SearchLocation::Source,
            1,
            searchFlags(SearchFlags::Put),
            1
        );

        const FileMatches& file = found.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 1 });
        QCOMPARE(file.cells[0].rowIndex(), 3);
        QCOMPARE(file.cells[0].colIndex(), 1);

        const auto put = project.worker.replace(task, found.matches, SearchAction::Put, u"Боб"_s);

        QCOMPARE(put.deltas.at(0).delta, 1);
        QCOMPARE(project.read(u"actors"_s), withText(ACTORS, u"Bob<#>"_s, u"Bob<#>Боб"_s));
    }

    // Put is for a whole cell: a part of one is no result
    void putNeedsTheWholeSource() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto found = project.worker.search(
            project.task(TaskKind::Replace),
            SearchAction::Put,
            selectionOf(FileFlags_Actors),
            u"Har"_s,
            SearchLocation::Source,
            1,
            searchFlags(SearchFlags::Put),
            1
        );

        QVERIFY(found.matches.empty());
    }

    void putAddsAColumnTheRowLacks() {
        Project project;
        project.projectSettings.columns = { ColumnInfo{ u"Source"_s, 100 },
                                            ColumnInfo{ u"One"_s, 100 },
                                            ColumnInfo{ u"Two"_s, 100 } };
        project.write(u"actors"_s, u"Harold<#>one"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Put,
            selectionOf(FileFlags_Actors),
            u"Harold"_s,
            SearchLocation::Source,
            2,
            searchFlags(SearchFlags::Put),
            1
        );

        (void)project.worker.replace(task, found.matches, SearchAction::Put, u"two"_s);

        QCOMPARE(project.read(u"actors"_s), u"Harold<#>one<#>two"_s);
    }

    // ReplaceSingle
    void replaceSingleChangesOneCell() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const array<TextMatch, 1> matches = { TextMatch(0, 5, false) };

        const auto result = project.worker.replaceSingle(
            u"Элис"_s,
            SearchPanelDock::Action::Replace,
            u"actors"_s,
            1,
            1,
            matches,
            u"Алиса"_s
        );

        QVERIFY(result.has_value());
        QCOMPARE(result->text, u"Элис"_s);
        QCOMPARE(result->translatedDelta, 0);
        QCOMPARE(result->matches.size(), usize{ 1 });
        QCOMPARE(result->matches[0].get<"len">(), u32{ 4 });
        QCOMPARE(project.read(u"actors"_s), withText(ACTORS, u"Алиса"_s, u"Элис"_s));
    }

    void replaceSingleRefusesACellThatChanged() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const array<TextMatch, 1> matches = { TextMatch(0, 5, false) };

        // The row is not what was searched
        const auto changed = project.worker.replaceSingle(
            u"x"_s,
            SearchPanelDock::Action::Replace,
            u"actors"_s,
            1,
            1,
            matches,
            u"Совсем другое"_s
        );

        QVERIFY(!changed.has_value());
        QVERIFY(changed.error().stale);

        // The row is gone
        const auto gone = project.worker.replaceSingle(
            u"x"_s,
            SearchPanelDock::Action::Replace,
            u"actors"_s,
            40,
            1,
            matches,
            u"Алиса"_s
        );

        QVERIFY(!gone.has_value());
        QVERIFY(gone.error().stale);

        // The positions do not fit the text
        const array<TextMatch, 1> outside = { TextMatch(3, 50, false) };
        const auto misfit = project.worker.replaceSingle(
            u"x"_s,
            SearchPanelDock::Action::Replace,
            u"actors"_s,
            1,
            1,
            outside,
            u"Алиса"_s
        );

        QVERIFY(!misfit.has_value());
        QVERIFY(misfit.error().stale);

        QCOMPARE(project.read(u"actors"_s), ACTORS);
    }

    void replaceSingleRefusesComments() {
        Project project;
        project.write(u"actors"_s, u"<!>FILE: actors<#>x\nA<#>b"_s);

        const auto result = project.worker.replaceSingle(
            u"y"_s,
            SearchPanelDock::Action::Replace,
            u"actors"_s,
            0,
            1,
            span<const TextMatch>(),
            u"x"_s
        );

        QVERIFY(!result.has_value());
        QVERIFY(result.error().stale);
    }

    void replaceSingleOfAMissingFileIsNotStale() {
        Project project;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));

        const auto result = project.worker.replaceSingle(
            u"y"_s,
            SearchPanelDock::Action::Put,
            u"actors"_s,
            0,
            1,
            span<const TextMatch>(),
            QString()
        );

        QVERIFY(!result.has_value());
        QVERIFY(!result.error().stale);
        QCOMPARE(result.error().filename, u"actors"_s);
    }

    void replaceSinglePutCountsTranslations() {
        Project project;
        project.write(u"actors"_s, ACTORS);

        const auto result = project.worker.replaceSingle(
            u"Боб\nBob"_s,
            SearchPanelDock::Action::Put,
            u"actors"_s,
            3,
            1,
            span<const TextMatch>(),
            QString()
        );

        QVERIFY(result.has_value());
        QCOMPARE(result->translatedDelta, 1);
        QCOMPARE(result->text, u"Боб Bob"_s);
        QVERIFY(project.read(u"actors"_s).endsWith(u"Bob<#>Боб\\#Bob"_qsv));
    }

    // Reading and writing the game, through the Rust core
    void readExtractsTheTextOfTheGame() {
        Project project;
        project.writeActorsData();

        const auto read = project.readActors();

        QVERIFY2(read.has_value(), errorOf(read).constData());
        QCOMPARE(project.read(u"actors"_s), u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>"_s);
        // The hashes come back as records of a name and a hash, after their count
        QCOMPARE(read->len, u32{ 4 + 16 + 8 });
    }

    void writePutsTheTranslationIntoTheGame() {
        Project project;
        project.writeActorsData();
        QVERIFY(project.readActors().has_value());

        project.write(u"actors"_s, u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>Храбрый"_s);

        const auto written = project.worker.write(onlyFiles(FileFlags_Actors));

        QVERIFY2(written.has_value(), errorOf(written).constData());
        QVERIFY(*written >= 0.0F);

        const QByteArray output = readFile(project.projectSettings.outputPath() + u"/data/Actors.json"_s);
        QVERIFY(!output.isEmpty());
        QVERIFY(output.contains("Гарольд"));
        QVERIFY(output.contains("Храбрый"));
        // What has no translation is kept
        QVERIFY(output.contains("A hero"));
        QVERIFY(!output.contains("Harold"));

        // The source is left as it was
        QVERIFY(readFile(project.projectSettings.sourcePath() + u"/Actors.json"_s).contains("Harold"));
    }

    void readAppendsWithoutLosingTranslations() {
        Project project;
        project.writeActorsData();
        QVERIFY(project.readActors().has_value());
        project.write(u"actors"_s, u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>"_s);

        // The game gets a new actor
        writeFile(
            project.projectSettings.sourcePath() + u"/Actors.json"_s,
            R"([null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"},{"id":2,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Alice","nickname":"","note":"","profile":""}])"
        );

        const auto read = project.readActors(ReadMode::AppendForce);

        QVERIFY2(read.has_value(), errorOf(read).constData());

        const QString text = project.read(u"actors"_s);
        QVERIFY(text.contains(u"Harold<#>Гарольд"_qsv));
        QVERIFY(text.contains(u"Alice<#>"_qsv));
    }

    void readAndWriteReportTheFilesTheyCannotUse() {
        Project project;

        // There is no game here
        const auto read = project.readActors();
        QVERIFY(!read.has_value());
        QVERIFY(!ffitostr(read.error()).isEmpty());

        const auto written = project.worker.write(onlyFiles(FileFlags_Actors));
        QVERIFY(!written.has_value());
        QVERIFY(!ffitostr(written.error()).isEmpty());
    }

    void purgeDropsEntriesNeverFilledIn() {
        Project project;
        project.writeActorsData();
        QVERIFY(project.readActors().has_value());
        project.write(
            u"actors"_s,
            u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>\nGhost<#>Призрак"_s
        );

        const auto purged = project.worker.purge(onlyFiles(FileFlags_Actors), false);

        QVERIFY2(purged.has_value(), errorOf(purged).constData());

        // Entries that were never filled in are dropped
        const QString text = project.read(u"actors"_s);
        QVERIFY(text.contains(u"Harold<#>Гарольд"_qsv));
        QVERIFY(!text.contains(u"The Brave"_qsv));
    }

    // Serde: the translation files as CSV, XLSX, XML, JSON and YAML
    void serdeRoundTripsEveryFormat_data() {
        QTest::addColumn<SerdeFormat>("format");
        QTest::addColumn<QString>("extension");

        QTest::newRow("csv") << SerdeFormat::Csv << u".csv"_s;
        QTest::newRow("xlsx") << SerdeFormat::Xlsx << u".xlsx"_s;
        QTest::newRow("xml") << SerdeFormat::Xml << u".xml"_s;
        QTest::newRow("json") << SerdeFormat::Json << u".json"_s;
        QTest::newRow("yaml") << SerdeFormat::Yaml << u".yaml"_s;
    }

    void serdeRoundTripsEveryFormat() {
        QFETCH(const SerdeFormat, format);
        QFETCH(const QString, extension);

        Project project;
        const QString text = SERDE_ACTORS;
        project.write(u"actors"_s, text);

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("actors") };
        const auto exported = project.worker.serdeExport(project.task(TaskKind::SerdeExport), filenames, exportDir.path(), format);

        QVERIFY2(exported.has_value(), errorOf(exported).constData());
        QCOMPARE(exported->skippedCount, u32{ 0 });
        QVERIFY(QFile::exists(exportDir.filePath(u"actors"_s + extension)));
        QVERIFY(QFileInfo(exportDir.filePath(u"actors"_s + extension)).size() > 0);

        // The file is replaced by what the export held
        project.write(u"actors"_s, u"Other<#>text"_s);

        filenames = { filenameOf("actors") };
        const auto imported = project.worker.serdeImport(project.task(TaskKind::SerdeImport), filenames, exportDir.path(), format);

        QVERIFY2(imported.has_value(), errorOf(imported).constData());
        QCOMPARE(imported->skippedCount, u32{ 0 });
        QCOMPARE(project.read(u"actors"_s), text);
    }

    void serdeImportTakesHandEditedFiles() {
        Project project;
        project.write(u"actors"_s, SERDE_ACTORS);

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("actors") };
        QVERIFY(project.worker.serdeExport(project.task(TaskKind::SerdeExport), filenames, exportDir.path(), SerdeFormat::Json).has_value());

        const QString path = exportDir.filePath(u"actors.json"_s);
        QByteArray json = readFile(path);
        QVERIFY(json.contains("Гарольд"));
        json.replace("Гарольд", "Хэл");
        writeFile(path, json);

        filenames = { filenameOf("actors") };
        QVERIFY(project.worker.serdeImport(project.task(TaskKind::SerdeImport), filenames, exportDir.path(), SerdeFormat::Json).has_value());

        QCOMPARE(project.read(u"actors"_s), withText(SERDE_ACTORS, u"Гарольд"_s, u"Хэл"_s));
    }

    void serdeExportsWithLineFeedsOnly() {
        Project project;
        project.write(u"actors"_s, QString(SERDE_ACTORS).replace(u'\n', u"\r\n"_s));

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("actors") };
        QVERIFY(project.worker.serdeExport(project.task(TaskKind::SerdeExport), filenames, exportDir.path(), SerdeFormat::Json).has_value());

        // A `\r` would show in the JSON as an escape of its own
        QVERIFY(!readFile(exportDir.filePath(u"actors.json"_s)).contains("\\r"));

        project.write(u"actors"_s, u"Other<#>text"_s);
        filenames = { filenameOf("actors") };
        QVERIFY(project.worker.serdeImport(project.task(TaskKind::SerdeImport), filenames, exportDir.path(), SerdeFormat::Json).has_value());
        QCOMPARE(project.read(u"actors"_s), SERDE_ACTORS);
    }

    void serdeExportSkipsFilesItCannotRead() {
        Project project;
        project.write(u"actors"_s, SERDE_ACTORS);

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("actors"), filenameOf("items") };
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));
        const auto exported = project.worker.serdeExport(project.task(TaskKind::SerdeExport), filenames, exportDir.path(), SerdeFormat::Csv);

        QVERIFY(exported.has_value());
        QCOMPARE(exported->skippedCount, u32{ 1 });
        // The skipped ones come first
        QCOMPARE(exported->filenames.front(), filenameOf("items"));
        QVERIFY(QFile::exists(exportDir.filePath(u"actors.csv"_s)));
        QVERIFY(!QFile::exists(exportDir.filePath(u"items.csv"_s)));
    }

    void serdeExportToAMissingFolderSkipsTheFile() {
        Project project;
        project.write(u"actors"_s, SERDE_ACTORS);

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("actors") };
        const auto exported = project.worker.serdeExport(
            project.task(TaskKind::SerdeExport),
            filenames,
            exportDir.filePath(u"no/such/folder"_s),
            SerdeFormat::Json
        );

        QVERIFY(exported.has_value());
        QCOMPARE(exported->skippedCount, u32{ 1 });
    }

    void serdeImportSkipsMissingAndBrokenFiles_data() {
        QTest::addColumn<SerdeFormat>("format");
        QTest::addColumn<QByteArray>("content");

        QTest::newRow("csv without the columns") << SerdeFormat::Csv << QByteArray("not,a,translation\n1,2,3\n");
        QTest::newRow("json that is not json") << SerdeFormat::Json << QByteArray("{ this is not json");
        QTest::newRow("json of another shape") << SerdeFormat::Json << QByteArray(R"({"unexpected":true})");
        QTest::newRow("xml that is not xml") << SerdeFormat::Xml << QByteArray("<open><unclosed>");
        QTest::newRow("xlsx that is not a workbook") << SerdeFormat::Xlsx << QByteArray("PK nope");
        QTest::newRow("yaml that is not a list") << SerdeFormat::Yaml << QByteArray("key: [unclosed");
    }

    void serdeImportSkipsMissingAndBrokenFiles() {
        QFETCH(const SerdeFormat, format);
        QFETCH(const QByteArray, content);

        Project project;
        project.write(u"actors"_s, SERDE_ACTORS);
        project.write(u"items"_s, u"I<#>i"_s);

        QTemporaryDir inputDir;
        QVERIFY(inputDir.isValid());
        writeFile(inputDir.filePath(u"actors"_s + QString::fromUtf8(QByteArray(serdeFormatExtension(format).toUtf8()))), content);
        // There is no file for `items` at all

        vector<FilenameArray> filenames{ filenameOf("actors"), filenameOf("items") };
        const auto imported = project.worker.serdeImport(project.task(TaskKind::SerdeImport), filenames, inputDir.path(), format);

        QVERIFY(imported.has_value());
        QCOMPARE(imported->skippedCount, u32{ 2 });
        // Nothing was replaced
        QCOMPARE(project.read(u"actors"_s), SERDE_ACTORS);
        QCOMPARE(project.read(u"items"_s), u"I<#>i"_s);
    }

    void serdeWorksOnTheMapsInMemory() {
        Project project;
        project.mapSections.emplace(1, u"Hello<#>Salut\nBye<#>"_s);

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("map1") };
        QVERIFY(project.worker.serdeExport(project.task(TaskKind::SerdeExport), filenames, exportDir.path(), SerdeFormat::Json).has_value());
        QVERIFY(QFile::exists(exportDir.filePath(u"map1.json"_s)));

        project.mapSections[1] = u"Changed<#>x"_s;

        filenames = { filenameOf("map1") };
        const auto imported = project.worker.serdeImport(project.task(TaskKind::SerdeImport), filenames, exportDir.path(), SerdeFormat::Json);

        QVERIFY(imported.has_value());
        QCOMPARE(imported->skippedCount, u32{ 0 });
        QCOMPARE(project.mapSections[1], u"Hello<#>Salut\nBye<#>"_s);
        QVERIFY(!QFile::exists(project.path(u"map1"_s)));
    }

    void serdeStopsWhenInterrupted() {
        Project project;
        project.write(u"actors"_s, SERDE_ACTORS);

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        const TaskToken task = project.task(TaskKind::SerdeExport);
        task->interrupt();

        vector<FilenameArray> filenames{ filenameOf("actors") };
        const auto exported = project.worker.serdeExport(task, filenames, exportDir.path(), SerdeFormat::Json);

        QVERIFY(exported.has_value());
        QVERIFY(!QFile::exists(exportDir.filePath(u"actors.json"_s)));
    }

    // Statistics
    void statisticsCountSourceAndTranslation() {
        Project project;
        project.write(u"actors"_s, u"<!>FILE: actors<#>\nHello world<#>Bonjour monde\nGoodbye<#>"_s);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));
        const auto result = project.worker.projectStatistics(
            project.task(TaskKind::Statistics),
            { filenameOf("actors"), filenameOf("items") }
        );

        QCOMPARE(result.files.size(), usize{ 1 });
        QCOMPARE(result.skipped, (vector<FilenameArray>{ filenameOf("items") }));

        const FileStatistics& statistics = result.files[0];
        QCOMPARE(statistics.filename, filenameOf("actors"));
        QCOMPARE(statistics.source.words, u32{ 3 });
        QCOMPARE(statistics.source.characters_no_whitespace, u32{ 17 });
        QCOMPARE(statistics.translation.words, u32{ 2 });
        QCOMPARE(statistics.translation.characters_no_whitespace, u32{ 12 });
    }

    // Batch scripts
    void lineScriptRewritesTheTranslations() {
        Project project;
        project.write(u"actors"_s, u"a<#>one<#>two\nb<#>three"_s);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"upper"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => s.toUpperCase()); }"_s };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(result->processedCount, u32{ 1 });
        QCOMPARE(result->skippedCount, u32{ 0 });
        QCOMPARE(project.read(u"actors"_s), u"a<#>ONE<#>TWO\nb<#>THREE"_s);
    }

    void lineScriptKeepsCellsItReturnsNothingFor() {
        Project project;
        project.write(u"actors"_s, u"a<#>one<#>two"_s);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"partial"_s,
                               .source = u"function rpgmLineCallback() { return [null, 'changed']; }"_s };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(project.read(u"actors"_s), u"a<#>one<#>changed"_s);
    }

    void lineScriptSkipsRowsWithoutTheColumn() {
        Project project;
        project.write(u"actors"_s, u"a<#>one\nb<#>one<#>two"_s);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"column"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => s + '!'); }"_s };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            2,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(project.read(u"actors"_s), u"a<#>one\nb<#>one!<#>two!"_s);
    }

    void lineScriptNeverRewritesComments() {
        Project project;
        const QString text = u"<!>FILE: actors<#>x\na<#>one"_s;
        project.write(u"actors"_s, text);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"all"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => 'changed'); }"_s };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(project.read(u"actors"_s), u"<!>FILE: actors<#>x\na<#>changed"_s);
    }

    void fileScriptSeesTheRows() {
        Project project;
        project.write(u"actors"_s, u"<!>FILE: actors<#>x\na<#>one\nb<#>two"_s);

        const JSScript script{
            .action = BatchAction::Custom,
            .name = u"file"_s,
            .source = u"function rpgmFileCallback(name, rows) { return rows.map(r => r.isComment ? ['no'] : [name + ':' + r.source + ':' + r.lineNumber]); }"_s
        };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(project.read(u"actors"_s), u"<!>FILE: actors<#>x\na<#>actors:a:1\nb<#>actors:b:2"_s);
    }

    void scriptLogIsReturned() {
        Project project;
        project.write(u"actors"_s, u"a<#>one"_s);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"log"_s,
                               .source = u"function rpgmLineCallback(src, tr) { rpgm.log('row ' + src); }"_s };

        QTest::ignoreMessage(QtInfoMsg, QRegularExpression(u"\\[script\\] row a"_s));
        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(result->log, QStringList{ u"row a"_s });
    }

    void scriptErrorsNameTheFileAndLine() {
        Project project;
        const QString text = u"a<#>one\nb<#>two\nc<#>three"_s;
        project.write(u"actors"_s, text);

        const JSScript script{
            .action = BatchAction::Custom,
            .name = u"throws"_s,
            .source = u"function rpgmLineCallback(src, tr) { if (src == 'b') throw new Error('bad row'); return tr.map(s => s.toUpperCase()); }"_s
        };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(!result.has_value());
        QVERIFY(result.error().message.contains(u"bad row"_qsv));
        QCOMPARE(result.error().filename, filenameOf("actors"));
        QCOMPARE(result.error().lineNumber, u32{ 2 });
        // A file the script did not finish is left as it was
        QCOMPARE(project.read(u"actors"_s), text);
    }

    void scriptsMustDefineExactlyOneCallback_data() {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("expected");

        QTest::newRow("none") << u"var x = 1;"_s << u"Script must define function"_s;
        QTest::newRow("both") << u"function rpgmLineCallback() {} function rpgmFileCallback() {}"_s
                              << u"only one of"_s;
        QTest::newRow("syntax error") << u"function ("_s << u"SyntaxError"_s;
    }

    void scriptsMustDefineExactlyOneCallback() {
        QFETCH(const QString, source);
        QFETCH(const QString, expected);

        Project project;
        project.write(u"actors"_s, u"a<#>one"_s);

        const JSScript script{ .action = BatchAction::Custom, .name = u"bad"_s, .source = source };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(!result.has_value());
        QVERIFY2(result.error().message.contains(expected), qPrintable(result.error().message));
        QCOMPARE(project.read(u"actors"_s), u"a<#>one"_s);
    }

    void scriptSkipsFilesThatAreMissing() {
        Project project;
        project.write(u"actors"_s, u"a<#>one"_s);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"noop"_s,
                               .source = u"function rpgmLineCallback() {}"_s };

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));
        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors | FileFlags_Items),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(result->skippedCount, u32{ 1 });
        QCOMPARE(result->processedCount, u32{ 1 });
    }

    void interruptedScriptLeavesFilesAlone() {
        Project project;
        project.write(u"actors"_s, u"a<#>one"_s);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"upper"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => s.toUpperCase()); }"_s };

        const TaskToken task = project.task(TaskKind::BatchScript);
        task->interrupt();

        const auto result = project.worker.performBatchAction(
            task,
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        // The script itself is stopped, which the window does not report as a failure of the script
        QVERIFY(!result.has_value());
        QVERIFY(result.error().message.contains(u"interrupted"_qsv));
        QCOMPARE(project.read(u"actors"_s), u"a<#>one"_s);
    }

    // Linting
    void lintFindsLeadingTrailingAndContiguousWhitespace() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(
            LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace | LintFlags::ContiguousWhitespace
        );

        const auto outcome = project.worker.lint(
            u"actors"_s,
            7,
            u"source"_qsv,
            u"  two  spaces ok   "_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            false
        );

        // The lead and the trail, each also a run of whitespace of its own, and the double space inside
        QCOMPARE(outcome.rows.size(), usize{ 5 });

        const auto startsOf = [&] -> QList<u32> {
            QList<u32> starts;
            for (const LintRow& row : outcome.rows) {
                starts.append(row.translationHighlights.at(0).start);
            }
            ranges::sort(starts);
            return starts;
        }();

        QCOMPARE(startsOf, (QList<u32>{ 0, 0, 5, 16, 16 }));

        for (const LintRow& row : outcome.rows) {
            QCOMPARE(row.filename, u"actors"_s);
            QCOMPARE(row.lineNumber, u32{ 7 });
            QCOMPARE(row.sourceText, u"-"_s);
            QCOMPARE(row.translationText, u"  two  spaces ok   "_s);
        }
    }

    void lintFlagsSwitchChecksOff() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);

        const auto outcome = project.worker.lint(
            u"actors"_s,
            1,
            u"s"_qsv,
            u"  (unclosed  "_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            false
        );

        QVERIFY(outcome.rows.empty());
        QVERIFY(outcome.charStates.empty());
    }

    // The trail of a text that is all whitespace is its lead already
    void lintDoesNotReportWhitespaceTwice() {
        Project project;
        project.settings.translation.lintFlags =
            static_cast<LintFlags>(LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace);

        const auto outcome =
            project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"   "_qsv, PreparedGlossary{}, CustomLintMatcher{}, false);

        QCOMPARE(outcome.rows.size(), usize{ 1 });
    }

    void lintCountsConfiguredWhitespace() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::LeadingWhitespace);

        const auto without =
            project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"　x"_qsv, PreparedGlossary{}, CustomLintMatcher{}, false);
        QVERIFY(without.rows.empty());

        project.settings.translation.whitespaceCharacters = { 0x3000 };
        const auto with =
            project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"　x"_qsv, PreparedGlossary{}, CustomLintMatcher{}, false);
        QCOMPARE(with.rows.size(), usize{ 1 });
    }

    void lintFindsUnclosedPunctuation_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QList<u32>>("positions");

        QTest::newRow("balanced") << u"(a) [b] {c} \"d\" «e» “f” 「g」"_s << QList<u32>{};
        QTest::newRow("unclosed bracket") << u"x (a"_s << QList<u32>{ 2 };
        QTest::newRow("stray closer") << u"a) b"_s << QList<u32>{ 1 };
        QTest::newRow("wrong closer") << u"(a]"_s << QList<u32>{ 0, 2 };
        QTest::newRow("unclosed quote") << u"say \"hi"_s << QList<u32>{ 4 };
        QTest::newRow("apostrophe inside a word") << u"it's you're"_s << QList<u32>{};
        QTest::newRow("apostrophe after plural") << u"the dogs' bowl"_s << QList<u32>{};
        QTest::newRow("single quotes") << u"'quoted'"_s << QList<u32>{};
        QTest::newRow("nested") << u"(a [b {c}])"_s << QList<u32>{};
        QTest::newRow("cjk") << u"「こんにちは"_s << QList<u32>{ 0 };
    }

    void lintFindsUnclosedPunctuation() {
        QFETCH(const QString, text);
        QFETCH(const QList<u32>, positions);

        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::UnclosedPunctuation);

        const auto outcome =
            project.worker.lint(u"actors"_s, 1, u"s"_qsv, text, PreparedGlossary{}, CustomLintMatcher{}, false);

        QList<u32> found;
        for (const LintRow& row : outcome.rows) {
            found.append(row.translationHighlights.at(0).start);
            QCOMPARE(row.translationHighlights.at(0).len, u32{ 1 });
        }
        ranges::sort(found);

        QCOMPARE(found, positions);
    }

    void lintComparesTags() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TagMismatch);

        const auto lint = [&](const QStringView source, const QStringView translation) -> usize {
            return project.worker.lint(u"actors"_s, 1, source, translation, PreparedGlossary{}, CustomLintMatcher{}, false)
                .rows.size();
        };

        QCOMPARE(lint(u"\\C[2]Hello\\C[0]"_qsv, u"\\C[2]Salut\\C[0]"_qsv), usize{ 0 });
        QCOMPARE(lint(u"\\C[2]Hello\\C[0]"_qsv, u"Salut"_qsv), usize{ 1 });
        QCOMPARE(lint(u"\\C[2]Hello\\C[0]"_qsv, u"\\C[3]Salut\\C[0]"_qsv), usize{ 1 });
        QCOMPARE(lint(u"plain"_qsv, u"simple"_qsv), usize{ 0 });

        const auto outcome = project.worker.lint(
            u"actors"_s,
            1,
            u"\\C[2]Hello"_qsv,
            u"Salut"_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            false
        );
        QVERIFY(outcome.rows.at(0).info.contains(u"Tags do not match"_qsv));
    }

    void lintHighlightsEveryCharacterOfTheTranslation() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::LeadingWhitespace);

        const auto outcome = project.worker.lint(
            u"actors"_s,
            1,
            u"s"_qsv,
            u"  ab"_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            true
        );

        QCOMPARE(outcome.charStates.size(), usize{ 4 });
        QVERIFY(outcome.charStates[0].background.has_value());
        QVERIFY(outcome.charStates[1].background.has_value());
        QVERIFY(!outcome.charStates[2].background.has_value());
        QCOMPARE(outcome.charStates[0].entries.size(), qsizetype{ 1 });
        QVERIFY(outcome.charStates[2].entries.isEmpty());
    }

    void lintAppliesCustomLints() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        project.settings.translation.customLints = { CustomLint{ .sequence = u"todo"_s,
                                                                 .tooltip = u"Finish it"_s,
                                                                 .caseInsensitive = true } };

        const CustomLintMatcher matcher = project.worker.customLintMatcher();
        const auto outcome = project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"a TODO b"_qsv, PreparedGlossary{}, matcher, true);

        QCOMPARE(outcome.charStates.size(), usize{ 8 });
        QVERIFY(outcome.charStates[0].entries.isEmpty());

        for (const usize idx : { 2, 3, 4, 5 }) {
            QCOMPARE(outcome.charStates[idx].entries.size(), qsizetype{ 1 });
            QCOMPARE(outcome.charStates[idx].entries[0].type, LintType::Custom);
            QCOMPARE(outcome.charStates[idx].entries[0].extra.toString(), u"Finish it"_s);
            QCOMPARE(outcome.charStates[idx].entries[0].captured, u"TODO"_s);
        }

        QVERIFY(outcome.charStates[6].entries.isEmpty());
    }

    void lintFilesChecksTranslatedRowsOnly() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TrailingWhitespace);
        project.write(
            u"actors"_s,
            u"<!>FILE: actors<#> \nA<#>fine\nB<#>bad \nC<#>\n<!>IN-GAME DISPLAYED NAME: M<#>map name \nD<#>also bad  "_s
        );
        project.write(u"items"_s, u"I<#>good"_s);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to open file"_s));
        const auto result = project.worker.lintFiles(
            project.task(TaskKind::GlobalLint),
            selectionOf(FileFlags_Actors | FileFlags_Items | FileFlags_Weapons),
            Glossary{}
        );

        QCOMPARE(result.skippedCount, u32{ 1 });
        QCOMPARE(result.rows.size(), usize{ 3 });

        QList<u32> lines;
        for (const LintRow& row : result.rows) {
            QCOMPARE(row.filename, u"actors"_s);
            lines.append(row.lineNumber);
        }
        ranges::sort(lines);

        // Line numbers count the lines of the file from 1: the comment is line 1, the empty translation is skipped
        QCOMPARE(lines, (QList<u32>{ 3, 5, 6 }));
    }

    void lintFilesStopsWhenInterrupted() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TrailingWhitespace);
        project.write(u"actors"_s, u"B<#>bad "_s);

        const TaskToken task = project.task(TaskKind::GlobalLint);
        task->interrupt();

        const auto result = project.worker.lintFiles(task, selectionOf(FileFlags_Actors), Glossary{});

        QVERIFY(result.rows.empty());
    }
};

QTEST_MAIN(TestTaskWorkerProject)
#include "tst_taskworker_project.moc"
