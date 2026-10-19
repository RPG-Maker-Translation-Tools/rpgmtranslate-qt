#include "TaskWorker.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "ScriptHost.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"

#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QtConcurrent>
#include <quickjs.h>

namespace {
constexpr QColor RED_HIGHLIGHT = QColor(UINT8_MAX, 0, 0, 80);

constexpr i32 MISC_LINT_UNCLOSED_PUNCTUATION_INDEX = 5;
constexpr i32 MISC_LINT_UNCLOSED_COLOR_INDEX = 6;
constexpr QColor DEFAULT_LINT_COLOR = QColor(0xEF, 0xBF, 0x04);
constexpr i32 SCRIPT_CALLBACK_ARG_COUNT = 5;
constexpr u32 PROGRESS_STEP = 256;

constexpr array<const char*, 7> MISC_LINTS = {
    QT_TRANSLATE_NOOP("Lint", "Misc: Unmatched closing punctuation."),
    QT_TRANSLATE_NOOP("Lint", "Misc: Multiple consequent whitespace."),
    QT_TRANSLATE_NOOP("Lint", "Misc: Trailing whitespace."),
    QT_TRANSLATE_NOOP("Lint", "Misc: Leading whitespace."),
    QT_TRANSLATE_NOOP("Lint", "Misc: Misspelled word."),
    QT_TRANSLATE_NOOP("Lint", "Misc: Unclosed punctuation."),
    QT_TRANSLATE_NOOP("Lint", "Misc: Unclosed tag."),
};

#ifdef ENABLE_LANGUAGETOOL
struct LTReplacement {
    QString value;
};

struct LTCategory {
    QString id;
    QString name;
};

struct LTRule {
    LTCategory category;
    QString description;
    QString id;
    QString issueType;
};

struct LTMatch {
    LTRule rule;
    vector<LTReplacement> replacements;
    QString message;
    QString shortMessage;
    u32 length;
    u32 offset;
};

// Builds the alternating text/markup segments LanguageTool's `data` check mode expects: [count][len, bytes]...,
// starting and ending with a (possibly empty) text segment. Even indices are text; odd ones are markup it ignores.
[[nodiscard]] auto buildLanguageToolSegments(const QStringView translation, vector<Span> spans) -> QByteArray {
    ranges::sort(spans, {}, &Span::start);

    vector<QByteArray> pieces;
    pieces.reserve((spans.size() * 2) + 1);

    u32 cursor = 0;

    for (const auto& span : spans) {
        if (span.start < cursor) {
            continue;
        }

        pieces.push_back(translation.sliced(cursor, span.start - cursor).toUtf8());
        pieces.push_back(translation.sliced(span.start, span.len).toUtf8());
        cursor = span.start + span.len;
    }

    pieces.push_back(translation.sliced(cursor).toUtf8());

    QByteArray buffer;
    const u32 count = scast<u32>(pieces.size());
    buffer.append(rcast<const char*>(&count), sizeof(u32));

    for (const auto& piece : pieces) {
        const u32 len = scast<u32>(piece.size());
        buffer.append(rcast<const char*>(&len), sizeof(u32));
        buffer.append(piece);
    }

    return buffer;
}
#endif

enum class RustLogLevel : u8 {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Trace = 4,
};

struct ToProcess {
    QStringView text;
    i8 columnIndex;
};

// A comment of the file that is no text of the game: nothing is replaced in it or put into it.
[[nodiscard]] auto isProtectedComment(const QStringView source) -> bool {
    return source.startsWith(COMMENT_PREFIX) && !source.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX);
}
}  // namespace

#ifdef ENABLE_LANGUAGETOOL
template <>
struct glz::meta<LTReplacement> {
    static constexpr auto value = glz::object("value", &LTReplacement::value);
};

template <>
struct glz::meta<LTCategory> {
    static constexpr auto value = glz::object("id", &LTCategory::id, "name", &LTCategory::name);
};

template <>
struct glz::meta<LTRule> {
    static constexpr auto value = glz::object(
        "category",
        &LTRule::category,
        "description",
        &LTRule::description,
        "id",
        &LTRule::id,
        "issueType",
        &LTRule::issueType
    );
};

template <>
struct glz::meta<LTMatch> {
    static constexpr auto value = glz::object(
        "length",
        &LTMatch::length,
        "message",
        &LTMatch::message,
        "offset",
        &LTMatch::offset,
        "replacements",
        &LTMatch::replacements,
        "rule",
        &LTMatch::rule,
        "shortMessage",
        &LTMatch::shortMessage
    );
};
#endif

auto taskName(const TaskKind kind) -> QString {
    switch (kind) {
        case TaskKind::Read:
            return QObject::tr("Read");
        case TaskKind::Write:
            return QObject::tr("Write");
        case TaskKind::Purge:
            return QObject::tr("Purge");
        case TaskKind::ExtractArchive:
            return QObject::tr("Extract archive");
        case TaskKind::Search:
            return QObject::tr("Search");
        case TaskKind::Replace:
            return QObject::tr("Replace");
        case TaskKind::ReplaceSingle:
            return QObject::tr("Replace single");
        case TaskKind::BatchScript:
            return QObject::tr("Script");
        case TaskKind::GlobalLint:
            return QObject::tr("Lint");
        case TaskKind::SerdeExport:
            return QObject::tr("Export");
        case TaskKind::SerdeImport:
            return QObject::tr("Import");
        case TaskKind::Statistics:
            return QObject::tr("Statistics");
    }

    std::unreachable();
}

auto TaskWorker::startTask(const TaskKind kind) -> Result<TaskToken, TaskKind> {
    for (const auto& running : runningTasks) {
        if (tasksConflict(kind, running->kind())) {
            return Err(running->kind());
        }
    }

    auto task = std::make_shared<Task>(nextTaskId++, kind);
    runningTasks.push_back(task);

    emit taskStarted(task);
    return task;
}

void TaskWorker::finishTask(const TaskToken& task) {
    std::erase(runningTasks, task);
    emit taskFinished(task->id());
}

auto TaskWorker::runningTask(const TaskKind kind) const -> TaskToken {
    const auto task =
        ranges::find_if(runningTasks, [kind](const TaskToken& running) -> bool { return running->kind() == kind; });

    return task == runningTasks.end() ? nullptr : *task;
}

auto TaskWorker::blocks(const TaskKind kind) const -> bool {
    return ranges::any_of(runningTasks, [kind](const TaskToken& running) -> bool {
        return tasksConflict(kind, running->kind());
    });
}

auto TaskWorker::blockingTask(const TaskAccess access) const -> TaskToken {
    const auto task = ranges::find_if(runningTasks, [access](const TaskToken& running) -> bool {
        return access == TaskAccess::Rewrites || taskAccess(running->kind()) == TaskAccess::Rewrites;
    });

    return task == runningTasks.end() ? nullptr : *task;
}

auto TaskWorker::uninterruptibleTask() const -> TaskToken {
    const auto task = ranges::find_if(runningTasks, [](const TaskToken& running) -> bool {
        return !taskInterruptible(running->kind());
    });

    return task == runningTasks.end() ? nullptr : *task;
}

void TaskWorker::interruptAll() const {
    for (const auto& task : runningTasks) {
        task->interrupt();
    }
}

auto fileLines(
    const QL1SV filename,
    const HashMap<u16, QString>& mapSections,
    const ProjectSettings* const projectSettings
) -> Result<FileLines, QString> {
    QString content;
    QSVList lines;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = stoa<u16>(filename.sliced(3)).value_or(0);

        {
            const std::scoped_lock lock(mapSectionsMutex);
            content = mapSections[mapNumber];
        }

        lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);
    } else {
        const QString path = projectSettings->translationPath() % u'/' % filename % u".txt"_qsv;

        auto file = QFile(path);

        if (!file.open(QFile::ReadOnly)) {
            qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(path, file.errorString());
            return Err(path);
        }

        content = QString::fromUtf8(file.readAll());
        lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);
    }

    return FileLines{ .content = content, .lines = lines };
}

template <class F>
auto modifyFile(
    const QL1SV filename,
    HashMap<u16, QString>& mapSections,
    const ProjectSettings* const projectSettings,
    F&& func
) -> bool {
    QString content;
    QString mapContent;
    QSVList lines;
    QString path;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = stoa<u16>(filename.sliced(3)).value_or(0);

        {
            const std::scoped_lock lock(mapSectionsMutex);
            mapContent = mapSections[mapNumber];
        }

        // `content` stays empty for a map, as it is not a file that can be compared with the result
        lines = QStringView(mapContent).split(u'\n', Qt::SkipEmptyParts);
    } else {
        path = projectSettings->translationPath() % u'/' % filename % u".txt"_qsv;

        // Read-only and closed straight away: `func` may be slow (scripts, network) and must not run with the file
        // held open, nor with the file already opened for writing.
        auto file = QFile(path);

        if (!file.open(QFile::ReadOnly)) {
            qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(path, file.errorString());
            return false;
        }

        content = QString::fromUtf8(file.readAll());
        lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);
    }

    QString result = func(content, lines);

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = stoa<u16>(filename.sliced(3)).value_or(0);
        const std::scoped_lock lock(mapSectionsMutex);
        mapSections.find(mapNumber)->second = std::move(result);
        return true;
    }

    // Nothing to write if the callback left the content as is
    if (result == content) {
        return true;
    }

    const QByteArray utf8 = result.toUtf8();

    auto saveFile = QSaveFile(path);
    saveFile.setDirectWriteFallback(true);

    if (!saveFile.open(QFile::WriteOnly) || saveFile.write(utf8) != utf8.size() || !saveFile.commit()) {
        qWarning().noquote() << u"Failed to write file %1: %2"_qsv.arg(path, saveFile.errorString());
        return false;
    }

    return true;
}

auto TaskWorker::read(
    const QString& sourcePath,
    const QString& translationPath,
    const ReadMode readMode,
    const EngineType engineType,
    const DuplicateMode duplicateMode,
    const Selected selected,
    const BaseFlags flags,
    const bool mapEvents,
    const HashMap<FilenameArray, u64>& hashes,
    const QString& title,
    const QString& readEncoding
) -> ReadResult {
    ByteBuffer outHashes;

    const QByteArray titleUtf8 = title.toUtf8();
    const QByteArray sourcePathUtf8 = sourcePath.toUtf8();
    const QByteArray translationPathUtf8 = translationPath.toUtf8();

    QByteArray hashesArray;
    const u32 hashesArraySize = (hashes.size() * sizeof(FilenameArray)) + (hashes.size() * sizeof(u64));
    hashesArray.reserve(hashesArraySize);

    for (const auto [filename, hash] : hashes) {
        const char* const hashBytes = rcast<const char*>(&hash);

        hashesArray.append(filename);
        hashesArray.append(hashBytes, sizeof(u64));
    }

    const ByteBuffer hashesBuf =
        ByteBuffer{ .ptr = rcast<const u8*>(hashesArray.data()), .len = scast<u32>(hashesArray.size()), .cap = 0 };

    const QByteArray readEncodingUtf8 = readEncoding.toUtf8();

    const bool success = rpgm_read(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        readMode,
        engineType,
        duplicateMode,
        selected.forEngine(engineType),
        flags,
        mapEvents,
        hashesBuf,
        strtoffi(titleUtf8),
        strtoffi(readEncodingUtf8),
        &outHashes
    );

    return success ? ReadResult{ outHashes } : ReadResult{ Err(rpgm_error()) };
}

auto TaskWorker::write(const Selected selected) -> WriteResult {
    const QByteArray sourcePathUtf8 = projectSettings->actualSourcePath().toUtf8();
    const QByteArray translationPathUtf8 = projectSettings->translationPath().toUtf8();
    const QByteArray outputPathUtf8 = projectSettings->outputPath().toUtf8();

    f32 elapsed;
    const QByteArray readEncodingUtf8 = projectSettings->readEncoding.toUtf8();
    const QByteArray writeEncodingUtf8 = projectSettings->writeEncoding.toUtf8();
    const Selected selection = selected.forEngine(projectSettings->engineType);

    const bool success = rpgm_write(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        strtoffi(outputPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        projectSettings->flags,
        selection,
        strtoffi(readEncodingUtf8),
        strtoffi(writeEncodingUtf8),
        &elapsed
    );

    return success ? WriteResult{ elapsed } : WriteResult{ Err(rpgm_error()) };
}

auto TaskWorker::extractArchive(const QString& archivePath, const QString& folder, const QString& archiveEncoding)
    -> ExtractResult {
    const QByteArray archivePathUtf8 = archivePath.toUtf8();
    const QByteArray folderUtf8 = folder.toUtf8();
    const QByteArray archiveEncodingUtf8 = archiveEncoding.toUtf8();

    const bool success =
        rpgm_extract_archive(strtoffi(archivePathUtf8), strtoffi(folderUtf8), strtoffi(archiveEncodingUtf8));

    return success ? ExtractResult{} : ExtractResult{ Err(rpgm_error()) };
}

auto TaskWorker::purge(const Selected selected, const bool createIgnore) -> PurgeResult {
    const QByteArray sourcePathUtf8 = projectSettings->actualSourcePath().toUtf8();
    const QByteArray translationPathUtf8 = projectSettings->translationPath().toUtf8();

    const QByteArray readEncodingUtf8 = projectSettings->readEncoding.toUtf8();
    const QByteArray writeEncodingUtf8 = projectSettings->writeEncoding.toUtf8();

    BaseFlags flags = projectSettings->flags;

    if (createIgnore) {
        flags |= BaseFlags_CreateIgnore;
    }

    const bool success = rpgm_purge(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        flags,
        selected.forEngine(projectSettings->engineType),
        strtoffi(readEncodingUtf8),
        strtoffi(writeEncodingUtf8)
    );

    return success ? PurgeResult{} : PurgeResult{ Err(rpgm_error()) };
}

template <typename AttemptFn>
auto TaskWorker::processFilesTrackingSkips(const TaskToken& task, vector<FilenameArray>& filenames, AttemptFn&& attempt)
    -> u32 {
    u32 skippedCount = 0;

    for (const auto [idx, filenameArray] : views::enumerate(filenames)) {
        if (task->interrupted()) {
            break;
        }

        if (!attempt(filenameArray)) {
            std::swap(filenames[idx], filenames[skippedCount++]);
        }
    }

    return skippedCount;
}

template <typename ClosureFn>
auto TaskWorker::runLockedModify(QL1SV filename, ClosureFn&& closure) -> bool {
    emit lockFile(filename);
    return modifyFile(filename, *mapSections, projectSettings, std::forward<ClosureFn>(closure));
}

auto TaskWorker::search(
    const TaskToken& task,
    const SearchAction action,
    const Selected selected,
    const QString& searchText,
    SearchLocation searchLocation,
    const i8 searchColumnIndex,
    const SearchFlags searchFlags,
    const u16 tabCount
) -> SearchResult {
    if (action == SearchAction::Replace) {
        searchLocation &= ~SearchLocation::Source;
    }

    if (action == SearchAction::Put) {
        searchLocation &= ~SearchLocation::Translation;
    }

    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;

    if ((searchFlags & SearchFlags::CaseSensitive) == 0) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    QString pattern;

    if ((searchFlags & SearchFlags::RegExp) != 0) {
        pattern = searchText;
    } else {
        pattern = QRegularExpression::escape(searchText);
    }

    if ((searchFlags & SearchFlags::WholeWord) != 0) {
        // Not `\b`: it fails next to a non-word character, and wrapping an alternation needs the group
        pattern = u"(?<!\\w)(?:"_s % pattern % u")(?!\\w)";
    }

    const auto regexp = QRegularExpression(pattern, options);

    CellMatches searchMatches;
    searchMatches.reserve(tabCount);

    // The lines of the file being searched, for the entry name of a result
    const QSVList* fileLinesView = nullptr;
    const bool collectResultText = action == SearchAction::Search;
    const bool collectEntryNames = collectResultText && settings->appearance.displayEntryNames;

    // The matches of the cell being searched, reused by every cell
    vector<TextMatch> foundMatches;

    const auto closure = [&, searchFlags, searchColumnIndex, action](
                             const FilenameArray filename,
                             const QStringView line,
                             const u32 rowIndex,
                             const u32 columnIndex
                         ) -> auto {
        const auto matches = regexp.globalMatchView(line);

        if (!matches.hasNext()) {
            return;
        }

        const MatchIndex matchIndex(rowIndex, action == SearchAction::Put ? searchColumnIndex : columnIndex);

        foundMatches.clear();

        for (const auto& match : matches) {
            if ((searchFlags & SearchFlags::Put) != 0) {
                const u32 start = match.capturedStart();
                const u32 length = match.capturedLength();

                if (start == 0 && length == line.size()) {
                    foundMatches.emplace_back(0, 0, false);
                } else {
                    break;
                }
            } else {
                // A match that is empty (`(?=x)`) would insert text between characters when replaced
                if (match.capturedLength() == 0) {
                    continue;
                }

                for (i32 idx = 0; idx < match.lastCapturedIndex() + 1; idx++) {
                    // A group that took no part in the match is kept empty, so the numbers of the groups after it hold
                    if (match.capturedStart(idx) < 0) {
                        foundMatches.emplace_back(0, 0, true);
                        continue;
                    }

                    foundMatches.emplace_back(match.capturedStart(idx), match.capturedLength(idx), idx > 0);
                }
            }
        }

        // A cell with nothing to put (the text is not the whole cell) is no result
        if (foundMatches.empty()) {
            return;
        }

        auto cell = CellMatch{
            .matchStart = 0,
            .matchCount = 0,
            .textStart = 0,
            .textLength = 0,
            .entryIndex = NO_ENTRY,
            .matchIndex = matchIndex,
        };

        FileMatches& fileMatches = searchMatches.emplace(filename, FileMatches{}).first->second;

        fileMatches.setMatches(cell, foundMatches);

        if (collectResultText) {
            fileMatches.setText(cell, line);
        }

        if (collectEntryNames) {
            fileMatches.setEntry(cell, entryNameFor(*fileLinesView, rowIndex));
        }

        fileMatches.cells.push_back(std::move(cell));
    };

    auto filenames = selected.filenames(projectSettings->engineType);

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        u32 done = 0;
        const auto filename = QL1SV(filenameArray.data());
        const auto result = fileLines(filename, *mapSections, projectSettings);

        if (!result) {
            return false;
        }

        const auto lines = result.value().lines;
        fileLinesView = &lines;

        // Reused by every row
        vector<ToProcess> toProcess;

        for (const auto [rowIndex, line] : views::enumerate(lines)) {
            if (task->interrupted()) {
                break;
            }

            if (isProtectedComment(line) && !line.startsWith(BOOKMARK_COMMENT)) {
                if ((searchFlags & SearchFlags::Comment) != 0) {
                    closure(filenameArray, line, rowIndex, 0);
                }

                continue;
            }

            const auto parts = lineParts(line, 0, filename);

            if (parts.isEmpty()) {
                continue;
            }

            const auto source = getSource(parts);

            toProcess.clear();

            if ((searchLocation & SearchLocation::Source) != 0) {
                toProcess.emplace_back(source, 0);
            }

            if ((searchLocation & SearchLocation::Translation) != 0) {
                if (searchColumnIndex == -1) {
                    for (const auto [idx, translation] : views::enumerate(views::drop(parts, 1))) {
                        if (translation.empty()) {
                            continue;
                        }

                        toProcess.emplace_back(translation, idx + 1);
                    }
                } else if (searchColumnIndex == 0) {
                    const auto translation = getTranslation(parts);

                    toProcess.emplace_back(translation.getTranslation(), translation.getIndex());
                } else if (searchColumnIndex < parts.size()) {
                    // A row can have fewer columns than the one searched
                    toProcess.emplace_back(parts[searchColumnIndex], searchColumnIndex);
                }
            }

            for (const auto entryToProcess : toProcess) {
                // The copy is only needed when the cell has line breaks to turn into line feeds
                if (entryToProcess.text.contains(NEW_LINE)) {
                    const auto normalized = qsvReplace(entryToProcess.text, NEW_LINE, LINE_FEED);

                    closure(filenameArray, normalized, rowIndex, entryToProcess.columnIndex);
                } else {
                    closure(filenameArray, entryToProcess.text, rowIndex, entryToProcess.columnIndex);
                }
            }

            // Every emit queues an event for the GUI thread, so report in batches rather than per row.
            if ((++done % PROGRESS_STEP) == 0) {
                emit taskProgress(task->id(), svtostr(QL1SV(filenameArray.data())), done, scast<u32>(lines.size()));
            }
        }

        if (const auto found = searchMatches.find(filenameArray); found != searchMatches.end()) {
            found->second.trim();
        }

        return true;
    });

    return { .matches = std::move(searchMatches), .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::lintFiles(const TaskToken& task, const Selected selected, const Glossary& glossary)
    -> GlobalLintResult {
    struct LintFile {
        FileLines content;
        QString name;
    };

    auto filenames = selected.filenames(projectSettings->engineType);
    const auto customLints = customLintMatcher();
    const PreparedGlossary preparedGlossary = prepareGlossary(glossary);

    vector<LintFile> files;
    files.reserve(filenames.size());

    vector<pair<u32, u32>> rowRefs;

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        const auto filename = QL1SV(filenameArray.data());
        auto result = fileLines(filename, *mapSections, projectSettings);

        if (!result) {
            return false;
        }

        rowRefs.reserve(rowRefs.size() + result.value().lines.size());

        for (u32 line = 0; line < result.value().lines.size(); line++) {
            rowRefs.emplace_back(scast<u32>(files.size()), line);
        }

        files.push_back({ .content = std::move(result.value()), .name = filename.toString() });

        return true;
    });

    const bool parallel = (settings->translation.lintFlags & LintFlags::LanguageTool) == 0;
    vector<vector<LintRow>> outcomes(rowRefs.size());

    atomicU32 linted = 0;
    const u32 totalRows = scast<u32>(rowRefs.size());

    // Each slot is written by exactly one call, and its position in `outcomes` is the row's index in `rowRefs`.
    const auto lintRow = [&](vector<LintRow>& slot) -> void {
        // blockingMap cannot `break`, so the rest of the rows are skipped instead.
        if (task->interrupted()) {
            return;
        }

        const usize idx = scast<usize>(&slot - outcomes.data());
        const auto [fileIdx, lineIdx] = rowRefs[idx];
        const LintFile& file = files[fileIdx];

        // Every emit queues an event for the GUI thread, so report in batches rather than per row.
        if (const u32 current = ++linted; (current % PROGRESS_STEP) == 0) {
            emit taskProgress(task->id(), file.name, current, totalRows);
        }

        const auto parts = lineParts(file.content.lines[lineIdx], lineIdx, file.name);

        if (parts.isEmpty()) {
            return;
        }

        // Comments (`<!>ID`, `<!>NAME`, bookmarks...) hold no text of the game; the map display name is text, without
        // its prefix
        const optional<QStringView> source = translatableSource(getSource(parts));

        if (!source) {
            return;
        }

        const auto translation = getTranslation(parts).getTranslation();

        // An untranslated line has nothing to check
        if (translation.isEmpty()) {
            return;
        }

        auto outcome = lint(file.name, lineIdx + 1, *source, translation, preparedGlossary, customLints, false);

        slot = std::move(outcome.rows);
    };

    // The LanguageTool check talks to a server, so it stays on one thread to not flood it with requests.
    if (parallel) {
        QtConcurrent::blockingMap(outcomes, lintRow);
    } else {
        ranges::for_each(outcomes, lintRow);
    }

    usize rowCount = 0;
    for (const auto& outcome : outcomes) {
        rowCount += outcome.size();
    }

    vector<LintRow> rows;
    rows.reserve(rowCount);

    for (auto& outcome : outcomes) {
        rows.insert(rows.end(), std::make_move_iterator(outcome.begin()), std::make_move_iterator(outcome.end()));
    }

    return { .rows = std::move(rows), .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::projectStatistics(const TaskToken& task, const vector<FilenameArray>& filenames) -> StatisticsResult {
    const u32 total = scast<u32>(filenames.size());
    atomicU32 done = 0;

    // Files are independent, so they are counted in parallel; the segmentation data is shared and read-only.
    const auto statisticsOf =
        [this, &task, &done, total](const FilenameArray& filenameArray) -> optional<FileStatistics> {
        if (task->interrupted()) {
            return nullopt;
        }

        const auto filename = QL1SV(filenameArray.data());
        const auto result = fileLines(filename, *mapSections, projectSettings);

        if (!result) {
            return nullopt;
        }

        QString source;
        QString translation;
        source.reserve(result.value().content.size());
        translation.reserve(result.value().content.size());
        u32 counted = 0;

        for (const auto [idx, line] : views::enumerate(result.value().lines)) {
            const QSVList parts = lineParts(line, idx, filename);

            if (parts.isEmpty()) {
                continue;
            }

            if (const auto text = translatableSource(parts[0])) {
                source += *text;
                source += u'\n';

                if (parts.size() > 1) {
                    translation += parts[1];
                }

                translation += u'\n';
                counted++;
            }
        }

        FileStatistics statistics = { .filename = filenameArray,
                                      .source = countText(source, counted),
                                      .translation = countText(translation, counted) };

        emit taskProgress(task->id(), svtostr(filename), ++done, total);

        return statistics;
    };

    vector<optional<FileStatistics>> outcomes(filenames.size());

    // Each slot is written by exactly one call, and its position in `outcomes` is the file's index in `filenames`.
    QtConcurrent::blockingMap(outcomes, [&](optional<FileStatistics>& slot) -> void {
        slot = statisticsOf(filenames[scast<usize>(&slot - outcomes.data())]);
    });

    StatisticsResult result;
    result.files.reserve(outcomes.size());

    for (const auto [idx, outcome] : views::enumerate(outcomes)) {
        if (outcome) {
            result.files.push_back(*outcome);
        } else if (!task->interrupted()) {
            result.skipped.push_back(filenames[idx]);
        }
    }

    return result;
}

auto TaskWorker::matchGlossaryTerm(
    const QString& filename,
    const u32 lineNumber,
    const QStringView source,
    const QStringView translation,
    const GlossaryRow& glossaryRow,
    const PreparedTerm& prepared
) -> optional<LintRow> {
    const Term& term = *prepared.term;

    ByteBuffer matches;

    const bool success = rpgm_find_all_matches(
        strtoffi(glossaryRow.source),
        strtoffi(prepared.termUtf8),
        term.sourceMatchMode,
        strtoffi(glossaryRow.translation),
        strtoffi(prepared.translationUtf8),
        term.translationMatchMode,
        strtoffi(glossaryRow.sourceLang),
        strtoffi(glossaryRow.translationLang),
        &matches
    );

    if (!success) {
        qWarning().noquote() << u"Glossary matching failed for file %1: %2"_qsv.arg(
            filename,
            ffitostr(rpgm_error()).toString()
        );
        return nullopt;
    }

    if (matches.len == 0) {
        rpgm_buffer_free(matches);
        return nullopt;
    }

    const u8* ptr = matches.ptr;

    const u32 sourceCount = *rcast<const u32*>(ptr);
    ptr += sizeof(u32);

    const auto sourceMatches = span(rcast<const SearchMatch*>(ptr), sourceCount);
    ptr += sourceCount * sizeof(SearchMatch);

    const u32 translationCount = *rcast<const u32*>(ptr);
    ptr += sizeof(u32);

    const auto translationMatches = span(rcast<const SearchMatch*>(ptr), translationCount);

    QStringList sourceMatchDescs;
    vector<Span> sourceHighlights;

    sourceMatchDescs.reserve(sourceCount);
    sourceHighlights.reserve(sourceCount);

    // The matches count characters, a `QString` counts UTF-16 units: they differ after a character outside the BMP
    const auto utf16Span = [](const QStringView text, const u32 start, const u32 len) -> Span {
        if (ranges::none_of(text, [](const QChar chr) -> bool { return chr.isSurrogate(); })) {
            return { .start = start, .len = len };
        }

        const auto offsetOf = [text](u32 charCount) -> u32 {
            u32 pos = 0;

            for (; charCount > 0 && pos < scast<u32>(text.size()); charCount--) {
                const bool pair =
                    text[pos].isHighSurrogate() && pos + 1 < scast<u32>(text.size()) && text[pos + 1].isLowSurrogate();
                pos += pair ? 2 : 1;
            }

            return pos;
        };

        const u32 begin = offsetOf(start);
        return { .start = begin, .len = offsetOf(start + len) - begin };
    };

    for (const auto match : sourceMatches) {
        sourceMatchDescs.append(match.score == 0.0F ? tr("Exact") : tr("Fuzzy (%1)").arg(qsv(qftos(match.score, 3))));
        sourceHighlights.push_back(utf16Span(source, match.start, match.len));
    }

    QStringList translationMatchDescs;
    vector<Span> translationHighlights;

    translationMatchDescs.reserve(translationCount);
    translationHighlights.reserve(translationCount);

    for (const auto match : translationMatches) {
        translationMatchDescs.append(
            match.score == 0.0F ? tr("Exact") : tr("Fuzzy (%1)").arg(qsv(qftos(match.score, 3)))
        );
        translationHighlights.push_back(utf16Span(translation, match.start, match.len));
    }

    QString info;
    const bool matched = sourceCount <= translationCount;

    if (translation.isEmpty()) {
        info = tr("Translation is empty.");
    } else if (translationCount == 0) {
        info = tr("Term translation is not present.");
    } else if (!matched) {
        info = tr("Number of term occurrences doesn't match the number of translation occurrences.");
    } else {
        info = tr("Match.");
    }

    info += u'\n';
    info += tr("Term: %1, %n occurrence(s): %2", nullptr, scast<i32>(sourceCount))
                .arg(term.term, sourceMatchDescs.join(u", "));
    info += u'\n';
    info += tr("Translation: %1, %n occurrence(s): %2", nullptr, scast<i32>(translationCount))
                .arg(term.translation, translationMatchDescs.join(u", "));

    auto row = LintRow{
        .filename = filename,
        .sourceText = qsvReplace(source, u'\n', LINE_SEPARATOR),
        .translationText = qsvReplace(translation, u'\n', LINE_SEPARATOR),
        .sourceHighlights = std::move(sourceHighlights),
        .translationHighlights = std::move(translationHighlights),
        .info = std::move(info),
        .lineNumber = lineNumber,
    };

    rpgm_buffer_free(matches);

    return row;
}

auto prepareGlossary(const Glossary& glossary) -> PreparedGlossary {
    PreparedGlossary prepared;
    prepared.reserve(glossary.size());

    for (const Term& term : glossary) {
        if (!term.enabled || term.term.isEmpty() || term.translation.isEmpty()) {
            continue;
        }

        prepared.push_back(
            { .term = &term, .termUtf8 = term.term.toUtf8(), .translationUtf8 = term.translation.toUtf8() }
        );
    }

    return prepared;
}

#ifdef ENABLE_NUSPELL
static const QRegularExpression MISSPELLED_WORD_REGEXP =
    QRegularExpression(uR"(\b[\p{L}']+\b)"_s, QRegularExpression::UseUnicodePropertiesOption);
#endif

auto TaskWorker::customLintMatcher() const -> CustomLintMatcher {
    const auto& customLints = settings->translation.customLints;

    CustomLintMatcher matcher;
    matcher.indices.reserve(customLints.size());

    isize patternLength = 0;

    for (const auto& [idx, customLint] : views::enumerate(customLints)) {
        if (customLint.disabled) {
            continue;
        }

        patternLength += customLint.sequence.size() + (customLint.caseInsensitive ? 13 : 10) + 1;
        matcher.indices.push_back(scast<i32>(idx));
    }

    if (matcher.indices.empty()) {
        return matcher;
    }

    QString pattern;
    pattern.reserve(patternLength);

    for (const auto originalIdx : matcher.indices) {
        const auto& customLint = customLints[originalIdx];

        if (!pattern.isEmpty()) {
            pattern += u'|';
        }

        // Named, so these stay the only capturing groups (see `DontCaptureOption` below): group N is lint N - 1
        pattern += u"(?<l" % qsv(qitos(originalIdx)) % u'>';

        if (customLint.caseInsensitive) {
            pattern += u"?i:";
        }

        pattern += customLint.sequence;
        pattern += u')';
    }

    // A user's own `( )` groups must not shift the numbering
    matcher.regexp = QRegularExpression(pattern, QRegularExpression::DontCaptureOption);

    return matcher;
}

auto TaskWorker::lint(
    const QString& filename,
    const u32 lineNumber,
    const QStringView source,
    const QStringView translation,
    const PreparedGlossary& glossary,
    const CustomLintMatcher& customLints,
    const bool highlight
) -> LintOutcome {
    const i32 size = scast<i32>(translation.size());

    vector<LintRow> rows;

    if ((settings->translation.lintFlags & LintFlags::TermMismatch) != 0 && !projectSettings->sourceLang.isEmpty() &&
        !projectSettings->translationLang.isEmpty() && !glossary.empty()) {
        const auto glossaryRow = GlossaryRow{ .source = source.toUtf8(),
                                              .translation = translation.toUtf8(),
                                              .sourceLang = projectSettings->sourceLang.toUtf8(),
                                              .translationLang = projectSettings->translationLang.toUtf8() };

        for (const PreparedTerm& prepared : glossary) {
            if (auto row = matchGlossaryTerm(filename, lineNumber, source, translation, glossaryRow, prepared)) {
                rows.push_back(std::move(*row));
            }
        }
    }

    if ((settings->translation.lintFlags & LintFlags::TagMismatch) != 0) {
        vector<QString> sourceTags;
        vector<QString> translationTags;

        vector<Span> sourceHighlights;
        vector<Span> translationHighlights;

        static constexpr array<const QRegularExpression*, 4> regexps = { &TextCodes::REGEXP,
                                                                         &Notetags::REGEXP,
                                                                         &PluginCommands::REGEXP,
                                                                         &CommentTags::REGEXP };

        // The first group of each match that took part in it is the tag
        const auto collectTags = [](const QRegularExpression& regexp,
                                    const QStringView text,
                                    vector<QString>& tags,
                                    vector<Span>& highlights) -> void {
            for (const auto& match : regexp.globalMatchView(text)) {
                for (i32 idx = 1; idx < match.lastCapturedIndex() + 1; idx++) {
                    if (match.capturedLength(idx) > 0) {
                        tags.emplace_back(match.captured(idx));
                        highlights.emplace_back(
                            scast<i32>(match.capturedStart(idx)),
                            scast<i32>(match.capturedLength(idx))
                        );
                        break;
                    }
                }
            }
        };

        for (const QRegularExpression* regexp : regexps) {
            collectTags(*regexp, source, sourceTags, sourceHighlights);
            collectTags(*regexp, translation, translationTags, translationHighlights);
        }

        if (sourceTags != translationTags) {
            rows.push_back(
                LintRow{
                    .filename = filename,
                    .sourceText = source.toString(),
                    .translationText = translation.toString(),
                    .sourceHighlights = std::move(sourceHighlights),
                    .translationHighlights = std::move(translationHighlights),
                    .info = tr("Tags do not match between source and translation."),
                    .lineNumber = lineNumber,
                }
            );
        }
    }

    vector<LintCharState> lintCharStates;

    if (highlight) {
        lintCharStates.resize(size);
    }

    const auto addEntry = [&](const i32 start, const i32 len, const LintEntry& entry, const bool showable) -> void {
        // A position outside the text (a LanguageTool offset, a stale match) can't be shown
        if (start < 0 || start >= size) {
            return;
        }

        if (highlight) {
            for (i32 i = start; i < start + len && i < size; i++) {
                lintCharStates[i].entries.push_back(entry);
            }
        }

        if (showable) {
            vector<Span> translationHighlights;
            translationHighlights.emplace_back(start, len);

            rows.push_back(
                LintRow{
                    .filename = filename,
                    .sourceText = u"-"_s,
                    .translationText = translation.toString(),
                    .translationHighlights = std::move(translationHighlights),
                    .info = entry.translationId != nullptr ? QCoreApplication::translate("Lint", entry.translationId)
                                                           : entry.extra.toString(),
                    .lineNumber = lineNumber,
                }
            );
        }
    };

    const auto setBackground = [&](const i32 start, const i32 len, const QColor color) -> void {
        if (!highlight) {
            return;
        }

        for (i32 i = start; i < start + len && i < size; i++) {
            lintCharStates[i].background = color;
        }
    };

    const auto setUnderline =
        [&](const i32 start, const i32 len, const QTextCharFormat::UnderlineStyle style, const QColor color) -> void {
        if (!highlight) {
            return;
        }

        for (i32 i = start; i < start + len && i < size; i++) {
            lintCharStates[i].underlineStyle = style;
            lintCharStates[i].underlineColor = color;
        }
    };

    const auto setForeground = [&](const i32 start, const i32 len, const QColor color) -> void {
        if (!highlight) {
            return;
        }

        for (i32 i = start; i < start + len && i < size; i++) {
            lintCharStates[i].foreground = color;
        }
    };

#ifdef ENABLE_NUSPELL
    if ((settings->translation.lintFlags & LintFlags::Spellcheck) != 0 && dictionary) {
        const auto matches = MISSPELLED_WORD_REGEXP.globalMatchView(translation);

        for (const auto& match : matches) {
            const QString word = match.captured();
            const QByteArray utf8Word = word.toUtf8();
            const bool misspelled = !dictionary->spell(string_view(utf8Word.data(), utf8Word.size()));

            if (misspelled) {
                const i32 start = scast<i32>(match.capturedStart());
                const i32 len = scast<i32>(match.capturedLength());

                // Slow, and only shown for the cell being edited: a lint of files has no use for them
                QStringList qsuggestions;

                if (highlight) {
                    vector<string> suggestions;
                    dictionary->suggest(string_view(utf8Word.data(), utf8Word.size()), suggestions);

                    qsuggestions.reserve(scast<isize>(suggestions.size()));
                    for (const auto& suggestion : suggestions) {
                        qsuggestions.emplace_back(QString::fromUtf8(suggestion));
                    }
                }

                setUnderline(start, len, QTextCharFormat::SpellCheckUnderline, Qt::red);
                addEntry(
                    start,
                    len,
                    LintEntry{
                        .suggestions = QVariant(qsuggestions),
                        .captured = word,
                        .translationId = MISC_LINTS[4],
                        .index = 4,
                        .type = LintType::Misc,
                    },
                    true
                );
            }
        }
    }
#endif

    // The space and the tab always count; the setting adds the others (ideographic space, no-break space...)
    const auto isWhitespace = [this](const u32 codePoint) -> bool {
        return codePoint == U' ' || codePoint == U'\t' ||
               ranges::contains(settings->translation.whitespaceCharacters, codePoint);
    };

    i32 lead = 0;

    if ((settings->translation.lintFlags & LintFlags::LeadingWhitespace) != 0) {
        while (lead < size && isWhitespace(codePointAt(translation, lead))) {
            lead++;
        }

        if (lead > 0) {
            setBackground(0, lead, RED_HIGHLIGHT);
            addEntry(
                0,
                lead,
                LintEntry{
                    .translationId = MISC_LINTS[3],
                    .index = 3,
                    .type = LintType::Misc,
                },
                true
            );
        }
    }

    if ((settings->translation.lintFlags & LintFlags::TrailingWhitespace) != 0) {
        i32 lastNonSpace = size - 1;

        while (lastNonSpace >= 0 && isWhitespace(codePointAt(translation, lastNonSpace))) {
            lastNonSpace--;
        }

        const i32 trailStart = lastNonSpace + 1;
        const i32 trailLen = size - trailStart;

        if (trailLen > 0 && trailStart >= lead) {
            setBackground(trailStart, trailLen, RED_HIGHLIGHT);
            addEntry(
                trailStart,
                trailLen,
                LintEntry{
                    .translationId = MISC_LINTS[2],
                    .index = 2,
                    .type = LintType::Misc,
                },
                true
            );
        }
    }

    if ((settings->translation.lintFlags & LintFlags::ContiguousWhitespace) != 0) {
        i32 idx = 0;

        while (idx < size) {
            if (isWhitespace(codePointAt(translation, idx))) {
                const i32 start = idx;

                while (idx < size && isWhitespace(codePointAt(translation, idx))) {
                    idx++;
                }

                const i32 len = idx - start;

                if (len >= 2) {
                    setBackground(start, len, RED_HIGHLIGHT);
                    addEntry(
                        start,
                        len,
                        LintEntry{
                            .translationId = MISC_LINTS[1],
                            .index = 1,
                            .type = LintType::Misc,
                        },
                        true
                    );
                }
            } else {
                idx++;
            }
        }
    }

    if ((settings->translation.lintFlags & LintFlags::UnclosedPunctuation) != 0) {
        struct Punctuation {
            const QChar open;
            const QChar close;
            const bool symmetric;
        };

        static constexpr array<Punctuation, 15> puncts = {
            Punctuation{ .open = u'(', .close = u')', .symmetric = false },
            { .open = u'[', .close = u']', .symmetric = false },
            { .open = u'{', .close = u'}', .symmetric = false },
            { .open = u'"', .close = u'"', .symmetric = true },
            { .open = u'\'', .close = u'\'', .symmetric = true },

            { .open = u'«', .close = u'»', .symmetric = false },
            { .open = u'‹', .close = u'›', .symmetric = false },
            { .open = u'“', .close = u'”', .symmetric = false },
            { .open = u'‘', .close = u'’', .symmetric = false },

            { .open = u'「', .close = u'」', .symmetric = false },
            { .open = u'『', .close = u'』', .symmetric = false },
            { .open = u'（', .close = u'）', .symmetric = false },
            { .open = u'【', .close = u'】', .symmetric = false },
            { .open = u'《', .close = u'》', .symmetric = false },
            { .open = u'〈', .close = u'〉', .symmetric = false },
        };

        struct StackEntry {
            i32 pos;
            QChar expectedClose;
        };

        vector<StackEntry> stack;
        stack.reserve(16);

        const auto findPunctuation = [](const QChar chr) -> optional<Punctuation> {
            for (const auto punctuation : puncts) {
                if (punctuation.open == chr || punctuation.close == chr) {
                    return punctuation;
                }
            }

            return nullopt;
        };

        // A single quote after a letter is an apostrophe, not punctuation: always between letters ("it's", "you're"),
        // and after one ("dogs'") unless it closes a quote that is currently open.
        const auto isApostrophe = [&](const i32 idx, const QChar chr) -> bool {
            if ((chr != u'\'' && chr != u'’') || idx == 0 || !translation.at(idx - 1).isLetterOrNumber()) {
                return false;
            }

            const bool nextIsLetter = idx + 1 < size && translation.at(idx + 1).isLetter();
            return nextIsLetter || stack.empty() || stack.back().expectedClose != chr;
        };

        for (i32 idx = 0; idx < size; idx++) {
            const QChar chr = translation.at(idx);
            const optional<Punctuation> punctuation = findPunctuation(chr);

            if (!punctuation || isApostrophe(idx, chr)) {
                continue;
            }

            if (punctuation->symmetric) {
                if (!stack.empty() && stack.back().expectedClose == chr) {
                    stack.pop_back();
                } else {
                    stack.push_back({ .pos = idx, .expectedClose = chr });
                }
            } else if (chr == punctuation->open) {
                stack.push_back({ .pos = idx, .expectedClose = punctuation->close });
            } else if (chr == punctuation->close) {
                if (!stack.empty() && stack.back().expectedClose == chr) {
                    stack.pop_back();
                } else {
                    setBackground(idx, 1, RED_HIGHLIGHT);
                    addEntry(
                        idx,
                        1,
                        LintEntry{
                            .translationId = MISC_LINTS[0],
                            .index = 0,
                            .type = LintType::Misc,
                        },
                        true
                    );
                }
            }
        }

        for (const auto entry : stack) {
            setBackground(entry.pos, 1, RED_HIGHLIGHT);
            addEntry(
                entry.pos,
                1,
                LintEntry{
                    .translationId = MISC_LINTS[MISC_LINT_UNCLOSED_PUNCTUATION_INDEX],
                    .index = MISC_LINT_UNCLOSED_PUNCTUATION_INDEX,
                    .type = LintType::Misc,
                },
                true
            );
        }
    }

    vector<Span> capturedMatches;

    const auto applyLints = [&](const QRegularExpression& regexp,
                                const LintType type,
                                const span<const char* const> tooltips,
                                const span<const SequenceSettings> sequenceSettings,
                                auto&& onMatch) -> auto {
        for (const auto& match : regexp.globalMatchView(translation)) {
            for (i32 idx = 1; idx < match.lastCapturedIndex() + 1; idx++) {
                if (match.capturedLength(idx) > 0) {
                    const auto& sequence = sequenceSettings[idx - 1];

                    if (sequence.disabled) {
                        break;
                    }

                    const i32 start = scast<i32>(match.capturedStart(idx));
                    const i32 len = scast<i32>(match.capturedLength(idx));

                    setForeground(
                        start,
                        len,
                        sequence.useCustomColor ? QColor::fromRgba(sequence.customColor) : DEFAULT_LINT_COLOR
                    );

#ifdef ENABLE_LANGUAGETOOL
                    capturedMatches.emplace_back(scast<u32>(start), scast<u32>(len));
#endif

                    const QString captured = match.captured(idx);

                    auto entry = LintEntry{
                        .captured = captured,
                        .translationId = tooltips[idx - 1],
                        .index = idx - 1,
                        .type = type,
                    };
                    onMatch(match, idx, entry, start, len);

                    addEntry(start, len, entry, false);
                    break;
                }
            }
        }
    };

    u16 closedColor = 0;
    u16 unclosedColor = 0;
    vector<pair<i32, i32>> unclosedColorTags;

    applyLints(
        TextCodes::REGEXP,
        LintType::TextCode,
        TextCodes::TOOLTIPS,
        settings->translation.textCodes,
        [&](const auto& /* match */, const i32 idx, LintEntry& lint, const i32 start, const i32 len) -> auto {
        switch (idx - 1) {
            case 31: {
                const optional<u32> parsedColorIndex = stoa<u32>(QStringView(lint.captured).sliced(3).chopped(1));

                // A color picked by a variable (like a nested variable code) is only known while the game runs.
                if (!parsedColorIndex) {
                    break;
                }

                const u32 colorIndex = *parsedColorIndex;

                const auto colors = getWindowColors(projectSettings);

                if (colors && (colorIndex >= colors.value().size() || qAlpha(colors.value()[colorIndex]) == 0)) {
                    lint.extra = tr("There is no color with this number.");
                } else if (colors) {
                    if (colorIndex == 0) {
                        closedColor++;
                        if (!unclosedColorTags.empty()) {
                            unclosedColorTags.pop_back();
                        }
                    } else {
                        unclosedColor++;
                        unclosedColorTags.emplace_back(start, len);
                    }

                    const auto color = colors.value()[colorIndex];
                    setForeground(start, len, QColor(color));
                    lint.extra = color;
                } else {
                    lint.extra = colors.error().translated();
                }
                break;
            }
            case 110: {
                const QStringView rgbHex = QStringView(lint.captured).sliced(4).chopped(1);
                const QRgb color = stoa<u32>(rgbHex, 16).value_or(0);

                if (QColor(color) == QColor(Qt::white)) {
                    closedColor++;
                    if (!unclosedColorTags.empty()) {
                        unclosedColorTags.pop_back();
                    }
                } else {
                    unclosedColor++;
                    unclosedColorTags.emplace_back(start, len);
                }

                setForeground(start, len, QColor(color));
                lint.extra = color;
                break;
            }
            default:
                break;
        }
    }
    );

    if (closedColor != unclosedColor) {
        for (const auto& [start, len] : unclosedColorTags) {
            setUnderline(start, len, QTextCharFormat::WaveUnderline, QColor(Qt::yellow));
            addEntry(
                start,
                len,
                LintEntry{
                    .captured = QString(),
                    .translationId = MISC_LINTS[MISC_LINT_UNCLOSED_COLOR_INDEX],
                    .index = MISC_LINT_UNCLOSED_COLOR_INDEX,
                    .type = LintType::Misc,
                },
                true
            );
        }
    }

    applyLints(
        Notetags::REGEXP,
        LintType::NoteTag,
        Notetags::TOOLTIPS,
        settings->translation.notetags,
        [](const auto& match, const i32 idx, LintEntry& entry, const i32 start, const i32 len) -> auto {}
    );
    applyLints(
        PluginCommands::REGEXP,
        LintType::PluginCommand,
        PluginCommands::TOOLTIPS,
        settings->translation.pluginCommands,
        [](const auto& match, const i32 idx, LintEntry& entry, const i32 start, const i32 len) -> auto {}
    );
    applyLints(
        CommentTags::REGEXP,
        LintType::CommentTag,
        CommentTags::TOOLTIPS,
        settings->translation.commentTags,
        [](const auto& match, const i32 idx, LintEntry& entry, const i32 start, const i32 len) -> auto {}
    );

    if (!customLints.indices.empty()) {
        for (const auto& match : customLints.regexp.globalMatchView(translation)) {
            for (i32 idx = 1; idx < match.lastCapturedIndex() + 1; idx++) {
                if (match.capturedLength(idx) == 0) {
                    continue;
                }

                const i32 originalIdx = customLints.indices[idx - 1];
                const auto& customLint = settings->translation.customLints[originalIdx];
                const i32 start = scast<i32>(match.capturedStart(idx));
                const i32 len = scast<i32>(match.capturedLength(idx));

                if (customLint.useCustomColor) {
                    setForeground(start, len, QColor::fromRgba(customLint.customColor));
                }

                addEntry(
                    start,
                    len,
                    LintEntry{
                        .extra = customLint.tooltip,
                        .captured = match.captured(idx),
                        .translationId = nullptr,
                        .index = originalIdx,
                        .type = LintType::Custom,
                    },
                    false
                );
                break;
            }
        }
    }

#ifdef ENABLE_LANGUAGETOOL
    if ((settings->translation.lintFlags & LintFlags::LanguageTool) != 0) {
        if (projectSettings->translationLang.isEmpty() || settings->translation.languageTool.baseURL.isEmpty()) {
            rows.push_back(
                LintRow{
                    .filename = filename,
                    .sourceText = u"-"_s,
                    .translationText = translation.toString(),
                    .info =
                        projectSettings->translationLang.isEmpty()
                            ? tr("LanguageTool check skipped: set the translation language in settings to enable it.")
                            : tr("LanguageTool check skipped: set the LanguageTool base URL in settings to enable it."),
                    .lineNumber = lineNumber,
                }
            );
        } else if (const auto result = languageToolLint(translation, capturedMatches); !result) {
            rows.push_back(
                LintRow{
                    .filename = filename,
                    .sourceText = u"-"_s,
                    .translationText = translation.toString(),
                    .info = tr("LanguageTool check failed: %1").arg(ffitostr(result.error()).toString()),
                    .lineNumber = lineNumber,
                }
            );
        } else {
            const FFIString outJSON = result.value();
            const QString json = QString::fromUtf8(outJSON.ptr, outJSON.len);
            rpgm_string_free(outJSON);

            vector<LTMatch> matches;
            glz::context ctx{};
            const auto parseError =
                glz::read<glz::opts{ .error_on_unknown_keys = false }>(matches, json.toStdString(), ctx);

            if (parseError) {
                rows.push_back(
                    LintRow{
                        .filename = filename,
                        .sourceText = u"-"_s,
                        .translationText = translation.toString(),
                        .info = tr("Failed to parse LanguageTool response."),
                        .lineNumber = lineNumber,
                    }
                );
            } else {
                for (const auto& ltMatch : matches) {
                    const i32 start = scast<i32>(ltMatch.offset);
                    const i32 len = scast<i32>(ltMatch.length);

                    QStringList suggestions;
                    suggestions.reserve(scast<isize>(ltMatch.replacements.size()));
                    for (const auto& replacement : ltMatch.replacements) {
                        suggestions.push_back(replacement.value);
                    }

                    setUnderline(start, len, QTextCharFormat::SpellCheckUnderline, QColor(Qt::red));
                    addEntry(
                        start,
                        len,
                        LintEntry{
                            .extra = ltMatch.message,
                            .suggestions = QVariant(suggestions),
                            .captured = ltMatch.shortMessage.isEmpty() ? ltMatch.rule.id : ltMatch.shortMessage,
                            .translationId = nullptr,
                            .index = 0,
                            .type = LintType::LanguageToolCheck,
                        },
                        true
                    );
                }
            }
        }
    }
#endif

    return { .rows = std::move(rows), .charStates = std::move(lintCharStates) };
}

auto TaskWorker::performBatchAction(
    const TaskToken& task,
    const JSScript& script,
    const Selected selected,
    const u8 columnIndex,
    const BatchVariant& variant,
    const Glossary& glossary
) -> ScriptResult {
    auto filenames = selected.filenames(projectSettings->engineType);

    return runBatchScript(task, script, filenames, columnIndex, variant, glossary);
}

auto TaskWorker::runBatchScript(
    const TaskToken& task,
    const JSScript& script,
    vector<FilenameArray>& filenames,
    const u8 columnIndex,
    const BatchVariant& variant,
    const Glossary& glossary
) -> ScriptResult {
    auto deadline = QDeadlineTimer(SCRIPT_TIMEOUT);
    auto runtime = ScriptRuntime(deadline, task.get(), script.hostApi);
    JSContext* const ctx = runtime.ctx;

    QStringList log;
    runtime.host.log = &log;

    const QByteArray src = script.source.toUtf8();
    const JSValue evalResult = JS_Eval(ctx, src.constData(), src.size(), "<user-script>", JS_EVAL_TYPE_GLOBAL);

    if (JS_IsException(evalResult)) {
        JS_FreeValue(ctx, evalResult);
        return Err(ScriptError{ .message = jsGetExceptionText(ctx), .filename = {}, .lineNumber = 0 });
    }

    JS_FreeValue(ctx, evalResult);

    const JSValue global = JS_GetGlobalObject(ctx);
    auto callback = JSValueGuard(ctx, JS_GetPropertyStr(ctx, global, "rpgmLineCallback"));
    auto fileCallback = JSValueGuard(ctx, JS_GetPropertyStr(ctx, global, "rpgmFileCallback"));
    JS_FreeValue(ctx, global);

    // rpgmFileCallback sees a whole file at once, so a script can batch lines into one call.
    const bool fileMode = JS_IsFunction(ctx, fileCallback);
    const bool lineMode = JS_IsFunction(ctx, callback);

    if (fileMode && lineMode) {
        return Err(
            ScriptError{
                .message = tr("Script must define only one of rpgmLineCallback and rpgmFileCallback, not both"),
                .filename = {},
                .lineNumber = 0,
            }
        );
    }

    if (!fileMode && !lineMode) {
        return Err(
            ScriptError{
                .message = tr(
                    "Script must define function rpgmLineCallback(sourceText, translations, filename, lineNumber, options) or rpgmFileCallback(filename, rows, options)"
                ),
                .filename = {},
                .lineNumber = 0,
            }
        );
    }

    auto options =
        JSValueGuard(ctx, makeScriptOptions(ctx, script.action, columnIndex, variant, projectSettings, glossary));

    // Overwrites `parts[1..]` with the non-null script translations. `ownedParts` keeps new strings alive while
    // `parts` views them; it is reserved up front so the views never dangle.
    const auto applyTranslations = [ctx](QSVList& parts, QStringList& ownedParts, const JSValue ret) -> void {
        if (!JS_IsArray(ret)) {
            return;
        }

        const u32 retLen = jsArrayLength(ctx, ret);
        ownedParts.reserve(parts.size());

        for (u32 i = 0; i + 1 < parts.size() && i < retLen; i++) {
            const auto element = JSValueGuard(ctx, JS_GetPropertyUint32(ctx, ret, i));

            if (!JS_IsUndefined(element) && !JS_IsNull(element)) {
                ownedParts.push_back(jsToQString(ctx, element));
                parts[i + 1] = QStringView(ownedParts.last());
            }
        }
    };

    optional<ScriptError> scriptError;
    u32 processedCount = 0;

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        if (scriptError) {
            return true;
        }

        const auto filename = QL1SV(filenameArray.data());
        u32 done = 0;

        auto closure = [&, ctx, columnIndex, filename](const QString& content, const QSVList& lines) -> QString {
            QString joined;
            joined.reserve(content.size() * 2);

            // Every emit queues an event for the GUI thread, so report in batches rather than per row.
            const auto reportRow = [&] -> void {
                if ((++done % PROGRESS_STEP) == 0) {
                    emit taskProgress(task->id(), svtostr(QL1SV(filenameArray.data())), done, scast<u32>(lines.size()));
                }
            };

            for (const auto [lineIdx, line] : views::enumerate(lines)) {
                // A file the script did not finish is left as it was, as with `rpgmFileCallback`
                if (task->interrupted()) {
                    return joinQSVList(lines, u'\n');
                }

                QSVList parts = lineParts(line, lineIdx, filename);

                // A line that can't be split is not text of the game: it is kept as it is
                if (parts.isEmpty()) {
                    joined += line;
                    joined += u'\n';
                    continue;
                }

                const QStringView sourceText = parts[0];

                const bool protectedComment = isProtectedComment(sourceText);

                if (!protectedComment && parts.size() <= columnIndex) {
                    joined += joinQSVList(parts, SEPARATOR);
                    joined += u'\n';
                    continue;
                }

                const JSValue translations = JS_NewArray(ctx);
                u32 arrIdx = 0;

                for (u32 i = 1; i < parts.size(); i++) {
                    JS_SetPropertyUint32(ctx, translations, arrIdx++, qstringToJS(ctx, parts[i]));
                }

                array<JSValue, SCRIPT_CALLBACK_ARG_COUNT> args = {
                    qstringToJS(ctx, sourceText), translations,
                    qstringToJS(ctx, filename),   JS_NewInt32(ctx, scast<i32>(lineIdx)),
                    JS_DupValue(ctx, options),
                };

                runtime.deadline_ = QDeadlineTimer(SCRIPT_TIMEOUT);
                const JSValue ret = JS_Call(ctx, callback, JS_UNDEFINED, SCRIPT_CALLBACK_ARG_COUNT, args.data());

                for (auto& arg : args) {
                    JS_FreeValue(ctx, arg);
                }

                if (JS_IsException(ret)) {
                    JS_FreeValue(ctx, ret);

                    scriptError = ScriptError{ .message = jsGetExceptionText(ctx),
                                               .filename = filenameArray,
                                               .lineNumber = scast<u32>(lineIdx) + 1 };

                    return joinQSVList(lines, u'\n');
                }

                if (protectedComment) {
                    JS_FreeValue(ctx, ret);

                    joined += joinQSVList(parts, SEPARATOR);
                    joined += u'\n';

                    reportRow();
                    continue;
                }

                QStringList ownedParts;
                applyTranslations(parts, ownedParts, ret);

                JS_FreeValue(ctx, ret);

                joined += joinQSVList(parts, SEPARATOR);
                joined += u'\n';

                reportRow();
            }

            // A file with no lines has no line feed to remove
            if (!joined.isEmpty()) {
                joined.removeLast();
            }

            // The last batch of the file, and the name of a file shorter than one batch
            emit taskProgress(
                task->id(),
                svtostr(QL1SV(filenameArray.data())),
                scast<u32>(lines.size()),
                scast<u32>(lines.size())
            );

            return joined;
        };

        // One call per file. The returned array has one entry per row: null/undefined leaves the row unchanged,
        // otherwise a translations array as in rpgmLineCallback. Comment rows are visible but never rewritten.
        auto fileClosure = [&, ctx, filename](const QString& content, const QSVList& lines) -> QString {
            if (task->interrupted()) {
                return joinQSVList(lines, u'\n');
            }

            const auto rows = JSValueGuard(ctx, JS_NewArray(ctx));

            for (const auto [lineIdx, line] : views::enumerate(lines)) {
                const QSVList parts = lineParts(line, lineIdx, filename);
                const JSValue translations = JS_NewArray(ctx);

                for (u32 i = 1; i < parts.size(); i++) {
                    JS_SetPropertyUint32(ctx, translations, i - 1, qstringToJS(ctx, parts[i]));
                }

                // A line that can't be split is shown to the script as a comment: visible, never rewritten
                const JSValue row = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, row, "source", qstringToJS(ctx, parts.isEmpty() ? line : parts[0]));
                JS_SetPropertyStr(ctx, row, "translations", translations);
                JS_SetPropertyStr(ctx, row, "lineNumber", JS_NewInt32(ctx, scast<i32>(lineIdx)));
                JS_SetPropertyStr(
                    ctx,
                    row,
                    "isComment",
                    JS_NewBool(ctx, parts.isEmpty() || isProtectedComment(parts[0]))
                );
                JS_SetPropertyUint32(ctx, rows, scast<u32>(lineIdx), row);
            }

            array<JSValue, 3> args = { qstringToJS(ctx, filename), JS_DupValue(ctx, rows), JS_DupValue(ctx, options) };

            runtime.deadline_ = QDeadlineTimer(SCRIPT_TIMEOUT);
            const auto ret = JSValueGuard(ctx, JS_Call(ctx, fileCallback, JS_UNDEFINED, args.size(), args.data()));

            for (auto& arg : args) {
                JS_FreeValue(ctx, arg);
            }

            if (JS_IsException(ret)) {
                scriptError =
                    ScriptError{ .message = jsGetExceptionText(ctx), .filename = filenameArray, .lineNumber = 0 };
                return joinQSVList(lines, u'\n');
            }

            QString joined;
            joined.reserve(content.size() * 2);

            for (const auto [lineIdx, line] : views::enumerate(lines)) {
                QSVList parts = lineParts(line, lineIdx, filename);
                QStringList ownedParts;

                if (!parts.isEmpty() && !isProtectedComment(parts[0]) && JS_IsArray(ret)) {
                    const auto element = JSValueGuard(ctx, JS_GetPropertyUint32(ctx, ret, scast<u32>(lineIdx)));
                    applyTranslations(parts, ownedParts, element);
                }

                if (parts.isEmpty()) {
                    joined += line;
                } else {
                    joined += joinQSVList(parts, SEPARATOR);
                }

                joined += u'\n';
            }

            if (!joined.isEmpty()) {
                joined.removeLast();
            }

            emit taskProgress(
                task->id(),
                svtostr(QL1SV(filenameArray.data())),
                scast<u32>(lines.size()),
                scast<u32>(lines.size())
            );

            return joined;
        };

        const bool processed = fileMode ? runLockedModify(filename, fileClosure) : runLockedModify(filename, closure);

        if (processed) {
            processedCount++;
        }

        return processed;
    });

    emit lockFile(QString());

    if (scriptError) {
        return Err(std::move(*scriptError));
    }

    return ScriptSuccess{ .filenames = std::move(filenames),
                          .log = std::move(log),
                          .skippedCount = skippedCount,
                          .processedCount = processedCount };
}

auto TaskWorker::serdeExport(
    const TaskToken& task,
    vector<FilenameArray>& filenames,
    const QString& outputDir,
    const SerdeFormat format
) -> SerdeResult {
    u32 done = 0;
    const u32 total = scast<u32>(filenames.size());

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        const auto filename = QL1SV(filenameArray.data());
        emit taskProgress(task->id(), svtostr(filename), done++, total);

        const auto result = fileLines(filename, *mapSections, projectSettings);

        if (!result) {
            return false;
        }

        const QByteArray contentUtf8 = result.value().content.toUtf8();
        ByteBuffer exported;

        if (!rpgm_serde_export(strtoffi(contentUtf8), format, &exported)) {
            return false;
        }

        const QString outPath = outputDir % u'/' % filename % serdeFormatExtension(format);
        // Overwrites a previous export: QSaveFile keeps it intact unless the new one is written completely
        auto outFile = QSaveFile(outPath);
        outFile.setDirectWriteFallback(true);

        const bool valid = outFile.open(QFile::WriteOnly) &&
                           outFile.write(rcast<const char*>(exported.ptr), exported.len) == exported.len &&
                           outFile.commit();

        rpgm_buffer_free(exported);

        return valid;
    });

    return SerdeSuccess{ .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::serdeImport(
    const TaskToken& task,
    vector<FilenameArray>& filenames,
    const QString& inputDir,
    const SerdeFormat format
) -> SerdeResult {
    u32 done = 0;
    const u32 total = scast<u32>(filenames.size());

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        const auto filename = QL1SV(filenameArray.data());
        emit taskProgress(task->id(), svtostr(filename), done++, total);

        const QString inPath = inputDir % u'/' % filename % serdeFormatExtension(format);
        auto inFile = QFile(inPath);

        if (!inFile.open(QFile::ReadOnly)) {
            return false;
        }

        const QByteArray raw = inFile.readAll();
        const ByteBuffer rawBuffer = { .ptr = rcast<const u8*>(raw.constData()),
                                       .len = scast<u32>(raw.size()),
                                       .cap = 0 };

        FFIString imported;

        if (!rpgm_serde_import(rawBuffer, format, &imported)) {
            return false;
        }

        QString importedContent = QString::fromUtf8(imported.ptr, scast<isize>(imported.len));
        rpgm_string_free(imported);

        return runLockedModify(filename, [&](const QString&, const QSVList&) -> QString { return importedContent; });
    });

    emit lockFile(QString());

    return SerdeSuccess{ .filenames = std::move(filenames), .skippedCount = skippedCount };
}

namespace {
// `part` with each full match of `matches` replaced by `replaceText`. Replacement escapes: `\\`, `\``, `\'`, `\+`,
// `\1`-`\99` (backslash, before, after, last group, numbered group). Replacement positions go to `newMatches`, if
// given.
//
// `nullopt` if the matches don't fit `part`: the text changed since they were found.
[[nodiscard]] auto replaceInCell(
    const QStringView part,
    const span<const TextMatch> matchSpan,
    const QString& replaceText,
    vector<TextMatch>* const newMatches
) -> optional<QString> {
    for (const TextMatch& match : matchSpan) {
        if (scast<usize>(match.get<"start">()) + match.get<"len">() > scast<usize>(part.size())) {
            return nullopt;
        }
    }

    const auto substitute = [&](const TextMatch& fullMatch, const u32 capturedBegin, const u32 capturedEnd) -> QString {
        const QStringView beforeFull = part.sliced(0, fullMatch.get<"start">());
        const QStringView afterFull = part.sliced(fullMatch.get<"start">() + fullMatch.get<"len">());

        QStringView lastCapture;
        for (u32 k = capturedEnd; k > capturedBegin;) {
            --k;
            if (matchSpan[k].get<"capture">()) {
                lastCapture = part.sliced(matchSpan[k].get<"start">(), matchSpan[k].get<"len">());
                break;
            }
        }

        QString result;
        result.reserve(replaceText.size());

        i32 cidx = 0;

        while (cidx < replaceText.size()) {
            if (replaceText[cidx] != u'\\' || cidx + 1 >= replaceText.size()) {
                result.append(replaceText[cidx]);
                cidx++;
                continue;
            }

            const QChar next = replaceText[cidx + 1];

            if (next == u'`') {
                result.append(beforeFull);
                cidx++;
            } else if (next == u'\'') {
                result.append(afterFull);
                cidx++;
            } else if (next == u'+') {
                result.append(lastCapture);
                cidx++;
            } else if (next == u'\\') {
                result.append(u'\\');
                cidx++;
            } else if (next.isDigit() && next != u'0') {
                bool handled = false;

                if (cidx + 2 < replaceText.size() && replaceText[cidx + 2].isDigit()) {
                    const i32 twoDigit = (next.digitValue() * 10) + replaceText[cidx + 2].digitValue();
                    const u32 idx = capturedBegin + scast<u32>(twoDigit - 1);
                    if (idx < capturedEnd && matchSpan[idx].get<"capture">()) {
                        result.append(part.sliced(matchSpan[idx].get<"start">(), matchSpan[idx].get<"len">()));
                        cidx += 2;
                        handled = true;
                    }
                }

                if (!handled) {
                    const i32 oneDigit = next.digitValue();
                    const u32 idx = capturedBegin + scast<u32>(oneDigit - 1);
                    if (idx < capturedEnd && matchSpan[idx].get<"capture">()) {
                        result.append(part.sliced(matchSpan[idx].get<"start">(), matchSpan[idx].get<"len">()));
                        cidx += 1;
                        handled = true;
                    }
                }

                if (!handled) {
                    result.append(u'\\');
                }
            } else {
                result.append(u'\\');
            }

            cidx++;
        }

        return result;
    };

    QString replaced;
    replaced.reserve(part.size());

    u32 start = 0;

    for (u32 idx = 0; idx < scast<u32>(matchSpan.size());) {
        const TextMatch& match = matchSpan[idx];

        if (match.get<"capture">()) {
            ++idx;
            continue;
        }

        // The full matches are in order and don't overlap
        if (match.get<"start">() < start) {
            return nullopt;
        }

        u32 jIdx = idx + 1;
        while (jIdx < scast<u32>(matchSpan.size()) && matchSpan[jIdx].get<"capture">()) {
            ++jIdx;
        }

        replaced.append(part.sliced(start, match.get<"start">() - start));

        const QString substitution = substitute(match, idx + 1, jIdx);

        if (newMatches != nullptr) {
            newMatches->emplace_back(scast<u32>(replaced.size()), scast<u32>(substitution.size()), false);
        }

        replaced.append(substitution);
        start = match.get<"start">() + match.get<"len">();
        idx = jIdx;
    }

    replaced.append(part.sliced(start));
    return replaced;
}
}  // namespace

auto TaskWorker::replace(
    const TaskToken& task,
    const CellMatches& searchMatches,
    const SearchAction action,
    const QString& replaceText
) -> ReplaceResult {
    ReplaceResult result;
    result.skipped.reserve(searchMatches.size());
    result.deltas.reserve(searchMatches.size());

    // The replacement as the file holds it, for putting it in place of a whole cell
    const QString fileReplaceText = qsvReplace(replaceText, LINE_FEED, NEW_LINE);

    for (const auto& [filename, fileMatches] : searchMatches) {
        const vector<CellMatch>& matches = fileMatches.cells;
        u32 done = 0;
        i32 translatedDelta = 0;

        if (task->interrupted()) {
            break;
        }

        auto closure = [&, action, filename](const QString& /* content */, const QSVList& lines) -> QString {
            QStringList ownedLines;
            ownedLines.reserve(scast<isize>(matches.size()));

            // The cells that are replaced, kept for as long as the rows are joined
            QStringList ownedCells;
            ownedCells.reserve(scast<isize>(matches.size()));

            QSVList newLines;
            newLines.reserve(lines.size());

            i32 rowStart = 0;

            // The cells of one row follow each other: the row is rewritten once, with all of them replaced
            for (usize matchIdx = 0; matchIdx < matches.size();) {
                const i32 row = matches[matchIdx].rowIndex();

                if (row < rowStart || row >= lines.size()) {
                    matchIdx++;
                    continue;
                }

                for (i32 idx = rowStart; idx < row; idx++) {
                    newLines.push_back(lines[idx]);
                }

                QSVList parts = lineParts(lines[row], row + 1, filename);

                // An empty list keeps the row as it is
                if (!parts.isEmpty() && isProtectedComment(parts[0])) {
                    parts.clear();
                }

                const bool translatedBefore = hasTranslation(parts);

                for (; matchIdx < matches.size() && matches[matchIdx].rowIndex() == row; matchIdx++) {
                    const CellMatch& cellMatch = matches[matchIdx];
                    const i32 column = cellMatch.colIndex();

                    // A row can lack the column that the text is put in
                    if (!parts.isEmpty() && action != SearchAction::Replace && column >= parts.size() &&
                        column < scast<i32>(projectSettings->columns.size())) {
                        parts.resize(column + 1);
                    }

                    if (column >= parts.size()) {
                        continue;
                    }

                    if (action == SearchAction::Replace) {
                        // The positions are those of the text as the search saw it, with line feeds
                        const QString cellText = qsvReplace(parts[column], NEW_LINE, LINE_FEED);
                        const optional<QString> replaced =
                            replaceInCell(cellText, fileMatches.matches(cellMatch), replaceText, nullptr);

                        if (!replaced) {
                            continue;
                        }

                        ownedCells.append(qsvReplace(*replaced, LINE_FEED, NEW_LINE));
                        parts[column] = ownedCells.last();
                    } else {
                        parts[column] = fileReplaceText;
                    }

                    // Every emit queues an event for the GUI thread
                    if ((++done % PROGRESS_STEP) == 0) {
                        emit
                            taskProgress(task->id(), svtostr(QL1SV(filename.data())), done, scast<u32>(matches.size()));
                    }
                }

                translatedDelta += scast<i32>(hasTranslation(parts)) - scast<i32>(translatedBefore);

                if (parts.isEmpty()) {
                    newLines.push_back(lines[row]);
                } else {
                    ownedLines.append(joinQSVList(parts, SEPARATOR));
                    newLines.push_back(ownedLines.last());
                }

                rowStart = row + 1;
            }

            for (i32 idx = rowStart; idx < lines.size(); idx++) {
                newLines.push_back(lines[idx]);
            }

            return joinQSVList(newLines, u'\n');
        };

        if (runLockedModify(QL1SV(filename.data()), closure)) {
            result.deltas.push_back({ .filename = filename, .delta = translatedDelta });
        } else {
            result.skipped.push_back(filename);
        }
    }

    emit lockFile(QString());
    return result;
}

auto TaskWorker::translateSingle(
    const QString& filename,
    const QString& text,
    const Glossary& glossary,
    const TranslateContext& context
) -> TranslateSingleResult {
    const QByteArray textUtf8 = text.toUtf8();

    const QByteArray sourceLang = context.sourceLang.toUtf8();
    const QByteArray translationLang = context.translationLang.toUtf8();

    // Each returns the translation, or the reason there is none
    const auto translateClosure = [&](const EndpointSettings& endpointSettings) -> EndpointAnswer {
        const auto endpointJson = glz::write_json(endpointSettings);

        if (!endpointJson) {
            return { .text = QString::fromStdString(glz::format_error(endpointJson.error())), .failed = true };
        }

        FFIString outString;

        const bool success = rpgm_translate_single(
            strtoffi(endpointJson.value().data()),
            strtoffi(sourceLang),
            strtoffi(translationLang),
            strtoffi(textUtf8),
            &outString
        );

        if (!success) {
            return { .text = ffitostr(rpgm_error()).toString(), .failed = true };
        }

        QString translation = QString::fromUtf8(outString.ptr, scast<isize>(outString.len));
        rpgm_string_free(outString);

        return { .text = std::move(translation), .failed = false };
    };

    const Glossary noGlossary;

    // Scripts only run once the user approved them (Settings > Translation); an unapproved one reports why instead.
    const auto scriptClosure = [&](const EndpointSettings& endpointSettings) -> EndpointAnswer {
        auto file = QFile(endpointSettings.scriptPath);

        if (!file.open(QFile::ReadOnly)) {
            return { .text = tr("Cannot read script: %1").arg(file.errorString()), .failed = true };
        }

        const QString source = QString::fromUtf8(file.readAll());

        if (!context.approvedScripts.contains(scriptHash(source))) {
            return { .text = tr("Script is not approved to run. Save the endpoint in Settings to review it."),
                     .failed = true };
        }

        auto result = runTranslateScript(
            source,
            text,
            context.sourceLang,
            context.translationLang,
            endpointSettings.useGlossary ? glossary : noGlossary,
            filename
        );

        if (result) {
            return { .text = std::move(result.value()), .failed = false };
        }

        return { .text = std::move(result.error()), .failed = true };
    };

    // The endpoints are independent and each waits for a server, so one slow endpoint doesn't hold back the rest.
    // Each slot is written by exactly one call, and its position is the endpoint's index in `context.endpoints`.
    TranslateSingleResult translations(context.endpoints.size());

    QtConcurrent::blockingMap(translations, [&](EndpointAnswer& slot) -> void {
        const EndpointSettings& endpoint = context.endpoints[scast<usize>(&slot - translations.data())];

        slot = endpoint.endpoint == TranslationEndpoint::Script ? scriptClosure(endpoint) : translateClosure(endpoint);
    });

    return translations;
}

auto TaskWorker::replaceSingle(
    const QString& replaceText,
    const SearchPanelDock::Action action,
    const QString& filename,
    const u32 rowIndex,
    const u8 columnIndex,
    const span<const TextMatch> matches,
    const QString& expectedText
) -> ReplaceSingleResult {
    ReplacedCell replacedData;

    // The file changed since the search found the cell, so the cell is not there to be replaced
    bool stale = false;

    auto closure =
        [&, action, rowIndex, columnIndex, matches](const QString& /* content */, QSVList& lines) -> QString {
        if (rowIndex >= scast<u32>(lines.size())) {
            stale = true;
            return joinQSVList(lines, u'\n');
        }

        QSVList parts = lineParts(lines[rowIndex], rowIndex + 1, filename);

        if (parts.isEmpty() || isProtectedComment(parts[0])) {
            stale = true;
            return joinQSVList(lines, u'\n');
        }

        const bool translatedBefore = hasTranslation(parts);

        // The cell is put in a column the row may not have yet
        if (action == SearchPanelDock::Action::Put && columnIndex < projectSettings->columns.size() &&
            columnIndex >= parts.size()) {
            parts.resize(columnIndex + 1);
        }

        if (columnIndex >= parts.size()) {
            stale = true;
            return joinQSVList(lines, u'\n');
        }

        // Kept until the line is joined, as `parts` views it
        QString cell;

        if (action == SearchPanelDock::Action::Put) {
            cell = qsvReplace(replaceText, LINE_FEED, NEW_LINE);

            replacedData.text = qsvReplace(replaceText, u'\n', LINE_SEPARATOR);
            replacedData.matches = { TextMatch(0, scast<u32>(replaceText.size()), false) };
        } else {
            // The positions are those of the text as the search saw it, with line feeds
            const QString cellText = qsvReplace(parts[columnIndex], NEW_LINE, LINE_FEED);

            // The positions fit the cell only if it is the one that was searched
            if (cellText != expectedText) {
                stale = true;
                return joinQSVList(lines, u'\n');
            }

            vector<TextMatch> newMatches;
            newMatches.reserve(matches.size());

            const optional<QString> replaced = replaceInCell(cellText, matches, replaceText, &newMatches);

            if (!replaced) {
                stale = true;
                return joinQSVList(lines, u'\n');
            }

            cell = qsvReplace(*replaced, LINE_FEED, NEW_LINE);

            replacedData.text = qsvReplace(*replaced, u'\n', LINE_SEPARATOR);
            replacedData.matches = std::move(newMatches);
        }

        parts[columnIndex] = QStringView(cell);
        replacedData.translatedDelta = scast<i32>(hasTranslation(parts)) - scast<i32>(translatedBefore);

        const QString merged = joinQSVList(parts, SEPARATOR);
        lines[rowIndex] = QStringView(merged);

        return joinQSVList(lines, u'\n');
    };

    const QByteArray filenameLatin1 = filename.toLatin1();
    const bool success = modifyFile(QL1SV(filenameLatin1), *mapSections, projectSettings, closure);
    if (!success) {
        return Err(ReplaceSingleError{ .filename = filename, .stale = false });
    }

    if (stale) {
        return Err(ReplaceSingleError{ .filename = filename, .stale = true });
    }

    return ReplaceSingleResult{ std::move(replacedData) };
}

#ifdef ENABLE_LANGUAGETOOL
auto TaskWorker::languageToolLint(const QStringView text, const vector<Span>& sequences) -> LintResult {
    const QByteArray baseURL = settings->translation.languageTool.baseURL.toUtf8();
    const QByteArray apiKey = settings->translation.languageTool.apiKey.toUtf8();
    const QByteArray dicts = settings->translation.languageTool.dicts.toUtf8();
    const QByteArray enabledCategories = settings->translation.languageTool.enabledCategories.toUtf8();
    const QByteArray disabledCategories = settings->translation.languageTool.disabledCategories.toUtf8();
    const QByteArray enabledRules = settings->translation.languageTool.enabledRules.toUtf8();
    const QByteArray disabledRules = settings->translation.languageTool.disabledRules.toUtf8();
    const QByteArray motherTongue = settings->translation.languageTool.motherTongue.toUtf8();
    const QByteArray preferredVariants = settings->translation.languageTool.preferredVariants.toUtf8();
    const QByteArray username = settings->translation.languageTool.username.toUtf8();
    const QByteArray translationLang = projectSettings->translationLang.toUtf8();
    FFIString outJSON;

    const QByteArray segmentsBuffer = buildLanguageToolSegments(text, sequences);
    const auto segments = ByteBuffer{ .ptr = rcast<const u8*>(segmentsBuffer.data()),
                                      .len = scast<u32>(segmentsBuffer.size()),
                                      .cap = 0 };

    const bool success = rpgm_language_tool_lint(
        segments,
        strtoffi(baseURL),
        strtoffi(apiKey),
        strtoffi(translationLang),
        strtoffi(dicts),
        strtoffi(disabledCategories),
        strtoffi(disabledRules),
        strtoffi(enabledCategories),
        strtoffi(enabledRules),
        settings->translation.languageTool.picky,
        strtoffi(motherTongue),
        strtoffi(preferredVariants),
        strtoffi(username),
        &outJSON
    );

    return success ? LintResult{ outJSON } : LintResult{ Err(rpgm_error()) };
}
#endif