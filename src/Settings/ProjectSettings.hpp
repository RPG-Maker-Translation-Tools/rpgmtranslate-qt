#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Hasher.hpp"
#include "glazemeta.hpp"
#include "rpgmtranslate_rs.h"

#include <QFile>

struct ColumnInfo {
    QString name;
    u16 width;
};

template <>
struct glz::meta<ColumnInfo> {
    static constexpr auto value = glz::array(&ColumnInfo::name, &ColumnInfo::width);
};

enum class SourceDirectory : u8 {
    None,
    UppercaseData,
    LowercaseData,
    /// RPG Maker 2000/2003 data lives at the project root
    Root,
};

struct ProjectSettings {
    HashMap<FilenameArray, u64> hashes;
    HashMap<QString, QString> fileContexts;

    QStringList completedFiles;

    vector<ColumnInfo> columns;

    QString projectPath;
    QString spellcheckDictionary;

    QString projectContext;

    QString sourceLang;
    QString translationLang;

    u16 lineLengthHint = 0;

    DuplicateMode duplicateMode = DuplicateMode::Allow;
    BaseFlags flags = BaseFlags(0);

    SourceDirectory sourceDirectory = SourceDirectory::None;
    EngineType engineType = EngineType::MVMZ;

    /// A codepage's WHATWG label (`"Shift_JIS"`, `"windows-1252"`, ...), or
    /// empty for "guess it" - see `rvpacker_txt_rs_lib::Processor::readEncoding`.
    /// Needed for RPG Maker 2000/2003, whose files carry no encoding of their
    /// own; optional elsewhere. Chosen once via `ReadMenu` when the project is
    /// first read; independent of `writeEncoding` below.
    QString readEncoding;

    /// A codepage's WHATWG label, or empty (the default) to always write
    /// translated text as UTF-8 - see `rvpacker_txt_rs_lib::Processor::writeEncoding`.
    /// Leaving this empty is deliberate: a translation is not generally
    /// representable in the source game's own codepage, so guessing otherwise
    /// risks silently corrupting it. Set from the "Write encoding" field in
    /// `SettingsWindow`'s Project tab, not from any per-write prompt - it
    /// takes effect on every write from then on.
    QString writeEncoding;

    /// # Panics
    ///
    /// RPG Maker 2000/2003 files don't follow the "one extension per entity
    /// file" convention the other engines use - `RPG_RT.ldb`/`.lmt`/`.lmu`
    /// each have their own fixed name, not a per-kind extension - so this is
    /// never meaningful to call for it.
    [[nodiscard]] constexpr auto engineExtension() const -> QStringView {
        switch (engineType) {
            case EngineType::MVMZ:
                return u"json";
                break;
            case EngineType::VXAce:
                return u"rvdata2";
                break;
            case EngineType::VX:
                return u"rvdata";
                break;
            case EngineType::XP:
                return u"rxdata";
                break;
            case EngineType::RM2K:
                std::unreachable();
        }
    }

    /// # Panics
    ///
    /// See the member overload's docs.
    [[nodiscard]] static constexpr auto engineExtension(const EngineType engineType) -> QStringView {
        switch (engineType) {
            case EngineType::MVMZ:
                return u"json";
                break;
            case EngineType::VXAce:
                return u"rvdata2";
                break;
            case EngineType::VX:
                return u"rvdata";
                break;
            case EngineType::XP:
                return u"rxdata";
                break;
            case EngineType::RM2K:
                std::unreachable();
        }

        std::unreachable();
    }

    [[nodiscard]] auto programDataPath() const -> QString { return projectPath + PROGRAM_DATA_DIRECTORY; }

    [[nodiscard]] auto sourcePath() const -> QString {
        switch (sourceDirectory) {
            case SourceDirectory::UppercaseData:
                return projectPath + u"/Data";
            case SourceDirectory::LowercaseData:
                return projectPath + u"/data";
            case SourceDirectory::Root:
                return projectPath;
            default:
                std::unreachable();
        }
    }

    [[nodiscard]] auto baselineSourcePath() const -> QString { return programDataPath() + BASELINE_DATA_DIRECTORY; }

    [[nodiscard]] auto actualSourcePath() const -> QString {
        QString path = baselineSourcePath();

        if (QFile::exists(path)) {
            return path;
        }

        return sourcePath();
    }

    [[nodiscard]] auto translationPath() const -> QString { return programDataPath() + TRANSLATION_DIRECTORY; }

    [[nodiscard]] auto projectSettingsPath() const -> QString { return programDataPath() + PROJECT_SETTINGS_FILE; }

    [[nodiscard]] auto backupPath() const -> QString { return programDataPath() + BACKUP_DIRECTORY; }

    [[nodiscard]] auto outputPath() const -> QString { return programDataPath() + OUTPUT_DIRECTORY; }

    [[nodiscard]] auto glossaryPath() const -> QString { return programDataPath() + GLOSSARY_FILE; }
};

template <>
struct glz::meta<ProjectSettings> {
    using T = ProjectSettings;
    static constexpr auto value = glz::object(
        "engineType",
        &T::engineType,
        "sourceLang",
        &T::sourceLang,
        "translationLang",
        &T::translationLang,
        "duplicateMode",
        &T::duplicateMode,
        "flags",
        &T::flags,
        "hashes",
        &T::hashes,
        "completed",
        &T::completedFiles,
        "lineLengthHint",
        &T::lineLengthHint,
        "translationColumns",
        &T::columns,
        "sourceDirectory",
        &T::sourceDirectory,
        "spellcheckDictionaryPath",
        &T::spellcheckDictionary,
        "projectContext",
        &T::projectContext,
        "fileContexts",
        &T::fileContexts,
        "encoding",
        &T::readEncoding,
        "writeEncoding",
        &T::writeEncoding
    );
};