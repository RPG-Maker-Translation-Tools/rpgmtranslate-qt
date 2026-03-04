#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <bitset>

struct MatchModeInfo {
    f32 fuzzyThreshold;
    MatchMode mode;
    bool caseSensitive;
    bool permissive;
};

struct Term {
    QString term;
    QString translation;
    QString note;

    MatchModeInfo sourceMatchMode;
    MatchModeInfo translationMatchMode;
};

struct Glossary {
    vector<Term> terms;

    static auto matchModeInfotoJSON(const MatchModeInfo& info) -> QJsonObject {
        return { { u"fuzzyThreshold"_s, info.fuzzyThreshold },
                 { u"mode"_s, u8(info.mode) },
                 { u"caseSensitive"_s, info.caseSensitive },
                 { u"permissive"_s, info.permissive } };
    }

    static auto matchModeInfofromJSON(const QJsonObject& obj) -> MatchModeInfo {
        return { .fuzzyThreshold = f32(obj["fuzzyThreshold"_L1].toDouble()),
                 .mode = MatchMode(obj["mode"_L1].toInt()),
                 .caseSensitive = obj["caseSensitive"_L1].toBool(),
                 .permissive = obj["permissive"_L1].toBool() };
    }

    static auto fromJSON(const QJsonArray& array) -> Glossary {
        Glossary glossary;

        for (const auto& value : array) {
            if (!value.isObject()) {
                continue;
            }

            const QJsonObject obj = value.toObject();
            Term term;

            term.term = obj["term"_L1].toString();
            term.translation = obj["translation"_L1].toString();
            term.note = obj["note"_L1].toString();

            term.sourceMatchMode =
                matchModeInfofromJSON(obj["sourceMatchMode"_L1].toObject());
            term.translationMatchMode = matchModeInfofromJSON(
                obj["translationMatchMode"_L1].toObject()
            );

            glossary.terms.emplace_back(std::move(term));
        }

        return glossary;
    }

    [[nodiscard]] auto toJSON() const -> QJsonArray {
        QJsonArray array;

        for (const auto& term : terms) {
            array.append(
                QJsonObject{ {
                    { u"term"_s, term.term },
                    { u"translation"_s, term.translation },
                    { u"note"_s, term.note },
                    { u"sourceMatchMode"_s,
                      matchModeInfotoJSON(term.sourceMatchMode) },
                    { u"translationMatchMode"_s,
                      matchModeInfotoJSON(term.translationMatchMode) },
                } }
            );
        }

        return array;
    };
};

struct Span {
    u32 start;
    u32 len;
};

struct TextMatch {
    u64 bits;

    static constexpr u8 LEN_SHIFT = 32;
    static constexpr u8 CAP_SHIFT = 63;

    static constexpr u64 START_MASK = 0xFFFFFFFFULL;
    static constexpr u64 LEN_MASK = 0x7FFFFFFFULL;

    constexpr explicit TextMatch() = default;

    constexpr explicit TextMatch(
        const u32 start,
        const u32 len,
        const bool captured
    ) {
        setStart(start);
        setLen(len);
        setCaptured(captured);
    };

    [[nodiscard]] auto start() const -> u32 { return bits & START_MASK; }

    [[nodiscard]] auto len() const -> u32 {
        return (bits >> LEN_SHIFT) & LEN_MASK;
    }

    [[nodiscard]] auto capture() const -> bool {
        return (bits >> CAP_SHIFT) != 0;
    }

    void setStart(const u32 start) { bits = (bits & ~START_MASK) | u64(start); }

    void setLen(const u32 len) {
        bits = (bits & ~(LEN_MASK << LEN_SHIFT)) | (u64(len) << LEN_SHIFT);
    }

    void setCaptured(const bool captured) {
        bits = (bits & ~(1ULL << CAP_SHIFT)) | (u64(captured) << CAP_SHIFT);
    }
};

struct MatchIndex {
    u32 bits;

    constexpr explicit MatchIndex() = default;

    constexpr explicit MatchIndex(const u32 rowIndex, const u8 colIndex) {
        setRowIndex(rowIndex);
        setColIndex(colIndex);
    };

    static constexpr u32 ROW_MASK = 0xFF00'0000U;
    static constexpr u32 COL_SHIFT = 24;
    static constexpr u32 COL_MASK = 0x00FF'FFFFU;

    [[nodiscard]] constexpr auto rowIndex() const -> u32 {
        return bits & COL_MASK;
    }

    [[nodiscard]] constexpr auto colIndex() const -> u8 {
        return u8(bits >> COL_SHIFT);
    }

    constexpr void setRowIndex(const u32 rowIndex) {
        bits = (bits & ROW_MASK) | (rowIndex & COL_MASK);
    }

    constexpr void setColIndex(const u8 colIndex) {
        bits = (bits & COL_MASK) | (u32(colIndex) << COL_SHIFT);
    }
};

struct CellMatch {
    TextMatch* matches;
    u32 matchesCount;
    MatchIndex matchIndex;

    [[nodiscard]] constexpr auto rowIndex() const -> u32 {
        return matchIndex.rowIndex();
    }

    [[nodiscard]] constexpr auto colIndex() const -> u8 {
        return matchIndex.colIndex();
    }
};

inline auto u16ToAscii(u16 number) -> array<char, 4> {
    array<char, 4> out;

    if (number >= 1000) {
        out[0] = char('0' + (number / 1000));
        number %= 1000;
        out[1] = char('0' + (number / 100));
        number %= 100;
        out[2] = char('0' + (number / 10));
        out[3] = char('0' + (number % 10));
    } else if (number >= 100) {
        out[0] = char('0' + (number / 100));
        number %= 100;
        out[1] = char('0' + (number / 10));
        out[2] = char('0' + (number % 10));
        out[3] = '\0';
    } else if (number >= 10) {
        out[0] = char('0' + (number / 10));
        out[1] = char('0' + (number % 10));
        out[2] = '\0';
        out[3] = '\0';
    } else {
        out[0] = char('0' + number);
        out[1] = '\0';
        out[2] = '\0';
        out[3] = '\0';
    }

    return out;
}

struct Selected {
    std::bitset<2048> mapIndices = 0;
    std::bitset<2048> validIndices = 0;
    u16 mapCount = 0;
    FileFlags flags = FileFlags(0);

    [[nodiscard]] auto empty() const -> bool {
        for (const u16 idx : range<u16>(0, mapCount)) {
            if (mapIndices[idx]) {
                return false;
            }
        }

        return flags == 0;
    }

    [[nodiscard]] auto filenames(const EngineType engineType) const
        -> vector<FilenameArray> {
        vector<FilenameArray> filenames;

        u16 mapFileCount = 0;

        {
            u16 dense = 0;

            for (u16 actual = 0;
                 actual < validIndices.size() && dense < mapCount;
                 actual++) {
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

        for (u16 flagIdx :
             range<u16>(0, magic_enum::enum_count<FileFlags>() - 2)) {
            const auto flag = FileFlags(1 << flagIdx);

            if ((flags & flag) != 0 && flag != Map) {
                flagFileCount++;
            }
        }

        filenames.reserve(mapFileCount + flagFileCount);

        u16 dense = 0;

        for (u16 actual = 0; actual < validIndices.size() && dense < mapCount;
             actual++) {
            if (!validIndices[actual]) {
                continue;
            }

            if (mapIndices[dense]) {
                FilenameArray name;

                name[0] = 'm';
                name[1] = 'a';
                name[2] = 'p';

                const auto asciiNumber = u16ToAscii(actual);

                memcpy(name.data() + 3, asciiNumber.data(), 4);
                name[7] = '\0';

                filenames.push_back(name);
            }

            ++dense;
        }

        for (u16 flagIdx :
             range<u16>(1, magic_enum::enum_count<FileFlags>() - 2)) {
            const auto flag = FileFlags(1 << flagIdx);

            if ((flags & flag) == 0) {
                continue;
            }

            FilenameArray name;

            switch (flag) {
                case FileFlags::Actors:
                    memcpy(name.data(), "actors", 7);
                    break;
                case FileFlags::Armors:
                    memcpy(name.data(), "armors", 7);
                    break;
                case FileFlags::Classes:
                    memcpy(name.data(), "classes", 8);
                    break;
                case FileFlags::CommonEvents:
                    memcpy(name.data(), "commonevents", 13);
                    break;
                case FileFlags::Enemies:
                    memcpy(name.data(), "enemies", 8);
                    break;
                case FileFlags::Items:
                    memcpy(name.data(), "items", 6);
                    break;
                case FileFlags::Skills:
                    memcpy(name.data(), "skills", 7);
                    break;
                case FileFlags::States:
                    memcpy(name.data(), "states", 7);
                    break;
                case FileFlags::Troops:
                    memcpy(name.data(), "troops", 7);
                    break;
                case FileFlags::Weapons:
                    memcpy(name.data(), "weapons", 8);
                    break;
                case FileFlags::System:
                    memcpy(name.data(), "system", 7);
                    break;
                case FileFlags::Scripts:
                    if (engineType == EngineType::New) {
                        memcpy(name.data(), "plugins", 8);
                    } else {
                        memcpy(name.data(), "scripts", 8);
                    }
                    break;
                case FileFlags::Map:
                case FileFlags::Other:
                case FileFlags::All:
                    std::unreachable();
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