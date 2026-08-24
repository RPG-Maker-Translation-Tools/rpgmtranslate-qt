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
        }
    }

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
        &T::fileContexts
    );
};