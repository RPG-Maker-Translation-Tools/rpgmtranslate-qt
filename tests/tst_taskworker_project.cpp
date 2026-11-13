#include "Permissions.hpp"
#include "TaskWorker.hpp"

#include "HttpStub.hpp"
#include "ProjectSettings.hpp"
#include "ScriptHost.hpp"
#include "TestFiles.hpp"

#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QSignalSpy>
#include <QtConcurrent>
#include <QTemporaryDir>
#include <QtTest>

#include <nuspell/dictionary.hxx>

namespace {
// What a LanguageTool server answers for "helo world": one match, `helo` (offset 0, length 4)
constexpr auto LANGUAGETOOL_REPLY = R"json({"software":{"name":"LanguageTool","version":"6.0","buildDate":"2024","apiVersion":1,"premium":false,"status":""},"warnings":{"incompleteResults":false},"language":{"name":"English (US)","code":"en-US","detectedLanguage":{"name":"English (US)","code":"en-US","confidence":0.9}},"matches":[{"message":"Possible spelling mistake found.","shortMessage":"Spelling mistake","replacements":[{"value":"hello"}],"offset":0,"length":4,"context":{"text":"helo world","offset":0,"length":4},"sentence":"helo world","type":{"typeName":"Other"},"rule":{"id":"MORFOLOGIK_RULE_EN_US","description":"Possible spelling mistake","issueType":"misspelling","category":{"id":"TYPOS","name":"Possible Typo"}},"ignoreForIncompleteSentence":false,"contextForSureMatch":0}]})json";
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

auto termOf(const QString& term, const QString& translation) -> Term {
    return Term{ .term = term, .translation = translation, .note = QString(), .sourceMatchMode = {}, .translationMatchMode = {} };
}

// `count` rows of a file, each with a source and a translation
auto manyRows(const i32 count) -> QString {
    QString text;

    for (i32 row = 0; row < count; row++) {
        text += u"k%1<#>src<#>tr\n"_s.arg(row);
    }

    text.removeLast();
    return text;
}

// An MV/MZ system sheet with the window colors `\C[n]` reads: color 0 white, color 2 a known one, the rest black
void writeWindowSheet(const QString& systemDirectory) {
    QImage image(256, 256, QImage::Format_RGB32);
    image.fill(Qt::black);

    for (i32 index = 0; index < 32; index++) {
        image.setPixel(96 + ((index % 8) * 12), 144 + ((index / 8) * 12), index == 0 ? qRgb(255, 255, 255) : qRgb(10, 20, 30));
    }

    QVERIFY(QDir().mkpath(systemDirectory));
    QVERIFY(image.save(systemDirectory + u"/Window.png"_s, "PNG"));
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

    // Without the newline that ends a file after it is written by the worker, which the expected texts do not spell out
    [[nodiscard]] auto read(const QString& name) const -> QString {
        QString text = QString::fromUtf8(readFile(path(name)));
        text.chop(text.endsWith(u'\n') ? 1 : 0);
        return text;
    }

    [[nodiscard]] auto raw(const QString& name) const -> QByteArray { return readFile(path(name)); }

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

// The lint is a blocking call, and the server answers from this thread, so it runs on another one
[[nodiscard]] auto lintWithLanguageTool(Project& project, const QString& url, const QString& translation) -> LintOutcome {
    project.settings.translation.lintFlags = LintFlags::LanguageTool;
    project.settings.translation.languageTool.baseURL = url;
    project.projectSettings.translationLang = u"en-US"_s;

    auto future = QtConcurrent::run([&project, &translation] -> LintOutcome {
        return project.worker.lint(u"actors"_s, 5, u"source"_qsv, translation, PreparedGlossary{}, CustomLintMatcher{}, false);
    });

    // Not a QTRY: those return from a function that returns nothing. The server needs this thread to answer.
    while (!future.isFinished()) {
        QTest::qWait(10);
    }

    return future.result();
}


auto withText(QString text, const QString& from, const QString& with) -> QString {
    return text.replace(from, with);
}

// Comments, a line break, an empty translation and a second translation column
const QString SERDE_ACTORS =
    u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nLine1\\#Line2<#>Строка1\\#Строка2\nBob<#>\nTwo<#>Один<#>Два"_s;

const QString ACTORS = u"Harold<#>Гарольд\nAlice<#>Алиса\nLine1\\#Line2<#>Строка1\\#Строка2\nBob<#>"_s;

// The hashes a read reports: a count, then a name and a hash for each file
auto hashesOf(const ReadResult& read) -> HashMap<FilenameArray, u64> {
    HashMap<FilenameArray, u64> hashes;
    const u8* cursor = read->ptr + sizeof(u32);
    u32 count;
    memcpy(&count, read->ptr, sizeof(count));

    for (u32 file = 0; file < count; file++) {
        FilenameArray name;
        u64 hash;
        memcpy(name.data(), cursor, name.size());
        memcpy(&hash, cursor + name.size(), sizeof(hash));
        hashes.emplace(name, hash);
        cursor += name.size() + sizeof(hash);
    }

    return hashes;
}

// Whether a file of the project is named like the ignore list a purge can leave
auto hasIgnoreFile(const QString& root) -> bool {
    QDirIterator files(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);

    while (files.hasNext()) {
        if (files.next().contains(u"ignore"_qsv, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}
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

        const auto count = [&](const QString& text, const u8 flags, const SearchLocation location = SearchLocation::Source) -> usize {
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

        QCOMPARE(count(u"harold"_s, 0), usize{ 1 });
        QCOMPARE(count(u"harold"_s, SearchFlags::CaseSensitive), usize{ 0 });
        QCOMPARE(count(u"Harold"_s, SearchFlags::CaseSensitive), usize{ 1 });
        QCOMPARE(count(u"Ali"_s, 0), usize{ 1 });
        QCOMPARE(count(u"Ali"_s, SearchFlags::WholeWord), usize{ 0 });
        QCOMPARE(count(u"Alice"_s, SearchFlags::WholeWord), usize{ 1 });
        // Not a regular expression unless asked: the dot is a dot
        QCOMPARE(count(u"Har.ld"_s, 0), usize{ 0 });
        QCOMPARE(count(u"Har.ld"_s, SearchFlags::RegExp), usize{ 1 });
        // Cyrillic is case-folded too: the Cyrillic text is in the translation column, not in the source
        QCOMPARE(count(u"ГАРОЛЬД"_s, 0), usize{ 0 });
        QCOMPARE(count(u"ГАРОЛЬД"_s, 0, SearchLocation::Translation), usize{ 1 });
        QCOMPARE(count(u"ГАРОЛЬД"_s, SearchFlags::CaseSensitive, SearchLocation::Translation), usize{ 0 });
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
    void aReplacedFileEndsWithANewlineLikeASavedOne() {
        Project project;
        project.write(u"actors"_s, ACTORS);
        QVERIFY(!project.raw(u"actors"_s).endsWith('\n'));

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
        QVERIFY(project.raw(u"actors"_s).endsWith('\n'));
    }

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

    // Replacing the text of a match: back references, the text around the match, and the escapes that mean nothing
    void replaceTemplates_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("pattern");
        QTest::addColumn<bool>("regexp");
        QTest::addColumn<QString>("replacement");
        QTest::addColumn<QString>("expected");

        QTest::newRow("replaceUsesBackReferences")
            << u"x<#>John Smith\ny<#>Mary Jones"_s << u"(\\w+) (\\w+)"_s << true << u"\\2, \\1 (\\+) [\\\\]"_s
            << u"x<#>Smith, John (Smith) [\\]\ny<#>Jones, Mary (Jones) [\\]"_s;
        QTest::newRow("replaceKeepsEscapesThatMeanNothing")
            << u"x<#>John Smith"_s << u"(\\w+) (\\w+)"_s << true << u"\\q \\9 \\0 \\2\\"_s << u"x<#>\\q \\9 \\0 Smith\\"_s;
        QTest::newRow("replaceUsesTheTextBeforeAndAfterTheMatch")
            << u"x<#>one two three"_s << u"two"_s << false << u"[\\`|\\']"_s << u"x<#>one [one | three] three"_s;
        // A group that did not take part is empty, also the last one
        QTest::newRow("replaceTreatsAnOptionalGroupThatDidNotMatchAsEmpty")
            << u"x<#>ab\ny<#>a"_s << u"(a)(b)?"_s << true << u"[\\1|\\2]"_s << u"x<#>[a|b]\ny<#>[a|]"_s;
        // Group 10, then group 1 followed by a literal 1; with no group 99, group 9 and a literal 9; group 0 and other
        // escapes stay
        QTest::newRow("replaceReadsTwoDigitGroupsAndKeepsUnknownEscapes")
            << u"x<#>abcdefghij"_s << u"(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)"_s << true << u"\\10\\11|\\99|\\0|\\z|\\"_s
            << u"x<#>ja1|i9|\\0|\\z|\\"_s;
        // A backslash that starts no escape of the replacement stays in the text
        QTest::newRow("replaceKeepsABackslashThatEscapesNothing")
            << u"x<#>John"_s << u"John"_s << false << u"end\\>"_s << u"x<#>end\\>"_s;
    }

    void replaceTemplates() {
        QFETCH(const QString, text);
        QFETCH(const QString, pattern);
        QFETCH(const bool, regexp);
        QFETCH(const QString, replacement);
        QFETCH(const QString, expected);

        Project project;
        project.write(u"actors"_s, text);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            pattern,
            SearchLocation::Translation,
            -1,
            regexp ? searchFlags(SearchFlags::RegExp) : searchFlags(),
            1
        );

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, replacement);

        QVERIFY(replaced.skipped.empty());
        QCOMPARE(project.read(u"actors"_s), expected);
    }

    // An escape that means nothing, a group that does not exist and a lone trailing backslash stay as typed
    // The cell shrank after the search: the positions no longer fit, so the cell is left as it is
    void replaceLeavesACellThatChangedSinceTheSearch() {
        Project project;
        project.write(u"actors"_s, u"x<#>some long text here\ny<#>some long text here"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"here"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        project.write(u"actors"_s, u"x<#>short\ny<#>some long text here"_s);

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"there"_s);

        QCOMPARE(project.read(u"actors"_s), u"x<#>short\ny<#>some long text there"_s);
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

    void replaceStopsWhenTheTaskIsInterrupted() {
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
        task->interrupt();

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"Элис"_s);

        // Nothing is half done: the file was not entered
        QVERIFY(replaced.deltas.empty());
        QVERIFY(replaced.skipped.empty());
        QCOMPARE(project.read(u"actors"_s), ACTORS);
    }

    // ReplaceSingle
    void replaceSinglePutPadsAShortRowOfAConfiguredColumn() {
        Project project;
        project.projectSettings.columns = { ColumnInfo{ u"Source"_s, 100 },
                                            ColumnInfo{ u"One"_s, 100 },
                                            ColumnInfo{ u"Two"_s, 100 } };
        project.write(u"actors"_s, u"Harold<#>one"_s);

        const auto result = project.worker.replaceSingle(
            u"two"_s,
            SearchPanelDock::Action::Put,
            u"actors"_s,
            0,
            2,
            span<const TextMatch>(),
            QString()
        );

        QVERIFY(result.has_value());
        QCOMPARE(result->text, u"two"_s);
        QCOMPARE(result->translatedDelta, 0);
        QCOMPARE(project.read(u"actors"_s), u"Harold<#>one<#>two"_s);
    }

    void replaceSinglePutRefusesAColumnTheProjectDoesNotHave() {
        Project project;
        project.projectSettings.columns = { ColumnInfo{ u"Source"_s, 100 }, ColumnInfo{ u"One"_s, 100 } };
        project.write(u"actors"_s, u"Harold<#>one"_s);

        const auto result = project.worker.replaceSingle(
            u"two"_s,
            SearchPanelDock::Action::Put,
            u"actors"_s,
            0,
            2,
            span<const TextMatch>(),
            QString()
        );

        QVERIFY(!result.has_value());
        QVERIFY(result.error().stale);
        QCOMPARE(project.read(u"actors"_s), u"Harold<#>one"_s);
    }

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

    // The hashes of the sources go to the core and come back: a file whose source did not change is not read again
    void aReadGivenTheHashesOfASourceDoesNotTouchItsFile() {
        Project project;
        project.writeActorsData();
        const auto first = project.readActors();
        QVERIFY(first.has_value());
        const auto hashes = hashesOf(first);
        QCOMPARE(hashes.size(), usize{ 1 });
        QCOMPARE(QL1SV(hashes.begin()->first.data()), u"actors"_qsv);

        // The row was removed by hand
        const QString edited = u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд"_s;
        project.write(u"actors"_s, edited);

        const auto again = project.worker.read(
            project.projectSettings.sourcePath(),
            project.projectSettings.translationPath(),
            ReadMode::AppendDefault,
            EngineType::MVMZ,
            DuplicateMode::Allow,
            onlyFiles(FileFlags_Actors),
            static_cast<BaseFlags>(0),
            false,
            hashes,
            u"Test Game"_s,
            QString()
        );

        QVERIFY2(again.has_value(), errorOf(again).constData());
        QCOMPARE(project.read(u"actors"_s), edited);

        // A source that is not the one the hash was made of is read again
        auto stale = hashes;
        stale.begin()->second ^= 1;
        const auto changed = project.worker.read(
            project.projectSettings.sourcePath(),
            project.projectSettings.translationPath(),
            ReadMode::AppendDefault,
            EngineType::MVMZ,
            DuplicateMode::Allow,
            onlyFiles(FileFlags_Actors),
            static_cast<BaseFlags>(0),
            false,
            stale,
            u"Test Game"_s,
            QString()
        );

        QVERIFY2(changed.has_value(), errorOf(changed).constData());
        QVERIFY(project.read(u"actors"_s).contains(u"The Brave"_qsv));
    }

    void purgeCanLeaveAnIgnoreList() {
        Project project;
        project.writeActorsData();
        QVERIFY(project.readActors().has_value());
        project.write(u"actors"_s, u"<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>"_s);
        QVERIFY(!hasIgnoreFile(project.dir.path()));

        const auto purged = project.worker.purge(onlyFiles(FileFlags_Actors), true);

        QVERIFY2(purged.has_value(), errorOf(purged).constData());
        QVERIFY(hasIgnoreFile(project.dir.path()));
    }

    void readAndWriteReportTheFilesTheyCannotUse() {
        Project project;

        // There is no game here
        const auto read = project.readActors();
        QVERIFY(!read.has_value());
        const QString read_text = QString::fromUtf8(ffitostr(read.error()));
        QVERIFY2(read_text.contains(project.dir.path()), qPrintable(read_text));

        const auto written = project.worker.write(onlyFiles(FileFlags_Actors));
        QVERIFY(!written.has_value());
        const QString written_text = QString::fromUtf8(ffitostr(written.error()));
        QVERIFY2(written_text.contains(project.dir.path()), qPrintable(written_text));
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

    void aFileScriptThatThrowsLeavesTheFileAlone() {
        Project project;
        const QString text = u"a<#>one\nb<#>two"_s;
        project.write(u"actors"_s, text);

        const JSScript script{
            .action = BatchAction::Custom,
            .name = u"throws"_s,
            .source = u"function rpgmFileCallback(name, rows) { throw new Error('whole file'); }"_s
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
        QVERIFY(result.error().message.contains(u"whole file"_qsv));
        QCOMPARE(result.error().filename, filenameOf("actors"));
        QCOMPARE(project.read(u"actors"_s), text);
    }

    void fileScriptKeepsARowThatCannotBeSplit() {
        Project project;
        project.write(u"actors"_s, u"a<#>one\nno separator here\nb<#>two"_s);

        const JSScript script{
            .action = BatchAction::Custom,
            .name = u"all"_s,
            .source = u"function rpgmFileCallback(name, rows) { return rows.map(r => ['x']); }"_s
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
        QCOMPARE(project.read(u"actors"_s), u"a<#>x\nno separator here\nb<#>x"_s);
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

    // Spelling: a word the dictionary does not know is underlined, and offered what it does know
    void lintUnderlinesTheWordsTheDictionaryDoesNotKnow() {
        const QTemporaryDir dir;
        writeFile(dir.filePath(u"en.aff"_s), "SET UTF-8\n");
        writeFile(dir.filePath(u"en.dic"_s), "2\nhello\nworld\n");
        nuspell::Dictionary dictionary;
        dictionary.load_aff_dic(dir.filePath(u"en.aff"_s).toStdString());

        Project project;
        project.settings.translation.lintFlags = LintFlags::Spellcheck;
        project.worker.setDictionary(std::move(dictionary));

        const auto outcome =
            project.worker.lint(u"actors"_s, 3, u"source"_qsv, u"hello wrold"_qsv, PreparedGlossary{}, CustomLintMatcher{}, true);

        // Only the unknown word, where it is
        QCOMPARE(outcome.rows.size(), usize{ 1 });
        QCOMPARE(outcome.rows.front().lineNumber, u32{ 3 });
        QCOMPARE(outcome.rows.front().translationHighlights.at(0).start, u32{ 6 });
        QCOMPARE(outcome.rows.front().translationHighlights.at(0).len, u32{ 5 });

        // The suggestions are looked up for the cell that is being edited, and `world` is the swap of its letters
        QStringList suggestions;
        for (const LintCharState& state : outcome.charStates) {
            for (const LintEntry& entry : state.entries) {
                QCOMPARE(entry.captured, u"wrold"_s);
                suggestions = entry.suggestions.toStringList();
            }
        }
        QVERIFY2(suggestions.contains(u"world"_s), qPrintable(suggestions.join(u',')));

        // A lint of files has no use for the suggestions, which are the slow part
        const auto plain =
            project.worker.lint(u"actors"_s, 3, u"source"_qsv, u"hello wrold"_qsv, PreparedGlossary{}, CustomLintMatcher{}, false);
        QCOMPARE(plain.rows.size(), usize{ 1 });
        QVERIFY(plain.charStates.empty());

        // Words it knows, and no dictionary at all
        QVERIFY(project.worker.lint(u"actors"_s, 3, u"s"_qsv, u"hello world"_qsv, PreparedGlossary{}, CustomLintMatcher{}, false).rows.empty());
        project.worker.setDictionary(nullopt);
        QVERIFY(project.worker.lint(u"actors"_s, 3, u"s"_qsv, u"hello wrold"_qsv, PreparedGlossary{}, CustomLintMatcher{}, false).rows.empty());
    }

    void lintShowsWhatLanguageToolFindsWhereItFinds() {
        QByteArray request;
        const HttpStub server([&request](const QString&, const QString&, const QByteArray& body) -> QByteArray {
            request = body;
            return HttpStub::response("200 OK", LANGUAGETOOL_REPLY);
        });

        Project project;
        const auto outcome = lintWithLanguageTool(project, server.url(), u"helo world"_s);

        QCOMPARE(outcome.rows.size(), usize{ 1 });
        const LintRow& row = outcome.rows.front();
        QCOMPARE(row.info, u"Possible spelling mistake found."_s);
        QCOMPARE(row.lineNumber, u32{ 5 });
        QCOMPARE(row.translationHighlights.at(0).start, u32{ 0 });
        QCOMPARE(row.translationHighlights.at(0).len, u32{ 4 });
        // The text went to the server, as the language of the translation
        QVERIFY2(request.contains("helo"), request.constData());
        QVERIFY2(request.contains("en-US"), request.constData());
    }

    void lintGivesLanguageToolTheCodesOfTheTextAsMarkup() {
        QByteArray request;
        const HttpStub server([&request](const QString&, const QString&, const QByteArray& body) -> QByteArray {
            request = body;
            return HttpStub::response("200 OK", LANGUAGETOOL_REPLY);
        });

        Project project;
        (void)lintWithLanguageTool(project, server.url(), u"helo \\C[2]world"_s);

        // The code is not a word to check
        QVERIFY2(request.contains("markup"), request.constData());
        QVERIFY2(request.contains("text"), request.constData());
    }

    void lintIgnoresALanguageToolMatchOutsideTheText() {
        // The server points past the end of the text it was given
        QByteArray reply = QByteArray(LANGUAGETOOL_REPLY);
        reply.replace("\"offset\":0,\"length\":4,\"context\"", "\"offset\":50,\"length\":4,\"context\"");
        QVERIFY(reply != QByteArray(LANGUAGETOOL_REPLY));
        const HttpStub server([&reply](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("200 OK", reply);
        });

        Project project;
        const auto outcome = lintWithLanguageTool(project, server.url(), u"helo world"_s);

        QVERIFY(outcome.rows.empty());
    }

    void lintReportsALanguageToolThatCannotBeUsed_data() {
        QTest::addColumn<QString>("status");
        QTest::addColumn<QByteArray>("reply");

        QTest::newRow("a server that fails") << u"500 Internal Server Error"_s << QByteArray("{}");
        QTest::newRow("not LanguageTool") << u"200 OK"_s << QByteArray("[1, 2, 3]");
    }

    void lintReportsALanguageToolThatCannotBeUsed() {
        QFETCH(const QString, status);
        QFETCH(const QByteArray, reply);

        const HttpStub server([&](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response(status.toUtf8().constData(), reply);
        });

        Project project;
        const auto outcome = lintWithLanguageTool(project, server.url(), u"helo world"_s);

        // The check failing is a row of its own, not a lint of the text and not a crash
        QCOMPARE(outcome.rows.size(), usize{ 1 });
        QVERIFY2(outcome.rows.front().info.contains(u"LanguageTool check failed"_qsv), qPrintable(outcome.rows.front().info));
        QCOMPARE(outcome.rows.front().translationText, u"helo world"_s);
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

    void aTextCodeTheUserSwitchedOffIsNotHighlighted() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);

        const auto highlighted = [&] -> bool {
            const auto outcome =
                project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"\\C[2]ab"_qsv, PreparedGlossary{}, CustomLintMatcher{}, true);

            return ranges::any_of(outcome.charStates, [](const LintCharState& state) -> bool { return !state.entries.isEmpty(); });
        };

        QVERIFY(highlighted());

        for (auto& code : project.settings.translation.textCodes) {
            code.disabled = true;
        }

        QVERIFY(!highlighted());
    }

    void aFileThatCannotBeWrittenIsSkippedAndKeptByAScript() {
        if (runsAsRoot()) {
            QSKIP("root reads what a file refuses");
        }

        Project project;
        project.write(u"actors"_s, u"a<#>one"_s);
        const QString dirPath = project.projectSettings.translationPath();
        QVERIFY(QFile::setPermissions(project.path(u"actors"_s), QFileDevice::ReadOwner));
        QVERIFY(QFile::setPermissions(dirPath, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"all"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => 'changed'); }"_s };

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to write file"_s));
        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(QFile::setPermissions(dirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        QVERIFY(QFile::setPermissions(project.path(u"actors"_s), QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        QVERIFY(result.has_value());
        QCOMPARE(result->skippedCount, u32{ 1 });
        QCOMPARE(project.read(u"actors"_s), u"a<#>one"_s);
    }

    void aTextCodeWithAVariableColorAndACustomLintColorAreHighlighted() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        project.settings.translation.customLints = { CustomLint{ .sequence = u"todo"_s,
                                                                 .tooltip = u"Finish it"_s,
                                                                 .useCustomColor = true,
                                                                 .customColor = qRgb(1, 2, 3) } };

        const CustomLintMatcher matcher = project.worker.customLintMatcher();
        const auto outcome =
            project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"\\C[\\V[2]] todo"_qsv, PreparedGlossary{}, matcher, true);

        // The color of the variable is known only while the game runs, so the code is marked and nothing is colored by it
        QVERIFY(!outcome.charStates[0].entries.isEmpty());

        const auto todo = ranges::find_if(outcome.charStates, [](const LintCharState& state) -> bool {
            return state.foreground == QColor(qRgb(1, 2, 3));
        });
        QVERIFY(todo != outcome.charStates.end());
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

    // A lint that matches nothing at a place is no lint there, and does not hide the next one
    void aCustomLintThatMatchesEmptyDoesNotMarkAnythingNorShiftTheNextLint() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        project.settings.translation.customLints = { CustomLint{ .sequence = u"x*"_s, .tooltip = u"Empty"_s },
                                                     CustomLint{ .sequence = u"b"_s, .tooltip = u"The b"_s } };

        const CustomLintMatcher matcher = project.worker.customLintMatcher();
        const auto outcome = project.worker.lint(u"actors"_s, 1, u"s"_qsv, u"ab"_qsv, PreparedGlossary{}, matcher, true);

        QCOMPARE(outcome.charStates.size(), usize{ 2 });
        QVERIFY(outcome.charStates[0].entries.isEmpty());
        QCOMPARE(outcome.charStates[1].entries.size(), qsizetype{ 1 });
        QCOMPARE(outcome.charStates[1].entries[0].extra.toString(), u"The b"_s);
        QCOMPARE(outcome.charStates[1].entries[0].index, 1);
    }

    // Glossary
    void lintFindsATermTheTranslationDoesNotHave() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TermMismatch);
        project.projectSettings.sourceLang = u"en"_s;
        project.projectSettings.translationLang = u"ru"_s;

        const Glossary glossary = { termOf(u"Harold"_s, u"Гарольд"_s) };
        const PreparedGlossary prepared = prepareGlossary(glossary);

        const auto lint = [&](const QStringView source, const QStringView translation) -> LintOutcome {
            return project.worker.lint(u"actors"_s, 3, source, translation, prepared, CustomLintMatcher{}, false);
        };

        const auto matched = lint(u"Harold ran"_qsv, u"Гарольд побежал"_qsv);
        QCOMPARE(matched.rows.size(), usize{ 1 });
        QVERIFY(matched.rows[0].info.startsWith(u"Match."_qsv));
        QVERIFY(matched.rows[0].info.contains(u"Exact"_qsv));
        QCOMPARE(matched.rows[0].lineNumber, u32{ 3 });
        QCOMPARE(matched.rows[0].sourceHighlights.at(0).start, u32{ 0 });
        QCOMPARE(matched.rows[0].sourceHighlights.at(0).len, u32{ 6 });
        QCOMPARE(matched.rows[0].translationHighlights.at(0).len, u32{ 7 });

        const auto absent = lint(u"Harold ran"_qsv, u"Он побежал"_qsv);
        QCOMPARE(absent.rows.size(), usize{ 1 });
        QVERIFY(absent.rows[0].info.startsWith(u"Term translation is not present."_qsv));

        const auto empty = lint(u"Harold ran"_qsv, QStringView());
        QCOMPARE(empty.rows.size(), usize{ 1 });
        QVERIFY(empty.rows[0].info.startsWith(u"Translation is empty."_qsv));

        const auto fewer = lint(u"Harold met Harold"_qsv, u"Гарольд встретил его"_qsv);
        QCOMPARE(fewer.rows.size(), usize{ 1 });
        QVERIFY(fewer.rows[0].info.contains(u"doesn't match"_qsv));

        // A term the source doesn't hold has nothing to check
        QVERIFY(lint(u"Bob ran"_qsv, u"Боб побежал"_qsv).rows.empty());
    }

    // The matches count characters, a QString counts UTF-16 units
    void glossaryHighlightsFollowCharactersOutsideTheBmp() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TermMismatch);
        project.projectSettings.sourceLang = u"en"_s;
        project.projectSettings.translationLang = u"ru"_s;

        const Glossary glossary = { termOf(u"Harold"_s, u"Гарольд"_s) };
        const PreparedGlossary prepared = prepareGlossary(glossary);

        const auto outcome = project.worker.lint(
            u"actors"_s,
            1,
            u"\U0001F600 Harold"_qsv,
            u"\U0001F600 Гарольд"_qsv,
            prepared,
            CustomLintMatcher{},
            false
        );

        QCOMPARE(outcome.rows.size(), usize{ 1 });
        QCOMPARE(outcome.rows[0].sourceHighlights.at(0).start, u32{ 3 });
        QCOMPARE(outcome.rows[0].sourceHighlights.at(0).len, u32{ 6 });
        QCOMPARE(outcome.rows[0].translationHighlights.at(0).start, u32{ 3 });
    }

    void glossaryChecksNeedBothLanguagesAndTheFlag() {
        Project project;
        const Glossary glossary = { termOf(u"Harold"_s, u"Гарольд"_s) };
        const PreparedGlossary prepared = prepareGlossary(glossary);

        const auto rows = [&] -> usize {
            return project.worker.lint(u"a"_s, 1, u"Harold"_qsv, u"Он"_qsv, prepared, CustomLintMatcher{}, false)
                .rows.size();
        };

        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TermMismatch);
        QCOMPARE(rows(), usize{ 0 });

        project.projectSettings.sourceLang = u"en"_s;
        QCOMPARE(rows(), usize{ 0 });

        project.projectSettings.translationLang = u"ru"_s;
        QCOMPARE(rows(), usize{ 1 });

        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        QCOMPARE(rows(), usize{ 0 });
    }

    void prepareGlossaryKeepsOnlyFilledInEnabledTerms() {
        Glossary glossary = { termOf(u"a"_s, u"b"_s),
                              termOf(QString(), u"b"_s),
                              termOf(u"a"_s, QString()),
                              termOf(u"c"_s, u"d"_s) };
        glossary[3].enabled = false;

        const PreparedGlossary prepared = prepareGlossary(glossary);

        QCOMPARE(prepared.size(), usize{ 1 });
        QCOMPARE(prepared[0].term, &glossary[0]);
        QCOMPARE(prepared[0].termUtf8, QByteArray("a"));
        QCOMPARE(prepared[0].translationUtf8, QByteArray("b"));
    }

    // Window colors
    void colorCodesTakeTheColorOfTheSystemSheet() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        writeWindowSheet(project.projectSettings.projectPath + u"/img/system"_s);

        const auto outcome = project.worker.lint(
            u"a"_s,
            1,
            u"s"_qsv,
            u"\\C[2]Hi\\C[0]"_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            true
        );

        QCOMPARE(outcome.charStates[0].entries.size(), qsizetype{ 1 });
        QCOMPARE(outcome.charStates[0].entries[0].type, LintType::TextCode);
        QCOMPARE(outcome.charStates[0].entries[0].extra.toUInt(), qRgb(10, 20, 30));
        QCOMPARE(outcome.charStates[0].foreground, QColor(qRgb(10, 20, 30)));
        QCOMPARE(outcome.charStates[7].entries[0].extra.toUInt(), qRgb(255, 255, 255));
    }

    void aColorThatIsNeverClosedIsFlagged() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        writeWindowSheet(project.projectSettings.projectPath + u"/img/system"_s);

        const auto unclosed =
            project.worker.lint(u"a"_s, 1, u"s"_qsv, u"\\C[2]Hi"_qsv, PreparedGlossary{}, CustomLintMatcher{}, true);
        const auto closed = project.worker.lint(
            u"a"_s,
            1,
            u"s"_qsv,
            u"\\C[2]Hi\\C[0]"_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            true
        );

        const auto hasMisc = [](const LintOutcome& outcome) -> bool {
            return ranges::any_of(outcome.charStates[0].entries, [](const LintEntry& entry) -> bool {
                return entry.type == LintType::Misc;
            });
        };

        QVERIFY(hasMisc(unclosed));
        QVERIFY(!hasMisc(closed));
    }

    void aColorNumberTheSheetDoesNotHaveIsReported() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);
        writeWindowSheet(project.projectSettings.projectPath + u"/img/system"_s);

        const auto outcome =
            project.worker.lint(u"a"_s, 1, u"s"_qsv, u"\\C[40]Hi"_qsv, PreparedGlossary{}, CustomLintMatcher{}, true);

        QCOMPARE(outcome.charStates[0].entries[0].extra.toString(), u"There is no color with this number."_s);
    }

    void everyTaskKindHasItsOwnName() {
        QSet<QString> names;

        for (const TaskKind kind : enchantum::values<TaskKind>) {
            QVERIFY(!taskName(kind).isEmpty());
            names.insert(taskName(kind));
        }

        QCOMPARE(names.size(), static_cast<qsizetype>(enchantum::count<TaskKind>));
    }

    void aScriptReportsProgressInBatchesAndKeepsRowsItCannotSplit() {
        Project project;
        QString text;

        // More rows than one progress batch, and a row that is not text of the game
        for (i32 row = 0; row < 300; row++) {
            text += u"k%1<#>src<#>tr\n"_s.arg(row);
        }
        text += u"no separator at all";
        project.write(u"actors"_s, text);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"upper"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => s.toUpperCase()); }"_s };

        QSignalSpy progress(&project.worker, &TaskWorker::taskProgress);

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(result.has_value());
        // One report after 256 rows, one at the end of the file
        QCOMPARE(progress.count(), 2);

        const QString written = project.read(u"actors"_s);
        QVERIFY(written.contains(u"k299<#>SRC<#>TR"_s));
        QVERIFY(written.endsWith(u"no separator at all"_s));
    }

    void lintRecognizesTheRgbColorCode() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);

        const auto closed = project.worker.lint(
            u"a"_s,
            1,
            u"s"_qsv,
            u"\\hc[ff0000]Hi\\hc[ffffff]"_qsv,
            PreparedGlossary{},
            CustomLintMatcher{},
            true
        );
        const auto unclosed =
            project.worker.lint(u"a"_s, 1, u"s"_qsv, u"\\hc[ff0000]Hi"_qsv, PreparedGlossary{}, CustomLintMatcher{}, true);

        QCOMPARE(closed.charStates[0].foreground, QColor(qRgb(255, 0, 0)));
        QCOMPARE(closed.charStates[0].entries[0].extra.toUInt(), 0xFF0000u);

        const auto hasMisc = [](const LintOutcome& outcome) -> bool {
            return ranges::any_of(outcome.charStates[0].entries, [](const LintEntry& entry) -> bool {
                return entry.type == LintType::Misc;
            });
        };

        QVERIFY(!hasMisc(closed));
        QVERIFY(hasMisc(unclosed));
    }

    void searchReportsProgressAndStopsWhenInterruptedMidFile() {
        Project project;
        project.write(u"actors"_s, manyRows(600));

        const TaskToken task = project.task(TaskKind::Search);
        QSignalSpy progress(&project.worker, &TaskWorker::taskProgress);
        // The first report comes after 256 rows: the task is interrupted there
        QObject::connect(&project.worker, &TaskWorker::taskProgress, &project.worker, [task] { task->interrupt(); });

        const auto result = project.worker.search(
            task,
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"src"_s,
            SearchLocation::Source,
            -1,
            searchFlags(),
            1
        );

        QCOMPARE(progress.count(), 1);
        QCOMPARE(progress.at(0).at(2).toUInt(), u32{ 256 });
        // A file the search did not finish has no matches
        QVERIFY(result.matches.find(filenameOf("actors")) == result.matches.end());
    }

    void searchLeavesRowsWithoutColumnsAlone() {
        Project project;
        project.write(u"actors"_s, u"no separator src\nk<#>src"_s);

        const auto result = project.worker.search(
            project.task(TaskKind::Search),
            SearchAction::Search,
            selectionOf(FileFlags_Actors),
            u"src"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        const FileMatches& file = result.matches.at(filenameOf("actors"));
        QCOMPARE(file.cells.size(), usize{ 1 });
        QCOMPARE(file.cells[0].rowIndex(), 1);
    }

    void replaceReportsProgressInBatches() {
        Project project;
        project.write(u"actors"_s, manyRows(300));

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"tr"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        QSignalSpy progress(&project.worker, &TaskWorker::taskProgress);
        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"done"_s);

        QVERIFY(replaced.skipped.empty());
        QCOMPARE(progress.count(), 1);
        QVERIFY(project.read(u"actors"_s).contains(u"k299<#>src<#>done"_s));
    }

    void replaceIgnoresMatchesOfRowsTheFileNoLongerHas() {
        Project project;
        project.write(u"actors"_s, u"a<#>x<#>one\nb<#>y<#>one\nc<#>z<#>one"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        const auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"one"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        // The file lost its last two rows after the search
        project.write(u"actors"_s, u"a<#>x<#>one"_s);

        const auto replaced = project.worker.replace(task, found.matches, SearchAction::Replace, u"two"_s);

        QVERIFY(replaced.skipped.empty());
        QCOMPARE(project.read(u"actors"_s), u"a<#>x<#>two"_s);
    }

    void statisticsSkipRowsWithoutColumnsAndStopWhenInterrupted() {
        Project project;
        project.write(u"actors"_s, u"no separator here\nHello world<#>Bonjour"_s);

        const auto counted = project.worker.projectStatistics(project.task(TaskKind::Statistics), { filenameOf("actors") });

        QCOMPARE(counted.files.size(), usize{ 1 });
        QCOMPARE(counted.files[0].source.words, u32{ 2 });

        const TaskToken task = project.task(TaskKind::Statistics);
        task->interrupt();

        QVERIFY(project.worker.projectStatistics(task, { filenameOf("actors") }).files.empty());
    }

    void lintFilesReportsProgressAndSkipsRowsWithoutColumns() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TrailingWhitespace);
        QString text = u"no separator \n"_s;

        for (i32 row = 0; row < 300; row++) {
            text += u"k%1<#>src<#>bad \n"_s.arg(row);
        }

        project.write(u"actors"_s, text);

        QSignalSpy progress(&project.worker, &TaskWorker::taskProgress);
        const auto result = project.worker.lintFiles(project.task(TaskKind::GlobalLint), selectionOf(FileFlags_Actors), Glossary{});

        QCOMPARE(result.rows.size(), usize{ 300 });
        // The 301 lines are past one batch of 256, so the progress is reported once
        QCOMPARE(progress.count(), 1);
    }

    void extractingAnArchiveThatIsNotThereFails() {
        Project project;

        const auto result = project.worker.extractArchive(
            project.dir.path() + u"/missing.rgss3a"_s,
            project.dir.path() + u"/out"_s,
            QString()
        );

        QVERIFY(!result.has_value());
        const QString result_text = QString::fromUtf8(ffitostr(result.error()));
        QVERIFY2(result_text.contains(u"missing.rgss3a"_qsv), qPrintable(result_text));
    }

    void aColorCodeWithoutASheetShowsWhyItHasNoColor() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(0);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Failed to find the system graphic"_s));
        const auto outcome =
            project.worker.lint(u"a"_s, 1, u"s"_qsv, u"\\C[2]Hi"_qsv, PreparedGlossary{}, CustomLintMatcher{}, true);

        QVERIFY(outcome.charStates[0].entries[0].extra.toString().contains(u"Failed to find the system graphic"_qsv));
    }

#ifdef ENABLE_LANGUAGETOOL
    void languageToolSaysWhyItWasSkipped() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::LanguageTool);

        const auto lint = [&] -> LintOutcome {
            return project.worker.lint(u"a"_s, 4, u"s"_qsv, u"text"_qsv, PreparedGlossary{}, CustomLintMatcher{}, false);
        };

        const auto noLanguage = lint();
        QCOMPARE(noLanguage.rows.size(), usize{ 1 });
        QVERIFY(noLanguage.rows[0].info.contains(u"translation language"_qsv));
        QCOMPARE(noLanguage.rows[0].lineNumber, u32{ 4 });

        project.projectSettings.translationLang = u"ru"_s;
        project.settings.translation.languageTool.baseURL.clear();
        const auto noUrl = lint();
        QCOMPARE(noUrl.rows.size(), usize{ 1 });
        QVERIFY(noUrl.rows[0].info.contains(u"base URL"_qsv));
    }
#endif

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

    // The rows are linted on several threads: the ones that come after the interruption are not linted at all
    void lintFilesStopsMidRunWhenInterrupted() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TrailingWhitespace);
        QString text;
        for (i32 row = 0; row < 600; row++) {
            text += u"k%1<#>src<#>tr \n"_s.arg(row);
        }
        project.write(u"actors"_s, text);

        const TaskToken task = project.task(TaskKind::GlobalLint);
        QSignalSpy progress(&project.worker, &TaskWorker::taskProgress);
        // Called on the thread that lints: the first report, after 256 rows, interrupts
        QObject::connect(&project.worker, &TaskWorker::taskProgress, &project.worker, [task] { task->interrupt(); }, Qt::DirectConnection);

        const auto result = project.worker.lintFiles(task, selectionOf(FileFlags_Actors), Glossary{});

        // A row linted after the interruption would have brought the count to 512
        QCOMPARE(progress.count(), 1);
        QVERIFY(result.rows.size() < 600);
    }

    // The segments LanguageTool is given are the text and its markup, one after the other: a code inside another one
    // (a text code in a comment tag) is part of the outer markup, not a second piece of the same characters
    void languageToolIsGivenOverlappingCodesOnlyOnce() {
        QByteArray request;
        const HttpStub server([&request](const QString&, const QString&, const QByteArray& body) -> QByteArray {
            request = body;
            return HttpStub::response("200 OK", LANGUAGETOOL_REPLY);
        });

        Project project;
        (void)lintWithLanguageTool(project, server.url(), uR"(<Battle BGM Name: \C[2]>)"_s);

        const QByteArray decoded = QByteArray::fromPercentEncoding(request).replace('+', ' ');
        QVERIFY2(decoded.contains("<Battle BGM Name:"), decoded.constData());
        QCOMPARE(decoded.count("C[2]"), 1);
    }

    // Offsets are 32 bits here: a server that points further is not understood, and that is a row, not a crash
    void aLanguageToolAnswerThatDoesNotFitIsReported() {
        QByteArray reply = QByteArray(LANGUAGETOOL_REPLY);
        reply.replace("\"offset\":0,\"length\":4,\"context\"", "\"offset\":5000000000,\"length\":4,\"context\"");
        QVERIFY(reply != QByteArray(LANGUAGETOOL_REPLY));
        const HttpStub server([&reply](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("200 OK", reply);
        });

        Project project;
        const auto outcome = lintWithLanguageTool(project, server.url(), u"helo world"_s);

        QCOMPARE(outcome.rows.size(), usize{ 1 });
        QCOMPARE(outcome.rows.front().info, u"Failed to parse LanguageTool response."_s);
        QCOMPARE(outcome.rows.front().translationText, u"helo world"_s);
    }

    // The core cannot match in a language it has no tokenizer for: the term is left out of the lint, and the log says so
    void aGlossaryTermTheCoreCannotMatchIsLeftOutAndLogged() {
        Project project;
        project.settings.translation.lintFlags = static_cast<LintFlags>(LintFlags::TermMismatch);
        project.projectSettings.sourceLang = u"xx"_s;
        project.projectSettings.translationLang = u"ru"_s;

        const Glossary glossary = { termOf(u"Harold"_s, u"Гарольд"_s) };
        const PreparedGlossary prepared = prepareGlossary(glossary);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Glossary matching failed for file actors"_s));
        const auto outcome =
            project.worker.lint(u"actors"_s, 3, u"Harold ran"_qsv, u"Гарольд побежал"_qsv, prepared, CustomLintMatcher{}, false);

        QVERIFY(outcome.rows.empty());
    }

    // A match list is in order and a capture follows its full match; one that does not fit the text as it is changes nothing
    void replaceSkipsACaptureThatLeadsTheMatches() {
        Project project;
        project.write(u"actors"_s, u"x<#>ab ab"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"ab"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        FileMatches& file = found.matches.at(filenameOf("actors"));
        CellMatch& cell = file.cells.at(0);
        file.spans.insert(file.spans.begin() + cell.matchStart, TextMatch(0, 1, true));
        cell.matchCount++;

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"longer"_s);

        QCOMPARE(project.read(u"actors"_s), u"x<#>longer longer"_s);
    }

    void replaceLeavesACellWhoseMatchesAreOutOfOrder() {
        Project project;
        project.write(u"actors"_s, u"x<#>ab ab ab"_s);

        const TaskToken task = project.task(TaskKind::Replace);
        auto found = project.worker.search(
            task,
            SearchAction::Replace,
            selectionOf(FileFlags_Actors),
            u"ab"_s,
            SearchLocation::Translation,
            -1,
            searchFlags(),
            1
        );

        FileMatches& file = found.matches.at(filenameOf("actors"));
        const CellMatch& cell = file.cells.at(0);
        QVERIFY(cell.matchCount >= 2);
        std::swap(file.spans[cell.matchStart], file.spans[cell.matchStart + 1]);

        (void)project.worker.replace(task, found.matches, SearchAction::Replace, u"longer"_s);

        QCOMPARE(project.read(u"actors"_s), u"x<#>ab ab ab"_s);
    }

    // A worksheet cell holds 32767 characters at most: the export of such a file fails, and the file is skipped
    void serdeExportSkipsAFileTheCoreCannotExport() {
        Project project;
        project.write(u"actors"_s, u"Harold<#>" + QString(40000, u'a'));

        QTemporaryDir exportDir;
        QVERIFY(exportDir.isValid());

        vector<FilenameArray> filenames{ filenameOf("actors") };
        const auto exported = project.worker.serdeExport(project.task(TaskKind::SerdeExport), filenames, exportDir.path(), SerdeFormat::Xlsx);

        QVERIFY(exported.has_value());
        QCOMPARE(exported->skippedCount, u32{ 1 });
        QVERIFY(!QFile::exists(exportDir.filePath(u"actors.xlsx"_s)));
    }

    // The first error of a script ends the run: the files after it are not given to the script
    void aScriptThatFailedOnAFileIsNotRunOnTheNextOnes() {
        Project project;
        const QString actors = u"a<#>one"_s;
        const QString items = u"b<#>two"_s;
        project.write(u"actors"_s, actors);
        project.write(u"items"_s, items);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"throws"_s,
                               .source = u"function rpgmFileCallback(name, rows) { throw new Error('in ' + name); }"_s };

        const auto result = project.worker.performBatchAction(
            project.task(TaskKind::BatchScript),
            script,
            selectionOf(static_cast<FileFlags>(FileFlags_Actors | FileFlags_Items)),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QVERIFY(!result.has_value());
        QCOMPARE(result.error().filename, filenameOf("actors"));
        QVERIFY(result.error().message.contains(u"in actors"_qsv));
        QCOMPARE(project.read(u"actors"_s), actors);
        QCOMPARE(project.read(u"items"_s), items);
    }

    // The task is stopped after the file was chosen and before its script ran: the file is written back as it was
    void aFileScriptStoppedBeforeItRunsLeavesTheFileAsItWas() {
        Project project;
        const QString text = u"a<#>one\nb<#>two"_s;
        project.write(u"actors"_s, text);

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"upper"_s,
                               .source = u"function rpgmFileCallback(name, rows) { return rows.map(r => ['X']); }"_s };

        const TaskToken task = project.task(TaskKind::BatchScript);
        // The file is locked for the script right before it is read
        QObject::connect(&project.worker, &TaskWorker::lockFile, &project.worker, [task] { task->interrupt(); });

        (void)project.worker.performBatchAction(
            task,
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QCOMPARE(project.read(u"actors"_s), text);
    }

    // The script is stopped between rows: the file it did not finish is left as it was
    void scriptStoppedMidFileLeavesItAsItWas() {
        Project project;
        project.write(u"actors"_s, manyRows(300));

        const JSScript script{ .action = BatchAction::Custom,
                               .name = u"upper"_s,
                               .source = u"function rpgmLineCallback(src, tr) { return tr.map(s => s.toUpperCase()); }"_s };

        const TaskToken task = project.task(TaskKind::BatchScript);
        QSignalSpy progress(&project.worker, &TaskWorker::taskProgress);
        QObject::connect(&project.worker, &TaskWorker::taskProgress, &project.worker, [task] { task->interrupt(); });

        (void)project.worker.performBatchAction(
            task,
            script,
            selectionOf(FileFlags_Actors),
            1,
            BatchVariant{ u8{ 0 } },
            Glossary{}
        );

        QCOMPARE(progress.count(), 1);
        QCOMPARE(project.read(u"actors"_s), manyRows(300));
    }

    // The endpoints are asked at the same time, and each answer lands in the slot of its endpoint
    void everyEndpointAnswersInItsOwnSlot() {
        const HttpStub yandex([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("200 OK", R"({"translations":[{"text":"Hello"}]})");
        });
        const HttpStub refusing([](const QString&, const QString&, const QByteArray&) -> QByteArray {
            return HttpStub::response("403 Forbidden", R"({"message":"bad key"})");
        });

        const QTemporaryDir dir;
        const QString upper = u"function rpgmTranslateCallback(text) { return text.toUpperCase(); }"_s;
        const QString throwing = u"function rpgmTranslateCallback() { throw new Error('boom'); }"_s;
        writeFile(dir.filePath(u"upper.js"_s), upper.toUtf8());
        writeFile(dir.filePath(u"throwing.js"_s), throwing.toUtf8());
        writeFile(dir.filePath(u"unapproved.js"_s), "function rpgmTranslateCallback(text) { return text; }");

        const auto yandexAt = [](const QString& url) -> EndpointSettings {
            return EndpointSettings{ .name = u"yandex"_s,
                                     .apiKey = u"key"_s,
                                     .yandexFolderId = u"folder"_s,
                                     .yandexBaseUrl = url,
                                     .endpoint = TranslationEndpoint::Yandex };
        };
        const auto scriptAt = [&dir](const QString& name) -> EndpointSettings {
            return EndpointSettings{ .name = name, .scriptPath = dir.filePath(name), .endpoint = TranslationEndpoint::Script };
        };

        Project project;
        const TranslateContext context{
            .endpoints = { yandexAt(yandex.url()),
                           scriptAt(u"upper.js"_s),
                           scriptAt(u"unapproved.js"_s),
                           scriptAt(u"missing.js"_s),
                           yandexAt(refusing.url()),
                           scriptAt(u"throwing.js"_s) },
            .approvedScriptsSha = { scriptHash(upper), scriptHash(throwing) },
            .sourceLang = u"ja"_s,
            .translationLang = u"en"_s,
        };

        // The stub answers from this thread's event loop, so the request cannot block it, as the window's does not
        auto request = QtConcurrent::run([&project, &context] -> TranslateSingleResult {
            return project.worker.translateSingle(u"actors"_s, u"konnichiwa"_s, Glossary{}, context);
        });
        QTRY_VERIFY_WITH_TIMEOUT(request.isFinished(), 20000);
        const auto answers = request.result();

        QCOMPARE(answers.size(), usize{ 6 });
        QVERIFY(!answers[0].failed);
        QCOMPARE(answers[0].text, u"Hello"_s);
        QVERIFY(!answers[1].failed);
        QCOMPARE(answers[1].text, u"KONNICHIWA"_s);
        QVERIFY(answers[2].failed);
        QVERIFY2(answers[2].text.contains(u"not approved"_qsv), qPrintable(answers[2].text));
        QVERIFY(answers[3].failed);
        QVERIFY2(answers[3].text.contains(u"Cannot read script"_qsv), qPrintable(answers[3].text));
        QVERIFY(answers[4].failed);
        QVERIFY2(answers[4].text.contains(u"403"_qsv), qPrintable(answers[4].text));
        QVERIFY(answers[5].failed);
        QVERIFY2(answers[5].text.contains(u"boom"_qsv), qPrintable(answers[5].text));
    }

    void withoutEndpointsThereAreNoAnswers() {
        Project project;

        const auto answers = project.worker.translateSingle(u"actors"_s, u"text"_s, Glossary{}, TranslateContext{});

        QVERIFY(answers.empty());
    }
};

QTEST_MAIN(TestTaskWorkerProject)
#include "tst_taskworker_project.moc"
