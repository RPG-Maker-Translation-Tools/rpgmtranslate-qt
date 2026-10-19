#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Notice.hpp"
#include "QtNumbers.hpp"
#include "QtStrings.hpp"
#include "rpgmtranslate_rs.h"

#include <concepts>

constexpr static usize TAG_SHIFT = 56;
constexpr static usize TAG_MASK = 0xFFULL << TAG_SHIFT;  // 0xFF00000000000000
constexpr static usize ADDR_MASK = ~TAG_MASK;

static inline auto set_tag(void* const ptr, const u8 tag) -> u64 {
    return (rcast<u64>(ptr) & ADDR_MASK) | (scast<u64>(tag) << TAG_SHIFT);
}

static inline auto get_tag(const u64 tagged) -> u8 {
    return scast<u8>(tagged >> TAG_SHIFT);
}

static inline auto strip_tag(const u64 tagged) -> void* {
    return rcast<void*>(tagged & ADDR_MASK);
}

static inline auto replace_addr(const u64 tagged, void* const new_addr) -> u64 {
    const u64 preserved_tag = tagged & TAG_MASK;
    const u64 new_addr_bits = rcast<u64>(new_addr) & ADDR_MASK;
    return preserved_tag | new_addr_bits;
}

[[nodiscard]] auto dataLocation() -> QString;

struct Translation {
   private:
    QStringView translation;

   public:
    Translation(const QStringView view, const u8 idx) {
        setTranslation(view);
        setIndex(idx);
    }

    void setIndex(const u8 idx) {
        translation = QStringView(rcast<QChar*>(set_tag(ccast<QChar*>(translation.data()), idx)), translation.size());
    }

    [[nodiscard]] auto getIndex() const -> u8 { return get_tag(rcast<u64>(translation.data())); }

    void setTranslation(const QStringView translation) {
        this->translation = QStringView(
            rcast<QChar*>(replace_addr(rcast<u64>(this->translation.data()), ccast<QChar*>(translation.data()))),
            translation.size()
        );
    }

    [[nodiscard]] auto getTranslation() const -> QStringView {
        return { rcast<QChar*>(strip_tag(rcast<u64>(translation.data()))), translation.size() };
    }
};

[[nodiscard]] auto lineParts(QStringView line, u32 lineNumber, QAnyStringView filename) -> QSVList;

// The part of a row's source column that is text to translate: the text itself, the map display name without its
// prefix, or nothing for the other comments (`<!>...`) and bookmarks.
[[nodiscard]] auto translatableSource(QStringView source) -> optional<QStringView>;

// Words and characters of `text` as a player reads them: RPG Maker codes are not counted. Thread-safe.
// `text` is one entry per line, so the `lines` line feeds that join them are not characters of the text.
[[nodiscard]] auto countText(const QString& text, u32 lines) -> TextCounts;

// If `line` is a `<!>NAME<#>x` row, `x`: the name of the entry (actor, item, map...) that the rows after it belong to.
[[nodiscard]] auto entryNameOfLine(QStringView line) -> optional<QStringView>;

// The name of the entry the row at `rowIndex` belongs to: that of the nearest `<!>NAME` row at or before it, not
// looking past the `<!>ID` row that starts the entry. Empty if there is none.
[[nodiscard]] auto entryNameFor(const QSVList& lines, usize rowIndex) -> QStringView;
[[nodiscard]] auto getTranslation(const QSVList& parts) -> struct Translation;
[[nodiscard]] auto getSource(const QSVList& parts) -> QStringView;
[[nodiscard]] auto getTranslations(const QSVList& parts) -> QSVList;
// Whether a row has a translation: one of its translation columns is not empty, as `openProject` counts them.
[[nodiscard]] auto hasTranslation(const QSVList& parts) -> bool;
// Whether a non-blank line of a translation file is a row to translate: a comment is not, but a map's display name is.
[[nodiscard]] auto isRowLine(QStringView line) -> bool;

struct Abort;

// Atomic: the target is only replaced by `commitFile`; dropping the file without committing leaves it untouched.
[[nodiscard]] auto openForWriting(QWidget* parent, QString& path, QStringView suffix)
    -> Result<unique_ptr<QSaveFile>, Abort>;

// Flush any `QTextStream` on `file` first. Shows a notice and returns false on failure.
[[nodiscard]] auto commitFile(QWidget* parent, QSaveFile& file) -> bool;

static constexpr QStringView BACKUP_SUFFIX = u".bak";
static constexpr QStringView CORRUPT_SUFFIX = u".corrupt";

// Copies the existing, non-empty `path` to `path.bak`. Call before replacing a file whose loss would hurt (settings,
// glossary), so that `readJsonFile` has an older version to fall back to.
void keepBackup(const QString& path);

[[nodiscard]] static constexpr auto strtoffi(const QByteArrayView utf8) -> FFIString {
    return { .ptr = utf8.data(), .len = scast<u32>(utf8.size()) };
}

[[nodiscard]] static constexpr auto ffitostr(const FFIString str) -> QUtf8SV {
    return { str.ptr, scast<isize>(str.len) };
}

// Names a `FilenameArray` holds whole: the 4 to 15 character ones that `copyFilenameArray` copies correctly
[[nodiscard]] constexpr auto fitsFilenameArray(const usize size) -> bool {
    return size >= 4 && size < sizeof(FilenameArray);
}

// Callers check `fitsFilenameArray` first: below 4 bytes the last branch is unreachable
static inline void copyFilenameArray(const char* const src, const usize size, char* const dst) {
    if (size >= sizeof(u64)) {
        u64 tmpA;
        u64 tmpB;
        memcpy(&tmpA, src, sizeof(u64));
        memcpy(&tmpB, src + size - sizeof(u64), sizeof(u64));
        memcpy(dst, &tmpA, sizeof(u64));
        memcpy(dst + size - sizeof(u64), &tmpB, sizeof(u64));
    } else if (size >= sizeof(u32)) {
        u32 tmpA;
        u32 tmpB;
        memcpy(&tmpA, src, sizeof(u32));
        memcpy(&tmpB, src + size - sizeof(u32), sizeof(u32));
        memcpy(dst, &tmpA, sizeof(u32));
        memcpy(dst + size - sizeof(u32), &tmpB, sizeof(u32));
    } else {
        std::unreachable();
    }
}

static constexpr i32 WINDOW_COLOR_COUNT = 32;

// `sets` decides where the project keeps its system graphics, and for RPG Maker 2000/2003 which one the database
// selects.
auto getWindowColors(const ProjectSettings* sets) -> Result<array<QRgb, WINDOW_COLOR_COUNT>, Notice>;

auto getIcon(i32 iconIndex, const ProjectSettings* sets) -> Result<QPixmap, Notice>;

// Where a project keeps its saves: MV/MZ in a `save` directory, the other engines next to the executable.
struct SaveLocation {
    QString directory;
    QStringList filter;
    bool recursive;
};

[[nodiscard]] auto saveLocation(const ProjectSettings* sets) -> SaveLocation;

// The summary of `sets->activeSave`, which must not be empty. Parsed once per modification of the file, so it is cheap
// to ask for on every tooltip.
[[nodiscard]] auto getSaveSummary(const ProjectSettings* sets) -> Result<shared_ptr<const SaveSummary>, Notice>;

[[nodiscard]] constexpr auto serdeFormatExtension(const SerdeFormat format) -> QStringView {
    switch (format) {
        case SerdeFormat::Csv:
            return u".csv";
        case SerdeFormat::Xlsx:
            return u".xlsx";
        case SerdeFormat::Xml:
            return u".xml";
        case SerdeFormat::Json:
            return u".json";
        case SerdeFormat::Yaml:
            return u".yaml";
    }

    std::unreachable();
}