#pragma once

#include "Constants.hpp"
#include "PluginSequences.hpp"
#include "glazemeta.hpp"
#include "rpgmtranslate_rs.h"

#include <QLocale>
#include <QStringList>
#include <variant>

enum ReasoningEffort : u8 {
    Low,
    Medium,
    High
};

struct Backup {
    u16 period = MIN_BACKUP_PERIOD;
    u8 max = MAX_BACKUPS;
    bool enabled = true;
};

struct GitSettings {
    QString username;
    QString useremail;
    bool preferGitConfig = true;
};

struct CoreSettings {
    GitSettings git;
    QStringList recentProjects;
    QString projectPath;
    Backup backup;
    bool checkForAppUpdates = true;
    bool checkForSourceUpdates = true;
};

struct AppearanceSettings {
    QString translationTableFont;
    QString style;
    Qt::ColorScheme theme = Qt::ColorScheme::Unknown;
    QLocale::Language language = QLocale().language();
    u8 translationTableFontSize = 0;
    bool displayPercents = false;
};

struct EndpointSettings {
    static constexpr i32 DEFAULT_TOKEN_LIMIT = 4000;

    // Stable identifier, immutable after creation - used as the OS keyring account name for
    // this endpoint's API key, so renaming the endpoint doesn't orphan its stored credential.
    QString id;

    QString name;
    QString apiKey;
    QString yandexFolderID;
    QString baseUrl;
    QString model;
    QString systemPrompt;
    QString singleTranslateSystemPrompt;

    optional<f32> temperature;
    optional<f32> frequencyPenalty;
    optional<f32> precensePenalty;
    optional<f32> topP;

    u16 tokenLimit = DEFAULT_TOKEN_LIMIT;
    u16 outputTokenLimit = UINT16_MAX;
    u16 thinkingBudget = UINT16_MAX;

    ReasoningEffort reasoningEffort = ReasoningEffort::Low;

    bool useGlossary = false;
    bool thinking = false;
    bool singleTranslation = false;

    TranslationEndpoint type;
};

struct LanguageToolSettings {
    QString baseURL;
    QString apiKey;
    QString username;
    QString motherTongue;
    QString preferredVariants;
    QString dicts;
    QString enabledRules;
    QString disabledRules;
    QString enabledCategories;
    QString disabledCategories;
    bool picky = false;
    bool enabledOnly = false;
};

enum LintFlags : u16 {
    LintingDisabled = 1 << 0,
    LeadingWhitespace = 1 << 1,
    TrailingWhitespace = 1 << 2,
    ContiguousWhitespace = 1 << 3,
    UnclosedPunctuation = 1 << 4,
    TagMismatch = 1 << 5,
    Spellcheck = 1 << 6,
    LanguageTool = 1 << 7,
    TermMismatch = 1 << 8,
};

struct EndpointContext {
    u8 endpointIndex;
    QString context;
};

using BatchVariant = std::variant<LintFlags, EndpointContext, u8>;

struct Replacement {
    QString source;
    QString replacement;
    bool enabled = false;
};

struct SequenceSettings {
    QRgb customColor = 0;
    bool useCustomColor = false;
    bool disabled = false;
};

struct CustomLint {
    QString sequence;
    QString tooltip;
    QRgb customColor = 0;
    bool caseInsensitive = false;
    bool useCustomColor = false;
    bool disabled = false;
};

struct TranslationSettings {
    LanguageToolSettings languageTool;

    array<SequenceSettings, TextCodes::PATTERN_PARTS.size()> textCodes;
    array<SequenceSettings, Notetags::PATTERN_PARTS.size()> notetags;
    array<SequenceSettings, PluginCommands::PATTERN_PARTS.size()> pluginCommands;
    array<SequenceSettings, CommentTags::PATTERN_PARTS.size()> commentTags;

    vector<CustomLint> customLints;

    vector<EndpointSettings> endpoints;

    vector<Replacement> replacements = {
        Replacement(u"<<"_s, u"\u00AB"_s, false), Replacement(u">>"_s, u"\u00BB"_s, false),
        Replacement(u"--"_s, u"\u2014"_s, false), Replacement(u",,"_s, u"\u201E"_s, false),
        Replacement(u"''"_s, u"\u201C"_s, false),
    };
    vector<u32> whitespaceCharacters;

    LintFlags lintFlags = scast<LintFlags>(ranges::fold_left(enchantum::values<LintFlags>, 0, std::bit_or<>{})) &
                          ~LintFlags::LintingDisabled;

    bool displayWordsAndCharacters = false;
};

struct ControlSettings {
    QString searchPanel = u"Ctrl+R"_s;
    QString tabPanel = u"Tab"_s;
    QString goToRow = u"Ctrl+G"_s;
    QString batchMenu = u"Ctrl+B"_s;
    QString bookmarkMenu = u"Alt+B"_s;
    QString lintMenu = u"Ctrl+M"_s;
    QString glossaryMenu = u"Alt+L"_s;
    QString translationsMenu = u"Ctrl+S"_s;
};

struct Settings {
    TranslationSettings translation;
    ControlSettings controls;
    CoreSettings core;
    AppearanceSettings appearance;
};
