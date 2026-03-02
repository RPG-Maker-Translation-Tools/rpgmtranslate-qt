#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QStringList>

struct ColumnInfo {
    QString name;
    u16 width;
};

struct ProjectSettings {
    vector<u128> hashes;
    QStringList completed;

    vector<ColumnInfo> columns;

    QString projectPath;
    QString sourceDirectory;

    QString projectContext;
    QVariantMap fileContexts;

    u16 sourceColumnWidth = DEFAULT_COLUMN_WIDTH;
    u16 lineLengthHint = 0;

    EngineType engineType = EngineType::New;

    Algorithm sourceLang = Algorithm::None;
    Algorithm translationLang = Algorithm::None;

    DuplicateMode duplicateMode = DuplicateMode::Allow;
    BaseFlags flags = BaseFlags(0);

    [[nodiscard]] auto programDataPath() const -> QString {
        return projectPath + PROGRAM_DATA_DIRECTORY;
    }

    [[nodiscard]] auto sourcePath() const -> QString {
        return projectPath + sourceDirectory;
    }

    [[nodiscard]] auto translationPath() const -> QString {
        return programDataPath() + TRANSLATION_DIRECTORY;
    }

    [[nodiscard]] auto projectSettingsPath() const -> QString {
        return programDataPath() + PROJECT_SETTINGS_FILE;
    }

    [[nodiscard]] auto backupPath() const -> QString {
        return programDataPath() + BACKUP_DIRECTORY;
    }

    [[nodiscard]] auto outputPath() const -> QString {
        return programDataPath() + PROGRAM_DATA_DIRECTORY + OUTPUT_DIRECTORY;
    }

    [[nodiscard]] auto glossaryPath() const -> QString {
        return programDataPath() + GLOSSARY_FILE;
    }

    [[nodiscard]] auto serializeTranslationColumns() const -> QJsonArray;
    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> ProjectSettings;
};