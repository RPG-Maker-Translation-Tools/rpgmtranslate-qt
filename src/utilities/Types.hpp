#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "Numbers.hpp"
#include "PackedBits.hpp"
#include "glazemeta.hpp"
#include "rpgmtranslate_rs.h"

#include <QColor>
#include <QTextCharFormat>

template <>
struct glz::meta<MatchModeInfo> {
    using T = MatchModeInfo;
    static constexpr auto value = glz::object(
        "mode",
        glz::custom<[](MatchModeInfo& out, const u8 input) -> void {
        out.mode.tag = MatchMode::Tag(input);
    }, [](const MatchModeInfo& input) -> u8 { return scast<u8>(input.mode.tag); }>,
        "fuzzyThreshold",
        glz::custom<
            [](MatchModeInfo& out, const f64 input) -> void {
        if (out.mode.tag == MatchMode::Tag::Fuzzy) {
            out.mode.fuzzy.threshold = input;
        } else if (out.mode.tag == MatchMode::Tag::Both) {
            out.mode.both.threshold = input;
        }
    },
            [](const MatchModeInfo& input) -> f64 {
        return input.mode.tag != MatchMode::Tag::Exact ? input.mode.fuzzy.threshold : 0.0;
    }>,
        "caseSensitive",
        &T::case_sensitive,
        "permissive",
        &T::permissive
    );
};

struct Term {
    QString term;
    QString translation;
    QString note;
    MatchModeInfo sourceMatchMode;
    MatchModeInfo translationMatchMode;
    bool enabled = true;
};

using Glossary = std::vector<Term>;

struct Span {
    u32 start;
    u32 len;
};

using TextMatch = PackedBits<Field<"start", u32, 32>, Field<"len", u32, 31>, Field<"capture", bool, 1>>;

using MatchIndex = PackedBits<Field<"row", u32, 24>, Field<"col", u8, 8>>;

static constexpr u32 NO_ENTRY = std::numeric_limits<u32>::max();

// The match, text and entry fields locate the matches of the cell, the text it shows and its entry in the
// `FileMatches` that holds it. A search fills the text and entry for the result panel only; a replace leaves them as
// they are.
struct CellMatch {
    u32 matchStart;
    u32 matchCount;
    u32 textStart;
    u32 textLength;
    u32 entryIndex;
    MatchIndex matchIndex;

    [[nodiscard]] constexpr auto rowIndex() const -> i32 { return matchIndex.get<"row">(); }

    [[nodiscard]] constexpr auto colIndex() const -> i32 { return matchIndex.get<"col">(); }
};

// The results of one file, with the matches and text they show kept in one block each instead of one allocation per
// result.
struct FileMatches {
    // The texts of the cells one after another, line breaks as `LINE_SEPARATOR`
    QString texts;
    // The names of the entries the results belong to; results of one entry in a row share a name
    QStringList entries;
    // The matches of the cells one after another
    vector<TextMatch> spans;
    vector<CellMatch> cells;

    [[nodiscard]] auto matches(const CellMatch& cell) const -> span<const TextMatch> {
        return span(spans).subspan(cell.matchStart, cell.matchCount);
    }

    // Appended, not written over the old matches: the other results point into `spans` too
    void setMatches(CellMatch& cell, const span<const TextMatch> found) {
        cell.matchStart = scast<u32>(spans.size());
        cell.matchCount = scast<u32>(found.size());
        spans.insert(spans.end(), found.begin(), found.end());
    }

    [[nodiscard]] auto text(const CellMatch& cell) const -> QStringView {
        return QStringView(texts).sliced(cell.textStart, cell.textLength);
    }

    // Empty if the result belongs to no entry
    [[nodiscard]] auto entry(const CellMatch& cell) const -> QStringView {
        return cell.entryIndex == NO_ENTRY ? QStringView() : QStringView(entries[scast<isize>(cell.entryIndex)]);
    }

    // Appended, not written over the old text: the other results point into `texts` too
    void setText(CellMatch& cell, const QStringView text) {
        cell.textStart = scast<u32>(texts.size());
        cell.textLength = scast<u32>(text.size());
        texts.append(text);

        const auto written = span(texts.data() + cell.textStart, cell.textLength);
        ranges::replace(written, QChar(u'\n'), LINE_SEPARATOR);
    }

    void setEntry(CellMatch& cell, const QStringView name) {
        if (name.isEmpty()) {
            return;
        }

        if (entries.isEmpty() || entries.last() != name) {
            entries.append(name.toString());
        }

        cell.entryIndex = scast<u32>(entries.size() - 1);
    }

    // The search is done adding: the slack is not needed
    void trim() {
        texts.squeeze();
        spans.shrink_to_fit();
        cells.shrink_to_fit();
    }
};

inline auto u16ToAscii(u16 number) -> array<char, 4> {
    array<char, 4> out{};

    // NOLINTBEGIN(readability-magic-numbers)
    if (number >= 1000) {
        out[0] = scast<char>('0' + (number / 1000));
        number %= 1000;
        out[1] = scast<char>('0' + (number / 100));
        number %= 100;
        out[2] = scast<char>('0' + (number / 10));
        out[3] = scast<char>('0' + (number % 10));
    } else if (number >= 100) {
        out[0] = scast<char>('0' + (number / 100));
        number %= 100;
        out[1] = scast<char>('0' + (number / 10));
        out[2] = scast<char>('0' + (number % 10));
    } else if (number >= 10) {
        out[0] = scast<char>('0' + (number / 10));
        out[1] = scast<char>('0' + (number % 10));
    } else {
        out[0] = scast<char>('0' + number);
    }
    // NOLINTEND(readability-magic-numbers)

    return out;
}

struct Selected {
    bitset<2048> mapIndices = 0;
    bitset<2048> validIndices = 0;
    u16 mapCount = 0;
    FileFlags flags = scast<FileFlags>(0);

    [[nodiscard]] auto empty() const -> bool {
        for (u16 idx = 0; idx < mapCount; idx++) {
            if (mapIndices[idx]) {
                return false;
            }
        }

        return flags == 0;
    }

    // RPG Maker 2000/2003's `RPG_RT.ldb` bundles all of it into one `Database` flag
    [[nodiscard]] auto forEngine(const EngineType engineType) const -> Selected {
        static constexpr FileFlags otherFamily = FileFlags_Actors | FileFlags_Armors | FileFlags_Classes |
                                                 FileFlags_CommonEvents | FileFlags_Enemies | FileFlags_Items |
                                                 FileFlags_Skills | FileFlags_States | FileFlags_Troops |
                                                 FileFlags_Weapons | FileFlags_System;

        Selected result = *this;

        if (engineType == EngineType::RM2K && (result.flags & otherFamily) != 0) {
            result.flags |= FileFlags_Database;
        }

        return result;
    }

    [[nodiscard]] auto filenames(const EngineType engineType) const -> vector<FilenameArray> {
        vector<FilenameArray> filenames;

        u16 mapFileCount = 0;

        {
            u32 dense = 0;

            for (u32 actual = 0; actual < validIndices.size() && dense < mapCount; actual++) {
                if (!validIndices[actual]) {
                    continue;
                }

                if (mapIndices[dense]) {
                    mapFileCount++;
                }

                dense++;
            }
        }

        u16 flagFileCount = 0;

        // FileFlags_Database is the last flag, so countr_zero + 1 is the flag count.
        //! Update if last FileFlags member changes.
        static constexpr i32 totalFlags = std::countr_zero(FileFlags_Database) + 1;

        for (i32 flagIdx = 1; flagIdx < totalFlags; flagIdx++) {
            const auto flag = scast<FileFlags>(1 << flagIdx);

            if ((flags & flag) != 0) {
                flagFileCount++;
            }
        }

        filenames.reserve(mapFileCount + flagFileCount);

        u32 dense = 0;

        for (u32 actual = 0; actual < validIndices.size() && dense < mapCount; actual++) {
            if (!validIndices[actual]) {
                continue;
            }

            if (mapIndices[dense]) {
                FilenameArray name{ "map" };

                const auto asciiNumber = u16ToAscii(actual);
                memcpy(name.data() + 3, asciiNumber.data(), 4);

                filenames.push_back(name);
            }

            dense++;
        }

        for (i32 flagIdx = 1; flagIdx < totalFlags; flagIdx++) {
            const auto flag = scast<FileFlags>(1 << flagIdx);

            if ((flags & flag) == 0) {
                continue;
            }

            FilenameArray name;

            switch (flag) {
                case FileFlags_Actors:
                    name = { "actors" };
                    break;
                case FileFlags_Armors:
                    name = { "armors" };
                    break;
                case FileFlags_Classes:
                    name = { "classes" };
                    break;
                case FileFlags_CommonEvents:
                    name = { "commonevents" };
                    break;
                case FileFlags_Enemies:
                    name = { "enemies" };
                    break;
                case FileFlags_Items:
                    name = { "items" };
                    break;
                case FileFlags_Skills:
                    name = { "skills" };
                    break;
                case FileFlags_States:
                    name = { "states" };
                    break;
                case FileFlags_Troops:
                    name = { "troops" };
                    break;
                case FileFlags_Weapons:
                    name = { "weapons" };
                    break;
                case FileFlags_System:
                    name = { "system" };
                    break;
                case FileFlags_Scripts:
                    if (engineType == EngineType::MVMZ) {
                        name = { "plugins" };
                    } else {
                        name = { "scripts" };
                    }
                    break;
                case FileFlags_Database:
                    name = { "terms" };
                    break;
                default:
                    throw std::runtime_error(format("Received {} flag. This shouldn't happen.", flag));
            }

            filenames.push_back(name);
        }

        return filenames;
    }
};

struct ContinueAnyway {};

struct Continue {
    QString s;
};

struct Abort {};

struct Retry {};

using ControlFlow = std::variant<ContinueAnyway, Continue, Abort, Retry>;

struct JSScript {
    BatchAction action;
    QString name;
    QString source;
    // Set by MainWindow once the user approved the script to run processes and touch files.
    bool hostApi = false;
};

// What a save file holds about its party. The member names are the keys of its JSON. `switches` and `variables` are
// indexed by the in-game ID, so index 0 is unused.
struct SaveActor {
    // Empty when the save has no name for the actor (RPG Maker 2000/2003 stores a name only after the game changed it).
    QString name;
    QString nickname;
    i32 actorId;
    i32 classId;
    i32 level;
    i32 hp;
    i32 mp;
    i32 tp;
};

struct SaveStack {
    i32 id;
    i32 count;
};

struct SaveSummary {
    // The actor IDs of the party, in order; `actors` holds every actor the save knows, the party's or not.
    vector<i32> party;
    vector<SaveActor> actors;
    // RPG Maker 2000/2003 has no weapons and armors apart from items; all of them are in `items`.
    vector<SaveStack> items;
    vector<SaveStack> weapons;
    vector<SaveStack> armors;
    vector<bool> switches;
    vector<i32> variables;
    // Playtime, 60 per second.
    u32 frames;
    i32 mapId;
    i32 gold;
};

struct LintEntry {
    QVariant extra;
    QVariant suggestions;
    QString captured;
    const char* translationId;
    i32 index;
    LintType type;
};
Q_DECLARE_METATYPE(LintEntry)
Q_DECLARE_METATYPE(QList<LintEntry>)

struct LintRow {
    QString filename;

    QString sourceText;
    QString translationText;

    vector<Span> sourceHighlights;
    vector<Span> translationHighlights;

    QString info;

    u32 lineNumber;
};

struct SearchMatch {
    const u32 start;
    const u32 len;
    const f32 score;
};

struct LintCharState {
    QList<LintEntry> entries;
    optional<QColor> background;
    optional<QColor> foreground;
    optional<QColor> underlineColor;
    QTextCharFormat::UnderlineStyle underlineStyle = QTextCharFormat::NoUnderline;
};

struct LintOutcome {
    vector<LintRow> rows;
    vector<LintCharState> charStates;
};

using CellMatches = HashMap<FilenameArray, FileMatches>;

struct SearchResult {
    CellMatches matches;
    vector<FilenameArray> filenames;
    u32 skippedCount;
};

struct ScriptSuccess {
    vector<FilenameArray> filenames;
    QStringList log;
    u32 skippedCount;
    // Files the script ran on. Zero means no file was read back, so the project need not be reopened.
    u32 processedCount;
};

struct SerdeSuccess {
    vector<FilenameArray> filenames;
    u32 skippedCount;
};

struct GlobalLintResult {
    vector<LintRow> rows;
    vector<FilenameArray> filenames;
    u32 skippedCount;
};

// Counts of the source column and of the first translation column of one file.
struct FileStatistics {
    FilenameArray filename;
    TextCounts source;
    TextCounts translation;
};

struct StatisticsResult {
    vector<FileStatistics> files;
    // Files that could not be read.
    vector<FilenameArray> skipped;
};

struct ScriptError {
    QString message;
    FilenameArray filename;
    u32 lineNumber;
};

struct TranslatedFiles {
    ByteBuffer files;
    ByteBuffer filesFFI;
};

struct ReplacedCell {
    QString text;
    vector<TextMatch> matches;
    // How many more (or fewer) rows of the file have a translation after the replacement
    i32 translatedDelta;
};

// How the number of translated rows of a file changed by a replacement
struct TranslatedDelta {
    FilenameArray filename;
    i32 delta;
};

struct ReplaceSuccess {
    // The files that couldn't be replaced in
    vector<FilenameArray> skipped;
    vector<TranslatedDelta> deltas;
};

using ExtractResult = expected<void, FFIString>;
using ReadResult = expected<ByteBuffer, FFIString>;
using WriteResult = expected<f32, FFIString>;
using PurgeResult = expected<void, FFIString>;
using LintResult = expected<FFIString, FFIString>;
using ScriptResult = expected<ScriptSuccess, ScriptError>;
using SerdeResult = expected<SerdeSuccess, FFIString>;
using ReplaceResult = ReplaceSuccess;

// What an endpoint answered, or the reason it didn't (`failed`): the latter must not be offered as a translation
struct EndpointAnswer {
    QString text;
    bool failed;
};

using TranslateSingleResult = vector<EndpointAnswer>;

// `stale`: the file was readable, but the cell is no longer the one that was searched
struct ReplaceSingleError {
    QString filename;
    bool stale;
};

using ReplaceSingleResult = expected<ReplacedCell, ReplaceSingleError>;

// The contents of `.rvpacker-metadata`
struct Metadata {
    HashMap<FilenameArray, u64> hashes;
    optional<string> lineSeparator;
    optional<string> lineBreak;
    optional<string> commentPrefix;
    optional<string> readEncoding;
    // A number, like the file has it
    optional<u8> duplicateMode;
};

// A hash is a `u64`, but rvpacker-txt-rs writes the same 64 bits as a signed number
template <>
struct glz::meta<Metadata> {
    using T = Metadata;

    static constexpr auto readHashes =
        [](T& metadata, const std::map<string, glz::raw_json>& input, glz::context& ctx) -> void {
        metadata.hashes.reserve(input.size());

        for (const auto& [name, raw] : input) {
            // Names are read into a `FilenameArray`, which has to hold them whole
            if (!fitsFilenameArray(name.size())) {
                ctx.error = glz::error_code::syntax_error;
                return;
            }

            const string_view text = raw.str;
            optional<u64> hash = stoa<u64>(text);

            if (!hash) {
                if (const optional<i64> signedHash = stoa<i64>(text)) {
                    hash = std::bit_cast<u64>(*signedHash);
                }
            }

            if (!hash) {
                ctx.error = glz::error_code::parse_number_failure;
                return;
            }

            FilenameArray filename{};
            copyFilenameArray(name.data(), name.size(), filename.data());
            metadata.hashes.insert({ filename, *hash });
        }
    };

    static constexpr auto writeHashes = [](const T& metadata) -> std::map<string, i64> {
        std::map<string, i64> out;

        for (const auto& [name, hash] : metadata.hashes) {
            out.emplace(string(name.data()), std::bit_cast<i64>(hash));
        }

        return out;
    };

    static constexpr auto value = glz::object(
        "hashes",
        glz::custom<readHashes, writeHashes>,
        "lineSeparator",
        &T::lineSeparator,
        "lineBreak",
        &T::lineBreak,
        "commentPrefix",
        &T::commentPrefix,
        "readEncoding",
        &T::readEncoding,
        "duplicateMode",
        &T::duplicateMode
    );
};
