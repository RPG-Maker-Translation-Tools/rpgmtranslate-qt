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

struct FileLines {
    QString content;
    QSVList lines;
};

auto fileLines(
    QL1SV filename,
    const HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings
) -> result<FileLines, QString>;

template <class F>
auto modifyFile(
    QL1SV filename,
    HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings,
    F&& func
) -> bool;

class TaskWorker final : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;

    [[nodiscard]] auto startTask(TaskKind kind) -> result<TaskToken, TaskKind>;
    void finishTask(const TaskToken& task);

    [[nodiscard]] auto runningTask(TaskKind kind) const -> TaskToken;

    [[nodiscard]] auto extractArchive(const QString& archivePath, const QString& folder) -> ExtractResult;

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
        const QString& title
    ) -> ReadResult;

    [[nodiscard]] auto write(const QString& gameTitle, Selected selected) -> WriteResult;

    [[nodiscard]] auto purge(const QString& gameTitle, Selected selected) -> PurgeResult;

    [[nodiscard]] auto serdeExport(
        const TaskToken& task,
        vector<FilenameArray>& filenames,
        const QString& outputDir,
        SerdeFormat format
    ) -> SerdeResult;

    [[nodiscard]] auto serdeImport(
        const TaskToken& task,
        vector<FilenameArray>& filenames,
        const QString& inputDir,
        SerdeFormat format
    ) -> SerdeResult;

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
    ) -> BatchResult;

    [[nodiscard]] auto
    replace(const TaskToken& task, const CellMatches& searchMatches, SearchAction action, const QString& replaceText)
        -> ReplaceResult;

    [[nodiscard]] auto translateSingle(const QString& filename, const QString& text, const Glossary& glossary)
        -> TranslateSingleResult;

    [[nodiscard]] auto replaceSingle(
        const QString& replaceText,
        SearchPanelDock::Action action,
        const QString& filename,
        u32 rowIndex,
        u8 columnIndex,
        span<const TextMatch> matches
    ) -> ReplaceSingleResult;

    [[nodiscard]] auto languageToolLint(QStringView text, const vector<Span>& sequences) -> LintResult;

    [[nodiscard]] auto lintFiles(const TaskToken& task, Selected selected, const Glossary& glossary)
        -> GlobalLintResult;

    [[nodiscard]] auto lint(
        const QString& filename,
        u32 lineNumber,
        QStringView source,
        QStringView translation,
        const Glossary& glossary,
        bool highlight
    ) -> LintOutcome;

    void init(
        const shared_ptr<Settings>& settings,
        const shared_ptr<ProjectSettings>& projectSettings,
        HashMap<u16, QString>* const mapSections
    ) {
        this->settings = settings;
        this->projectSettings = projectSettings;
        this->mapSections = mapSections;
    }

    void setDictionary(optional<nuspell::Dictionary> dict) { dictionary = std::move(dict); }

   signals:
    void lockFile(const QString& file);
    void taskStarted(const TaskToken& task);
    void taskProgress(u32 taskId, const QString& filename, u32 done, u32 total);
    void taskFinished(u32 taskId);

   private:
    template <typename ClosureFn>
    [[nodiscard]] auto runLockedModify(QL1SV filename, ClosureFn&& closure) -> bool;

    [[nodiscard]] auto runBatchTranslate(
        const TaskToken& task,
        const vector<FilenameArray>& filenames,
        const EndpointContext& endpointContext,
        const Glossary& glossary
    ) -> TranslateResult;

    [[nodiscard]] auto runBatchScript(
        const TaskToken& task,
        const JSScript& script,
        vector<FilenameArray>& filenames,
        u8 columnIndex,
        const BatchVariant& variant
    ) -> ScriptResult;

    template <typename AttemptFn>
    [[nodiscard]] inline auto
    processFilesTrackingSkips(const TaskToken& task, vector<FilenameArray>& filenames, AttemptFn&& attempt) -> u32;

    [[nodiscard]] auto matchGlossaryTerm(
        const QString& filename,
        u32 lineNumber,
        QStringView source,
        QStringView translation,
        const Term& term
    ) -> optional<LintRow>;

    optional<nuspell::Dictionary> dictionary;

    vector<TaskToken> runningTasks;

    shared_ptr<Settings> settings;
    shared_ptr<ProjectSettings> projectSettings;

    HashMap<u16, QString>* mapSections;

    u32 nextTaskId = 0;
};
