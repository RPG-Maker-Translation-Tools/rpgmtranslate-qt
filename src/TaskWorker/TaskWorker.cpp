#include "TaskWorker.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "PluginSequences.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QMessageBox>
#include <QRegularExpression>
#include <quickjs.h>

namespace {
constexpr QColor RED_HIGHLIGHT = QColor(UINT8_MAX, 0, 0, 80);

constexpr i32 SCRIPT_TIMEOUT = 15000;

constexpr i32 MISC_LINT_UNCLOSED_PUNCTUATION_INDEX = 5;
constexpr i32 MISC_LINT_UNCLOSED_COLOR_INDEX = 6;
constexpr QColor DEFAULT_LINT_COLOR = QColor(0xEF, 0xBF, 0x04);
constexpr i32 SCRIPT_CALLBACK_ARG_COUNT = 5;

constexpr usize SCRIPT_MEMORY_LIMIT = 64ULL * 1024 * 1024;
constexpr usize SCRIPT_MAX_STACK_SIZE = 1ULL * 1024 * 1024;

constexpr array<const char*, 7> MISC_LINTS = {
    QT_TR_NOOP("Misc: Unmatched closing punctuation."),
    QT_TR_NOOP("Misc: Multiple consequent whitespace."),
    QT_TR_NOOP("Misc: Trailing whitespace."),
    QT_TR_NOOP("Misc: Leading whitespace."),
    QT_TR_NOOP("Misc: Misspelled word."),
    QT_TR_NOOP("Misc: Unclosed punctuation."),
    QT_TR_NOOP("Misc: Unclosed tag."),
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

// Builds the alternating text/markup segment buffer LanguageTool's `data` check mode expects:
// [count][len, bytes]... starting and ending with a (possibly empty) text segment, so that
// even indices are plain text and odd indices are markup to be ignored by LanguageTool.
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

auto qstringToJS(JSContext* const ctx, const QStringView qsv) -> JSValue {
    const QByteArray utf8 = qsv.toUtf8();
    return JS_NewStringLen(ctx, utf8.constData(), utf8.size());
}

auto qstringToJS(JSContext* const ctx, const QLatin1StringView qsv) -> JSValue {
    return JS_NewStringLen(ctx, qsv.constData(), qsv.size());
}

auto jsToQString(JSContext* const ctx, const JSValueConst val) -> QString {
    usize len = 0;
    const char* const buf = JS_ToCStringLen(ctx, &len, val);

    if (buf == nullptr) {
        return {};
    }

    QString result = QString::fromUtf8(buf, scast<isize>(len));
    JS_FreeCString(ctx, buf);
    return result;
}

auto jsArrayLength(JSContext* const ctx, const JSValueConst val) -> u32 {
    u32 len;
    auto prop = JS_GetPropertyStr(ctx, val, "length");
    JS_ToUint32(ctx, &len, prop);
    return len;
}

auto jsGetExceptionText(JSContext* const ctx) -> QString {
    const JSValue exc = JS_GetException(ctx);
    QString msg = jsToQString(ctx, exc);
    JS_FreeValue(ctx, exc);
    return msg;
}

struct ScriptRuntime {
    explicit ScriptRuntime(QDeadlineTimer deadline) : deadline_(deadline) {
        rt_ = JS_NewRuntime();
        JS_SetMemoryLimit(rt_, SCRIPT_MEMORY_LIMIT);
        JS_SetMaxStackSize(rt_, SCRIPT_MAX_STACK_SIZE);
        JS_SetInterruptHandler(rt_, [](JSRuntime*, auto* opaque) -> i32 {
            return scast<QDeadlineTimer*>(opaque)->hasExpired() ? 1 : 0;
        }, &deadline_);
        ctx = JS_NewContext(rt_);
    }

    ~ScriptRuntime() {
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt_);
    }

    ScriptRuntime(const ScriptRuntime&) = delete;
    auto operator=(const ScriptRuntime&) -> ScriptRuntime& = delete;

    QDeadlineTimer deadline_;
    JSRuntime* rt_;
    JSContext* ctx;
};

struct JSValueGuard {
    JSValueGuard(JSContext* ctx, JSValue value) : ctx_(ctx), value_(value) {}

    ~JSValueGuard() { JS_FreeValue(ctx_, value_); }

    JSValueGuard(const JSValueGuard&) = delete;
    auto operator=(const JSValueGuard&) -> JSValueGuard& = delete;

    operator JSValue() const { return value_; }

    JSContext* ctx_;
    JSValue value_;
};

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

extern "C" auto taskInterrupted(void* const context) -> bool {
    return scast<const Task*>(context)->interrupted();
}

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
        case TaskKind::BatchTranslate:
            return QObject::tr("Translate");
        case TaskKind::GlobalLint:
            return QObject::tr("Lint");
        case TaskKind::SerdeExport:
            return QObject::tr("Export");
        case TaskKind::SerdeImport:
            return QObject::tr("Import");
    }

    std::unreachable();
}

auto TaskWorker::startTask(const TaskKind kind) -> result<TaskToken, TaskKind> {
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

auto fileLines(
    const QL1SV filename,
    const HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings
) -> result<FileLines, QString> {
    QString content;
    QSVList lines;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = stoa<u16>(filename.sliced(3));
        lines = QStringView(mapSections[mapNumber]).split(u'\n', Qt::SkipEmptyParts);
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
};

template <class F>
auto modifyFile(
    const QL1SV filename,
    HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings,
    F&& func
) -> bool {
    QString content;
    QSVList lines;

    QFile* file;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = stoa<u16>(filename.sliced(3));
        lines = QStringView(mapSections[mapNumber]).split(u'\n', Qt::SkipEmptyParts);
    } else {
        const QString path = projectSettings->translationPath() % u'/' % filename % u".txt"_qsv;

        file = new QFile(path);

        if (!file->open(QFile::ReadWrite)) {
            qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(path, file->errorString());
            delete file;
            return false;
        }

        content = QString::fromUtf8(file->readAll());
        lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);
    }

    QString result = func(content, lines);

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = stoa<u16>(filename.sliced(3));
        mapSections.find(mapNumber)->second = std::move(result);
    } else {
        const QByteArray utf8 = result.toUtf8();

        file->seek(0);
        file->resize(utf8.size());
        file->write(utf8);

        delete file;
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
    const QString& title
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

    const ByteBuffer hashesBuf = ByteBuffer{ .ptr = rcast<const u8*>(hashesArray.data()),
                                             .len = scast<u32>(hashesArray.size()),
                                             .cap = scast<u32>(hashes.size()) };

    const bool success = rpgm_read(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        readMode,
        engineType,
        duplicateMode,
        selected,
        flags,
        mapEvents,
        hashesBuf,
        strtoffi(titleUtf8),
        &outHashes
    );

    return success ? ReadResult{ outHashes } : ReadResult{ Err(rpgm_error()) };
}

auto TaskWorker::write(const QString& gameTitle, const Selected selected) -> WriteResult {
    const QByteArray sourcePathUtf8 = projectSettings->actualSourcePath().toUtf8();
    const QByteArray translationPathUtf8 = projectSettings->translationPath().toUtf8();
    const QByteArray outputPathUtf8 = projectSettings->outputPath().toUtf8();
    const QByteArray gameTitleUtf8 = gameTitle.toUtf8();

    f32 elapsed;

    const bool success = rpgm_write(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        strtoffi(outputPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        strtoffi(gameTitleUtf8),
        projectSettings->flags,
        selected,
        &elapsed
    );

    return success ? WriteResult{ elapsed } : WriteResult{ Err(rpgm_error()) };
}

auto TaskWorker::extractArchive(const QString& archivePath, const QString& folder) -> ExtractResult {
    const QByteArray archivePathUtf8 = archivePath.toUtf8();
    const QByteArray folderUtf8 = folder.toUtf8();

    const bool success = rpgm_extract_archive(strtoffi(archivePathUtf8), strtoffi(folderUtf8));

    return success ? ExtractResult{} : ExtractResult{ Err(rpgm_error()) };
}

auto TaskWorker::purge(const QString& gameTitle, const Selected selected) -> PurgeResult {
    const QByteArray sourcePathUtf8 = projectSettings->actualSourcePath().toUtf8();
    const QByteArray translationPathUtf8 = projectSettings->translationPath().toUtf8();
    const QByteArray gameTitleUtf8 = gameTitle.toUtf8();

    const bool success = rpgm_purge(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        strtoffi(gameTitleUtf8),
        projectSettings->flags,
        selected
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
        pattern = u"\\b"_s % pattern % u"\\b";
    }

    const auto regexp = QRegularExpression(pattern, options);

    CellMatches searchMatches;
    searchMatches.reserve(tabCount);

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

        u32 capturedCount = 0;
        vector<QRegularExpressionMatch> matchesVec;

        for (const auto& match : matches) {
            capturedCount += match.lastCapturedIndex() + 1;
            matchesVec.emplace_back(std::move(ccast<QRegularExpressionMatch&>(match)));
        }

        const MatchIndex matchIndex(rowIndex, action == SearchAction::Put ? searchColumnIndex : columnIndex);

        auto cellMatches = CellMatch{ .matches = new TextMatch[capturedCount],
                                      .matchesCount = capturedCount,
                                      .matchIndex = matchIndex };
        u32 matchesPos = 0;

        for (const auto& match : matchesVec) {
            if ((searchFlags & SearchFlags::Put) != 0) {
                const u32 start = match.capturedStart();
                const u32 length = match.capturedLength();

                if (start == 0 && length == line.size()) {
                    cellMatches.matches[matchesPos++] = TextMatch(0, 0, false);
                } else {
                    break;
                }
            } else {
                for (const auto idx : range(0, match.lastCapturedIndex() + 1)) {
                    cellMatches.matches[matchesPos++] =
                        TextMatch(match.capturedStart(idx), match.capturedLength(idx), idx > 0);
                }
            }
        }

        searchMatches.emplace(filename, vector<CellMatch>());
        searchMatches.find(filename)->second.push_back(cellMatches);
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

        for (const auto [rowIndex, line] : views::enumerate(lines)) {
            if (task->interrupted()) {
                break;
            }

            if (line.startsWith(COMMENT_PREFIX) && !line.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX) &&
                !line.startsWith(BOOKMARK_COMMENT)) {
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

            vector<ToProcess> toProcess;

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

                    toProcess.emplace_back(translation.translation, translation.index);
                } else {
                    toProcess.emplace_back(parts[searchColumnIndex], searchColumnIndex);
                }
            }

            for (const auto entryToProcess : toProcess) {
                const auto normalized = qsvReplace(entryToProcess.text, NEW_LINE, LINE_FEED);

                closure(filenameArray, normalized, rowIndex, entryToProcess.columnIndex);
            }

            emit taskProgress(task->id(), svtostr(QL1SV(filenameArray.data())), ++done, scast<u32>(lines.size()));
        }

        return true;
    });

    return { .matches = std::move(searchMatches), .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::lintFiles(const TaskToken& task, const Selected selected, const Glossary& glossary)
    -> GlobalLintResult {
    auto filenames = selected.filenames(projectSettings->engineType);
    vector<LintRow> rows;

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        const auto filename = QL1SV(filenameArray.data());
        const auto result = fileLines(filename, *mapSections, projectSettings);

        if (!result) {
            return false;
        }

        const auto fname = filename.toString();

        for (const auto [idx, line] : views::enumerate(result.value().lines)) {
            if (task->interrupted()) {
                break;
            }

            const auto parts = lineParts(line, idx, filename);

            if (parts.isEmpty()) {
                continue;
            }

            auto outcome =
                lint(fname, scast<u32>(idx) + 1, getSource(parts), getTranslation(parts).translation, glossary, false);

            rows.insert(
                rows.end(),
                std::make_move_iterator(outcome.rows.begin()),
                std::make_move_iterator(outcome.rows.end())
            );
        }

        return true;
    });

    return { .rows = std::move(rows), .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::matchGlossaryTerm(
    const QString& filename,
    const u32 lineNumber,
    const QStringView source,
    const QStringView translation,
    const Term& term
) -> optional<LintRow> {
    const QByteArray sourceUtf8 = source.toUtf8();
    const QByteArray termUtf8 = term.term.toUtf8();
    const QByteArray termTranslationUtf8 = term.translation.toUtf8();
    const QByteArray translationUtf8 = translation.toUtf8();

    const QByteArray sourceLang = projectSettings->sourceLang.toUtf8();
    const QByteArray translationLang = projectSettings->translationLang.toUtf8();

    ByteBuffer matches;

    const bool success = rpgm_find_all_matches(
        strtoffi(sourceUtf8),
        strtoffi(termUtf8),
        term.sourceMatchMode,
        strtoffi(translationUtf8),
        strtoffi(termTranslationUtf8),
        term.translationMatchMode,
        strtoffi(sourceLang),
        strtoffi(translationLang),
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

    for (const auto match : sourceMatches) {
        sourceMatchDescs.append(match.score == 0.0F ? tr("Exact") : tr("Fuzzy (%1)").arg(ftos(match.score, 3).qsv()));
        sourceHighlights.emplace_back(match.start, match.len);
    }

    QStringList translationMatchDescs;
    vector<Span> translationHighlights;

    translationMatchDescs.reserve(translationCount);
    translationHighlights.reserve(translationCount);

    for (const auto match : translationMatches) {
        translationMatchDescs.append(match.score == 0.0F ? tr("Exact") : tr("Fuzzy (%1)").arg(ftos(match.score).qsv()));
        translationHighlights.emplace_back(match.start, match.len);
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

static const QRegularExpression MISSPELLED_WORD_REGEXP =
    QRegularExpression(uR"(\b[\p{L}']+\b)"_s, QRegularExpression::UseUnicodePropertiesOption);

auto TaskWorker::lint(
    const QString& filename,
    const u32 lineNumber,
    const QStringView source,
    const QStringView translation,
    const Glossary& glossary,
    const bool highlight
) -> LintOutcome {
    const i32 size = scast<i32>(translation.size());

    vector<LintRow> rows;

    if ((settings->translation.lintFlags & LintFlags::TermMismatch) != 0 && !projectSettings->sourceLang.isEmpty() &&
        !projectSettings->translationLang.isEmpty()) {
        for (const Term& term : glossary) {
            if (!term.enabled) {
                continue;
            }

            if (auto row = matchGlossaryTerm(filename, lineNumber, source, translation, term)) {
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

        for (const QRegularExpression* regexp : regexps) {
            for (const auto& match : regexp->globalMatchView(source)) {
                for (const auto idx : range(1, match.lastCapturedIndex() + 1)) {
                    if (match.capturedLength(idx) > 0) {
                        const i32 start = scast<i32>(match.capturedStart(idx));
                        const i32 len = scast<i32>(match.capturedLength(idx));
                        QString captured = match.captured(idx);

                        sourceTags.emplace_back(std::move(captured));
                        sourceHighlights.emplace_back(start, len);
                        break;
                    }
                }
            };

            for (const auto& match : regexp->globalMatchView(translation)) {
                for (const auto idx : range(1, match.lastCapturedIndex() + 1)) {
                    if (match.capturedLength(idx) > 0) {
                        const i32 start = scast<i32>(match.capturedStart(idx));
                        const i32 len = scast<i32>(match.capturedLength(idx));
                        QString captured = match.captured(idx);

                        translationTags.emplace_back(std::move(captured));
                        translationHighlights.emplace_back(start, len);
                        break;
                    }
                }
            };
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
                    .info = entry.translationId != nullptr ? tr(entry.translationId) : entry.extra.toString(),
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

                vector<string> suggestions;
                dictionary->suggest(string_view(utf8Word.data(), utf8Word.size()), suggestions);

                QStringList qsuggestions;
                qsuggestions.reserve(scast<usize>(suggestions.size()));
                for (const auto& suggestion : suggestions) {
                    qsuggestions.emplace_back(QString::fromUtf8(suggestion));
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

    i32 lead = 0;

    if ((settings->translation.lintFlags & LintFlags::LeadingWhitespace) != 0) {
        while (lead < size &&
               ranges::contains(settings->translation.whitespaceCharacters, codePointAt(translation, lead))) {
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

        while (lastNonSpace >= 0 &&
               ranges::contains(settings->translation.whitespaceCharacters, codePointAt(translation, lastNonSpace))) {
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
            if (ranges::contains(settings->translation.whitespaceCharacters, codePointAt(translation, idx))) {
                const i32 start = idx;

                while (idx < size &&
                       ranges::contains(settings->translation.whitespaceCharacters, codePointAt(translation, idx))) {
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

        // TODO: keep an eye on this
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

        const auto findPunctuation = [](const QChar chr) -> optional<Punctuation> {
            for (const auto punctuation : puncts) {
                if (punctuation.open == chr || punctuation.close == chr) {
                    return punctuation;
                }
            }

            return nullopt;
        };

        for (const auto idx : range(0, size)) {
            const QChar chr = translation.at(idx);
            const optional<Punctuation> punctuation = findPunctuation(chr);

            if (!punctuation) {
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
            for (const auto idx : range(1, match.lastCapturedIndex() + 1)) {
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
    vector<std::pair<i32, i32>> unclosedColorTags;

    applyLints(
        TextCodes::REGEXP,
        LintType::TextCode,
        TextCodes::TOOLTIPS,
        settings->translation.textCodes,
        [&](const auto& /* match */, const i32 idx, LintEntry& lint, const i32 start, const i32 len) -> auto {
        switch (idx - 1) {
            case 31: {
                const auto colors = getWindowColors(settings->core.projectPath, projectSettings->engineType);

                if (colors) {
                    const u8 colorIndex = stoa<u8>(lint.captured.sliced(3, 1));

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
                const QRgb color = stoa<u32>(rgbHex, 16);

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

    if (!settings->translation.customLints.empty()) {
        vector<i32> customLintIndices;
        customLintIndices.reserve(settings->translation.customLints.size());

        isize patternLength = 0;

        for (const auto& [idx, customLint] : views::enumerate(settings->translation.customLints)) {
            if (customLint.disabled) {
                continue;
            }

            patternLength += customLint.sequence.size() + (customLint.caseInsensitive ? 5 : 2) + 1;
            customLintIndices.push_back(scast<i32>(idx));
        }

        if (!customLintIndices.empty()) {
            QString pattern;
            pattern.reserve(patternLength);

            for (const auto originalIdx : customLintIndices) {
                const auto& customLint = settings->translation.customLints[originalIdx];

                if (!pattern.isEmpty()) {
                    pattern += u'|';
                }

                pattern += u'(';

                if (customLint.caseInsensitive) {
                    pattern += u"?i:";
                }

                pattern += customLint.sequence;
                pattern += u')';
            }

            const auto mergedRegex = QRegularExpression(pattern);

            for (const auto& match : mergedRegex.globalMatchView(translation)) {
                for (const auto idx : range(1, match.lastCapturedIndex() + 1)) {
                    if (match.capturedLength(idx) == 0) {
                        continue;
                    }

                    const i32 originalIdx = customLintIndices[idx - 1];
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
) -> BatchResult {
    auto filenames = selected.filenames(projectSettings->engineType);

    if (script.action == BatchAction::Translate) {
        return runBatchTranslate(task, filenames, std::get<EndpointContext>(variant), glossary);
    }

    return runBatchScript(task, script, filenames, columnIndex, variant);
}

auto TaskWorker::runBatchTranslate(
    const TaskToken& task,
    const vector<FilenameArray>& filenames,
    const EndpointContext& endpointContext,
    const Glossary& glossary
) -> TranslateResult {
    const auto [endpointIndex, context] = endpointContext;

    const auto glossaryResult = glz::write_json(glossary);
    const auto endpointJson = glz::write_json(settings->translation.endpoints[endpointIndex]);

    const QByteArray projectContext = projectSettings->projectContext.toUtf8();
    const QByteArray localContext = context.toUtf8();
    const QByteArray translationPath = projectSettings->translationPath().toUtf8();

    const QByteArray sourceLang = projectSettings->sourceLang.toUtf8();
    const QByteArray translationLang = projectSettings->translationLang.toUtf8();

    ByteBuffer translatedFiles;
    ByteBuffer translatedFilesFFI;

    const bool success = rpgm_translate(
        strtoffi(endpointJson.value().data()),
        strtoffi(projectContext),
        strtoffi(localContext),
        strtoffi(translationPath),
        strtoffi(sourceLang),
        strtoffi(translationLang),
        { .ptr = rcast<const u8*>(filenames.data()), .len = scast<u32>(filenames.size()) },
        strtoffi(glossaryResult.value().data()),
        &translatedFiles,
        &translatedFilesFFI,
        &taskInterrupted,
        task.get()
    );

    return success ? TranslateResult{ TranslatedFiles{ .files = translatedFiles, .filesFFI = translatedFilesFFI } }
                   : TranslateResult{ Err(rpgm_error()) };
}

auto TaskWorker::runBatchScript(
    const TaskToken& task,
    const JSScript& script,
    vector<FilenameArray>& filenames,
    const u8 columnIndex,
    const BatchVariant& variant
) -> ScriptResult {
    auto deadline = QDeadlineTimer(SCRIPT_TIMEOUT);
    auto runtime = ScriptRuntime(deadline);
    JSContext* const ctx = runtime.ctx;

    const QByteArray src = script.source.toUtf8();
    const JSValue evalResult = JS_Eval(ctx, src.constData(), src.size(), "<user-script>", JS_EVAL_TYPE_GLOBAL);

    if (JS_IsException(evalResult)) {
        JS_FreeValue(ctx, evalResult);
        return Err(ScriptError{ .message = jsGetExceptionText(ctx), .filename = {}, .lineNumber = 0 });
    }

    JS_FreeValue(ctx, evalResult);

    const JSValue global = JS_GetGlobalObject(ctx);
    auto callback = JSValueGuard(ctx, JS_GetPropertyStr(ctx, global, "rpgmLineCallback"));
    JS_FreeValue(ctx, global);

    if (!JS_IsFunction(ctx, callback)) {
        return Err(
            ScriptError{
                .message = tr(
                    "Script must define function rpgmLineCallback(sourceText, translations, filename, lineNumber, options)"
                ),
                .filename = {},
                .lineNumber = 0,
            }
        );
    }

    auto options = JSValueGuard(ctx, JS_NewObject(ctx));
    JS_SetPropertyStr(ctx, options, "action", qstringToJS(ctx, batchActionId(script.action)));
    JS_SetPropertyStr(ctx, options, "columnIndex", JS_NewInt32(ctx, columnIndex));

    if (script.action == BatchAction::Trim) {
        const LintFlags trimFlags = std::get<LintFlags>(variant);

        JS_SetPropertyStr(
            ctx,
            options,
            "trimLeading",
            JS_NewBool(ctx, (trimFlags & LintFlags::LeadingWhitespace) != 0)
        );
        JS_SetPropertyStr(
            ctx,
            options,
            "trimTrailing",
            JS_NewBool(ctx, (trimFlags & LintFlags::TrailingWhitespace) != 0)
        );
    } else if (script.action == BatchAction::Wrap) {
        JS_SetPropertyStr(ctx, options, "wrapLength", JS_NewInt32(ctx, std::get<u8>(variant)));
    }

    optional<ScriptError> scriptError;

    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        if (scriptError) {
            return true;
        }

        const auto filename = QL1SV(filenameArray.data());
        u32 done = 0;

        auto closure = [&, ctx, columnIndex, filename](const QString& content, const QSVList& lines) -> QString {
            QString joined;
            joined.reserve(content.size() * 2);

            for (const auto [lineIdx, line] : views::enumerate(lines)) {
                if (task->interrupted()) {
                    for (const auto idx : range(lineIdx, lines.size())) {
                        joined += lines[idx];
                        joined += u'\n';
                    }

                    break;
                }

                QSVList parts = lineParts(line, lineIdx, filename);
                const QStringView sourceText = parts[0];

                const bool protectedComment =
                    sourceText.startsWith(COMMENT_PREFIX) && !sourceText.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX);

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

                    for (const auto idx : range(lineIdx, lines.size())) {
                        joined += lines[idx];
                        joined += u'\n';
                    }

                    break;
                }

                if (protectedComment) {
                    JS_FreeValue(ctx, ret);

                    joined += joinQSVList(parts, SEPARATOR);
                    joined += u'\n';

                    emit taskProgress(
                        task->id(),
                        svtostr(QL1SV(filenameArray.data())),
                        ++done,
                        scast<u32>(lines.size())
                    );
                    continue;
                }

                const u32 retLen = jsArrayLength(ctx, ret);
                QStringList ownedParts;
                ownedParts.reserve(parts.size());

                for (u32 i = 0; i + 1 < parts.size(); i++) {
                    if (i >= retLen) {
                        continue;
                    }

                    const JSValue element = JS_GetPropertyUint32(ctx, ret, i);

                    if (!JS_IsUndefined(element) && !JS_IsNull(element)) {
                        ownedParts.push_back(jsToQString(ctx, element));
                        parts[i + 1] = QStringView(ownedParts.last());
                    }

                    JS_FreeValue(ctx, element);
                }

                JS_FreeValue(ctx, ret);

                joined += joinQSVList(parts, SEPARATOR);
                joined += u'\n';

                emit taskProgress(task->id(), svtostr(QL1SV(filenameArray.data())), ++done, scast<u32>(lines.size()));
            }

            joined.removeLast();
            return joined;
        };

        return runLockedModify(filename, closure);
    });

    emit lockFile(QString());

    if (scriptError) {
        return Err(std::move(*scriptError));
    }

    return ScriptSuccess{ .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::serdeExport(
    const TaskToken& task,
    vector<FilenameArray>& filenames,
    const QString& outputDir,
    const SerdeFormat format
) -> SerdeResult {
    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        const auto filename = QL1SV(filenameArray.data());
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
        auto outFile = QFile(outPath);

        bool ok = false;

        if (outFile.open(QFile::WriteOnly | QFile::Truncate)) {
            ok = outFile.write(rcast<const char*>(exported.ptr), exported.len) != -1;
        }

        rpgm_buffer_free(exported);

        return ok;
    });

    return SerdeSuccess{ .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::serdeImport(
    const TaskToken& task,
    vector<FilenameArray>& filenames,
    const QString& inputDir,
    const SerdeFormat format
) -> SerdeResult {
    const u32 skippedCount = processFilesTrackingSkips(task, filenames, [&](const FilenameArray filenameArray) -> bool {
        const auto filename = QL1SV(filenameArray.data());
        const QString inPath = inputDir % u'/' % filename % serdeFormatExtension(format);
        auto inFile = QFile(inPath);

        if (!inFile.open(QFile::ReadOnly)) {
            return false;
        }

        const QByteArray raw = inFile.readAll();
        const ByteBuffer rawBuffer = { .ptr = rcast<const u8*>(raw.constData()), .len = scast<u32>(raw.size()), .cap = 0 };

        FFIString imported;

        if (!rpgm_serde_import(rawBuffer, format, &imported)) {
            return false;
        }

        const QString importedContent = QString::fromUtf8(imported.ptr, scast<isize>(imported.len));
        rpgm_string_free(imported);

        return runLockedModify(filename, [&](const QString&, const QSVList&) -> QString { return importedContent; });
    });

    emit lockFile(QString());

    return SerdeSuccess{ .filenames = std::move(filenames), .skippedCount = skippedCount };
}

auto TaskWorker::replace(
    const TaskToken& task,
    const CellMatches& searchMatches,
    const SearchAction action,
    const QString& replaceText
) -> ReplaceResult {
    ReplaceResult skipped;
    skipped.reserve(searchMatches.size());

    for (const auto& [filename, matches] : searchMatches) {
        u32 done = 0;

        if (task->interrupted()) {
            break;
        }

        auto closure = [&, action, filename](const QString& /* content */, const QSVList& lines) -> QString {
            QStringList ownedLines;
            ownedLines.reserve(scast<isize>(matches.size()));

            QSVList newLines;
            newLines.reserve(lines.size());

            u32 rowStart = 0;

            for (const auto& cellMatch : matches) {
                for (const auto idx : range(rowStart, cellMatch.rowIndex())) {
                    newLines.push_back(lines[idx]);
                }

                const QStringView line = lines[cellMatch.rowIndex()];
                QSVList parts = lineParts(line, cellMatch.rowIndex() + 1, filename);
                const QStringView part = parts[cellMatch.colIndex()];

                QString replaced;

                if (action == SearchAction::Replace) {
                    u32 start = 0;

                    const auto matchSpan = span(cellMatch.matches, cellMatch.matchesCount);

                    const auto applySubstitutions =
                        [&](const TextMatch& fullMatch, const u32 capturedBegin, const u32 capturedEnd) -> QString {
                        const QStringView beforeFull = part.sliced(0, fullMatch.start());
                        const QStringView afterFull = part.sliced(fullMatch.start() + fullMatch.len());

                        QStringView lastCapture;
                        for (u32 k = capturedEnd; k > capturedBegin;) {
                            --k;
                            if (matchSpan[k].capture()) {
                                lastCapture = part.sliced(matchSpan[k].start(), matchSpan[k].len());
                                break;
                            }
                        }

                        QString result;
                        result.reserve(replaceText.size());

                        for (auto cidx : range(0, replaceText.size())) {
                            if (replaceText[cidx] != u'\\' || cidx + 1 >= replaceText.size()) {
                                result.append(replaceText[cidx]);
                                continue;
                            }

                            const QChar next = replaceText[cidx + 1];

                            if (next == u'`') {
                                result.append(beforeFull);
                                ++cidx;
                            } else if (next == u'\'') {
                                result.append(afterFull);
                                ++cidx;
                            } else if (next == u'+') {
                                result.append(lastCapture);
                                ++cidx;
                            } else if (next == u'\\') {
                                result.append(u'\\');
                                ++cidx;
                            } else if (next.isDigit() && next != u'0') {
                                bool handled = false;

                                if (cidx + 2 < replaceText.size() && replaceText[cidx + 2].isDigit()) {
                                    const i32 twoDigit = (next.digitValue() * 10) + replaceText[cidx + 2].digitValue();
                                    const u32 idx = capturedBegin + scast<u32>(twoDigit - 1);
                                    if (idx < capturedEnd && matchSpan[idx].capture()) {
                                        result.append(part.sliced(matchSpan[idx].start(), matchSpan[idx].len()));
                                        cidx += 2;
                                        handled = true;
                                    }
                                }

                                if (!handled) {
                                    const i32 oneDigit = next.digitValue();
                                    const u32 idx = capturedBegin + scast<u32>(oneDigit - 1);
                                    if (idx < capturedEnd && matchSpan[idx].capture()) {
                                        result.append(part.sliced(matchSpan[idx].start(), matchSpan[idx].len()));
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
                        }

                        return result;
                    };

                    for (u32 idx = 0; idx < scast<u32>(matchSpan.size());) {
                        const auto& match = matchSpan[idx];

                        if (match.capture()) {
                            ++idx;
                            continue;
                        }

                        u32 jIdx = idx + 1;
                        while (jIdx < scast<u32>(matchSpan.size()) && matchSpan[jIdx].capture()) {
                            ++jIdx;
                        }

                        replaced.append(part.sliced(start, match.start() - start));
                        replaced.append(applySubstitutions(match, idx + 1, jIdx));
                        start = match.start() + match.len();
                        idx = jIdx;
                    }

                    replaced.append(part.sliced(start, part.size() - start));
                    parts[cellMatch.colIndex()] = replaced;
                } else {
                    parts[cellMatch.colIndex()] = replaceText;
                }

                emit taskProgress(task->id(), svtostr(QL1SV(filename.data())), ++done, scast<u32>(matches.size()));

                ownedLines.append(joinQSVList(parts, SEPARATOR));
                newLines.push_back(ownedLines.last());
                rowStart = cellMatch.rowIndex() + 1;
            }

            for (const auto idx : range(rowStart, lines.size())) {
                newLines.push_back(lines[idx]);
            }

            return joinQSVList(newLines, u'\n');
        };

        const bool success = runLockedModify(QL1SV(filename.data()), closure);

        if (!success) {
            skipped.push_back(filename);
        }
    }

    for (const auto& [key, matches] : searchMatches) {
        for (const auto match : matches) {
            delete[] match.matches;
        }
    }

    emit lockFile(QString());
    return skipped;
}

auto TaskWorker::translateSingle(const QString& filename, const QString& text, const Glossary& glossary)
    -> TranslateSingleResult {
    const auto glossaryJson = glz::write_json(glossary);

    TranslateSingleResult translations;
    translations.reserve(settings->translation.endpoints.size());

    QString localContext;

    if (projectSettings->fileContexts.contains(filename)) {
        localContext = projectSettings->fileContexts[filename];
    }

    const QByteArray projectContext = projectSettings->projectContext.toUtf8();
    const QByteArray localContextUtf8 = localContext.toUtf8();
    const QByteArray textUtf8 = text.toUtf8();

    const QByteArray sourceLang = projectSettings->sourceLang.toUtf8();
    const QByteArray translationLang = projectSettings->translationLang.toUtf8();

    const auto translateClosure = [&](const EndpointSettings& endpointSettings) -> auto {
        const auto endpointJson = glz::write_json(endpointSettings);

        FFIString outString;

        const bool success = rpgm_translate_single(
            strtoffi(endpointJson.value().data()),
            strtoffi(projectContext),
            strtoffi(localContextUtf8),
            strtoffi(sourceLang),
            strtoffi(translationLang),
            strtoffi(textUtf8),
            strtoffi(glossaryJson.value().data()),
            &outString
        );

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            translations.push_back(error.toString());
            return;
        }

        translations.push_back(QString::fromUtf8(outString.ptr, scast<isize>(outString.len)));
        rpgm_string_free(outString);
    };

    for (const auto& endpoint : settings->translation.endpoints) {
        if (!endpoint.singleTranslation) {
            continue;
        }

        translateClosure(endpoint);
    }

    return translations;
}

auto TaskWorker::replaceSingle(
    const QString& replaceText,
    const SearchPanelDock::Action action,
    const QString& filename,
    const u32 rowIndex,
    const u8 columnIndex,
    const span<const TextMatch> matches
) -> ReplaceSingleResult {
    ReplacedCell replacedData;

    auto closure =
        [&, action, rowIndex, columnIndex, matches](const QString& /* content */, QSVList& lines) -> QString {
        const QStringView line = lines[rowIndex];
        QSVList parts = lineParts(line, rowIndex + 1, filename);
        QString replaced;

        if (action == SearchPanelDock::Action::Put) {
            auto* const newMatches = new TextMatch[1];
            newMatches[0] = TextMatch(0, scast<u32>(replaceText.size()), false);
            parts[columnIndex] = QStringView(replaceText);
            replacedData = { .text = replaceText, .matches = newMatches };
        } else {
            const QStringView text = parts[columnIndex];
            replaced.reserve(scast<isize>(text.size() + (replaceText.size() * matches.size())));

            auto* const newMatches = new TextMatch[matches.size()];

            u32 pos = 0;
            u32 matchesIndex = 0;
            u32 delta = 0;

            for (const auto match : matches) {
                replaced.append(text.sliced(pos, match.start() - pos));

                const u32 newStart = match.start() + delta;
                const u32 newLen = scast<u32>(replaceText.size());

                newMatches[matchesIndex++] = TextMatch(newStart, newLen, false);
                replaced.append(replaceText);

                pos = match.start() + match.len();
                delta += newLen - match.len();
            }

            replaced.append(text.sliced(pos));
            parts[columnIndex] = QStringView(replaced);
            replacedData = { .text = replaced, .matches = newMatches };
        }

        const QString merged = joinQSVList(parts, SEPARATOR);
        lines[rowIndex] = QStringView(merged);

        return joinQSVList(lines, u'\n');
    };

    const QByteArray filenameLatin1 = filename.toLatin1();
    const bool success = modifyFile(QL1SV(filenameLatin1), *mapSections, projectSettings, closure);
    return success ? ReplaceSingleResult{ std::move(replacedData) } : ReplaceSingleResult{ Err(filename) };
}

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
    const ByteBuffer segments{ .ptr = rcast<const u8*>(segmentsBuffer.data()),
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