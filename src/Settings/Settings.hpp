#pragma once

#include "Constants.hpp"

#include <QJsonArray>
#include <QJsonObject>

struct Backup {
    u16 period = MIN_BACKUP_PERIOD;
    u8 max = MAX_BACKUPS;

    bool enabled = true;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> Backup;
};

struct CoreSettings {
    QStringList recentProjects;
    QString projectPath;

    Backup backup;

    bool checkForUpdates = true;
    bool firstLaunch = true;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> CoreSettings;
};

struct AppearanceSettings {
    QString translationTableFont;
    QString style;

    Qt::ColorScheme theme = Qt::ColorScheme::Unknown;
    QLocale::Language language = QLocale().language();

    u8 translationTableFontSize = 0;

    bool displayPercents = false;
    bool displayTrailingWhitespace = false;
    bool displayWordsAndCharacters = false;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> AppearanceSettings;
};

struct GoogleEndpointSettings {
    bool singleTranslation = false;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> GoogleEndpointSettings;
};

struct YandexEndpointSettings {
    QString apiKey;
    QString folderId;
    bool singleTranslation = false;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> YandexEndpointSettings;
};

struct DeepLEndpointSettings {
    QString apiKey;
    bool useGlossary = false;
    bool singleTranslation = false;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> DeepLEndpointSettings;
};

struct LLMSettings {
    constexpr static f32 DEFAULT_TEMPERATURE = 0.3F;
    constexpr static u16 DEFAULT_TOKEN_LIMIT = 4000;

    QString apiKey;
    QString baseUrl;
    QString model;
    QString systemPrompt;
    QString singleTranslateSystemPrompt;

    f32 temperature = DEFAULT_TEMPERATURE;
    u16 tokenLimit = DEFAULT_TOKEN_LIMIT;
    u16 outputTokenLimit = UINT16_MAX;

    bool useGlossary = false;
    bool thinking = false;

    bool singleTranslation = false;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> LLMSettings;
};

struct TranslationSettings {
    GoogleEndpointSettings google;
    YandexEndpointSettings yandex;
    DeepLEndpointSettings deepl;

    LLMSettings chatgpt;
    LLMSettings claude;
    LLMSettings gemini;
    LLMSettings deepseek;

    LLMSettings openaiCompatible;
    LLMSettings ollama;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> TranslationSettings;
};

struct ControlSettings {
    QString searchPanel = u"Ctrl+R"_s;
    QString tabPanel = u"Tab"_s;
    QString goToRow = u"Ctrl+G"_s;
    QString batchMenu = u"Ctrl+B"_s;
    QString bookmarkMenu = u"Alt+B"_s;
    QString matchMenu = u"Ctrl+M"_s;
    QString glossaryMenu = u"Alt+B"_s;
    QString translationsMenu = u"Ctrl+S"_s;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> ControlSettings;
};

struct Settings {
    CoreSettings core;
    ControlSettings controls;
    AppearanceSettings appearance;
    TranslationSettings translation;

    [[nodiscard]] auto toJSON() const -> QJsonObject;
    static auto fromJSON(const QJsonObject& obj) -> Settings;
};
