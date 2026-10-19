#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "ProjectSettings.hpp"
#include "SearchPanelDock.hpp"
#include "Settings.hpp"
#include "Task.hpp"
#include "Types.hpp"
#include "rpgmtranslate_rs.h"

#ifdef ENABLE_NUSPELL
#include <nuspell/dictionary.hxx>
#endif

#include <QObject>
#include <QRegularExpression>
#include <mutex>

// Guards the strings of `mapSections`: the workers read and rewrite them while the GUI thread saves into them. A
// reader copies its section under it (a shared, cheap copy) and works on the copy.
inline std::mutex mapSectionsMutex;

// `lines` are views into `content`, which holds the file, or the copy of the map section
struct FileLines {
    QString content;
    QSVList lines;
};

auto fileLines(QL1SV filename, const HashMap<u16, QString>& mapSections, const ProjectSettings* projectSettings)
    -> Result<FileLines, QString>;

template <class F>
auto modifyFile(QL1SV filename, HashMap<u16, QString>& mapSections, const ProjectSettings* projectSettings, F&& func)
    -> bool;

// The enabled custom lints merged into one alternation. Building it compiles a regular expression, so a lint run
// builds it once instead of once per row.
struct CustomLintMatcher {
    QRegularExpression regexp;
    // Capture group `n + 1` of `regexp` belongs to the custom lint at `indices[n]`.
    vector<i32> indices;
};

// A glossary term with the UTF-8 forms the matcher needs. They are made once for a lint run, not once per row.
// `term` points into the glossary it was made from, which must outlive it.
struct PreparedTerm {
    const Term* term;
    QByteArray termUtf8;
    QByteArray translationUtf8;
};

using PreparedGlossary = vector<PreparedTerm>;

// The enabled terms of `glossary` that have both sides: a term with an empty side is a row that was added and never
// filled in.
[[nodiscard]] auto prepareGlossary(const Glossary& glossary) -> PreparedGlossary;

// What a translation request reads of the settings. It is copied on the GUI thread, as the request runs for a while
// and the settings window can change them meanwhile.
struct TranslateContext {
    vector<EndpointSettings> endpoints;
    QStringList approvedScripts;
    QString sourceLang;
    QString translationLang;
};

class TaskWorker final : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;

    [[nodiscard]] auto startTask(TaskKind kind) -> Result<TaskToken, TaskKind>;
    void finishTask(const TaskToken& task);

    [[nodiscard]] auto runningTask(TaskKind kind) const -> TaskToken;

    [[nodiscard]] auto idle() const -> bool { return runningTasks.empty(); }

    // Whether a task of this kind would be refused to start now (see `startTask`).
    [[nodiscard]] auto blocks(TaskKind kind) const -> bool;

    // A running task that would hold back whatever has this access to the translation files, if there is one:
    // anything that rewrites them waits for every task, and anything that only reads them for those that rewrite.
    [[nodiscard]] auto blockingTask(TaskAccess access) const -> TaskToken;

    // A running task that can't be told to stop (see `taskInterruptible`), if there is one.
    [[nodiscard]] auto uninterruptibleTask() const -> TaskToken;

    // Asks every running task to stop. Those that can't be interrupted ignore it.
    void interruptAll() const;

    [[nodiscard]] auto extractArchive(const QString& archivePath, const QString& folder, const QString& archiveEncoding)
        -> ExtractResult;

    [[nodiscard]] auto read(
        const QString& sourcePath,
        const QString& translationPath,
        ReadMode readMode,
        EngineType engineType,
        DuplicateMode duplicateMode,
        Selected selected,
        BaseFlags flags,
        bool mapEvents,
        const HashMap<FilenameArray, u64>& hashes,
        const QString& title,
        const QString& readEncoding
    ) -> ReadResult;

    [[nodiscard]] auto write(Selected selected) -> WriteResult;

    [[nodiscard]] auto purge(Selected selected, bool createIgnore) -> PurgeResult;

    [[nodiscard]] auto
    serdeExport(const TaskToken& task, vector<FilenameArray>& filenames, const QString& outputDir, SerdeFormat format)
        -> SerdeResult;

    [[nodiscard]] auto
    serdeImport(const TaskToken& task, vector<FilenameArray>& filenames, const QString& inputDir, SerdeFormat format)
        -> SerdeResult;

    [[nodiscard]] auto search(
        const TaskToken& task,
        SearchAction action,
        Selected selected,
        const QString& searchText,
        SearchLocation searchLocation,
        i8 columnIndex,
        SearchFlags searchFlags,
        u16 tabCount
    ) -> SearchResult;

    [[nodiscard]] auto performBatchAction(
        const TaskToken& task,
        const JSScript& script,
        Selected selected,
        u8 columnIndex,
        const BatchVariant& variant,
        const Glossary& glossary
    ) -> ScriptResult;

    [[nodiscard]] auto
    replace(const TaskToken& task, const CellMatches& searchMatches, SearchAction action, const QString& replaceText)
        -> ReplaceResult;

    [[nodiscard]] auto translateSingle(
        const QString& filename,
        const QString& text,
        const Glossary& glossary,
        const TranslateContext& context
    ) -> TranslateSingleResult;

    [[nodiscard]] auto replaceSingle(
        const QString& replaceText,
        SearchPanelDock::Action action,
        const QString& filename,
        u32 rowIndex,
        u8 columnIndex,
        span<const TextMatch> matches,
        const QString& expectedText
    ) -> ReplaceSingleResult;

#ifdef ENABLE_LANGUAGETOOL
    [[nodiscard]] auto languageToolLint(QStringView text, const vector<Span>& sequences) -> LintResult;
#endif

    [[nodiscard]] auto lintFiles(const TaskToken& task, Selected selected, const Glossary& glossary)
        -> GlobalLintResult;

    // Counts words and characters of every given file, several files at a time.
    [[nodiscard]] auto projectStatistics(const TaskToken& task, const vector<FilenameArray>& filenames)
        -> StatisticsResult;

    [[nodiscard]] auto customLintMatcher() const -> CustomLintMatcher;

    [[nodiscard]] auto lint(
        const QString& filename,
        u32 lineNumber,
        QStringView source,
        QStringView translation,
        const PreparedGlossary& glossary,
        const CustomLintMatcher& customLints,
        bool highlight
    ) -> LintOutcome;

    void init(
        const Settings* const settings,
        const ProjectSettings* const projectSettings,
        HashMap<u16, QString>* const mapSections
    ) {
        this->settings = settings;
        this->projectSettings = projectSettings;
        this->mapSections = mapSections;
    }

#ifdef ENABLE_NUSPELL
    void setDictionary(optional<nuspell::Dictionary> dict) { dictionary = std::move(dict); }
#endif

   signals:
    void lockFile(const QString& file);
    void taskStarted(const TaskToken& task);
    void taskProgress(u32 taskId, const QString& filename, u32 done, u32 total);
    void taskFinished(u32 taskId);

   private:
    template <typename ClosureFn>
    [[nodiscard]] auto runLockedModify(QL1SV filename, ClosureFn&& closure) -> bool;

    [[nodiscard]] auto runBatchScript(
        const TaskToken& task,
        const JSScript& script,
        vector<FilenameArray>& filenames,
        u8 columnIndex,
        const BatchVariant& variant,
        const Glossary& glossary
    ) -> ScriptResult;

    template <typename AttemptFn>
    [[nodiscard]] inline auto
    processFilesTrackingSkips(const TaskToken& task, vector<FilenameArray>& filenames, AttemptFn&& attempt) -> u32;

    // UTF-8 forms of one row and of the project languages, shared by every glossary term the row is matched against.
    struct GlossaryRow {
        QByteArray source;
        QByteArray translation;
        QByteArray sourceLang;
        QByteArray translationLang;
    };

    [[nodiscard]] auto matchGlossaryTerm(
        const QString& filename,
        u32 lineNumber,
        QStringView source,
        QStringView translation,
        const GlossaryRow& glossaryRow,
        const PreparedTerm& prepared
    ) -> optional<LintRow>;

#ifdef ENABLE_NUSPELL
    optional<nuspell::Dictionary> dictionary;
#endif

    vector<TaskToken> runningTasks;

    const Settings* settings;
    const ProjectSettings* projectSettings;

    HashMap<u16, QString>* mapSections;

    u32 nextTaskId = 0;
};
