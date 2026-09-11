#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
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

struct CellMatch {
    TextMatch* matches;
    u32 matchesCount;
    MatchIndex matchIndex;

    [[nodiscard]] constexpr auto rowIndex() const -> i32 { return matchIndex.get<"row">(); }

    [[nodiscard]] constexpr auto colIndex() const -> i32 { return matchIndex.get<"col">(); }
};

inline auto u16ToAscii(u16 number) -> array<char, 4> {
    array<char, 4> out;

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
        out[3] = '\0';
    } else if (number >= 10) {
        out[0] = scast<char>('0' + (number / 10));
        out[1] = scast<char>('0' + (number % 10));
        out[2] = '\0';
    } else {
        out[0] = scast<char>('0' + number);
        out[1] = '\0';
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
        for (i32 idx = 0; idx < mapCount; idx++) {
            if (mapIndices[idx]) {
                return false;
            }
        }

        return flags == 0;
    }

    // RPG Maker 2000/2003 has no `Actors`/`Skills`/.../`System` file kinds of
    // its own - `RPG_RT.ldb` bundles all of it into one `Database` flag - but
    // its `.txt` outputs share the same names (`actors.txt`, `skills.txt`,
    // ...), so `FileSelectMenu`'s checkboxes still set the MV/VX-shaped bits.
    // Call this once, right before handing `flags` to the library, to fold
    // any of those bits into `Database` for an RM2K project.
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

        // This recovers the number by which FileFlags_Database was shifted
        // and since FileFlags_Database is the last flag it effectively
        // gives us flag count after adding 1 to the result of countr_zero.
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
                    name = { "database" };
                    break;
                case FileFlags_Map:
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
};

struct LintEntry {
    QVariant extra;
    // Clickable replacement suggestions (spellcheck word list, LanguageTool replacements) - kept
    // separate from `extra` since some lint kinds need both a free-form tooltip text and a
    // suggestion list at once.
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

using CellMatches = HashMap<FilenameArray, vector<CellMatch>>;

struct SearchResult {
    CellMatches matches;
    vector<FilenameArray> filenames;
    u32 skippedCount;
};

struct ScriptSuccess {
    vector<FilenameArray> filenames;
    u32 skippedCount;
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
    TextMatch* matches;
};

using ExtractResult = expected<void, FFIString>;
using ReadResult = expected<ByteBuffer, FFIString>;
using WriteResult = expected<f32, FFIString>;
using PurgeResult = expected<void, FFIString>;
using LintResult = expected<FFIString, FFIString>;
using ScriptResult = expected<ScriptSuccess, ScriptError>;
using SerdeResult = expected<SerdeSuccess, FFIString>;
using TranslateResult = expected<TranslatedFiles, FFIString>;
using BatchResult = std::variant<ScriptResult, TranslateResult>;
using ReplaceResult = vector<FilenameArray>;
using TranslateSingleResult = vector<QString>;
using ReplaceSingleResult = expected<ReplacedCell, QString>;

struct Metadata {
    std::map<FilenameArray, u64> hashes;
    optional<string> readEncoding;
    DuplicateMode duplicateMode;
};