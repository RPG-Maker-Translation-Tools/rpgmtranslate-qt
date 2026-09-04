#pragma once

#include "Constants.hpp"
#include "PluginSequences.hpp"
#include "glazemeta.hpp"
#include "magic_enum.hpp"
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

template <>
struct glz::meta<Backup> {
    static constexpr auto value =
        glz::object("enabled", &Backup::enabled, "period", &Backup::period, "max", &Backup::max);
};

struct GitSettings {
    QString username;
    QString useremail;
    bool preferGitConfig = true;
};

template <>
struct glz::meta<GitSettings> {
    static constexpr auto value = glz::object(
        "username",
        &GitSettings::username,
        "useremail",
        &GitSettings::useremail,
        "preferGitConfig",
        &GitSettings::preferGitConfig
    );
};

struct CoreSettings {
    QStringList recentProjects;
    QString projectPath;
    Backup backup;
    bool checkForAppUpdates = true;
    bool checkForSourceUpdates = true;
    GitSettings git;
};

template <>
struct glz::meta<CoreSettings> {
    static constexpr auto value = glz::object(
        "recentProjects",
        &CoreSettings::recentProjects,
        "projectPath",
        &CoreSettings::projectPath,
        "backup",
        &CoreSettings::backup,
        "checkForAppUpdates",
        &CoreSettings::checkForAppUpdates,
        "checkForSourceUpdates",
        &CoreSettings::checkForSourceUpdates,
        "git",
        &CoreSettings::git
    );
};

struct AppearanceSettings {
    QString translationTableFont;
    QString style;
    Qt::ColorScheme theme = Qt::ColorScheme::Unknown;
    QLocale::Language language = QLocale().language();
    u8 translationTableFontSize = 0;
    bool displayPercents = false;
};

template <>
struct glz::meta<AppearanceSettings> {
    static constexpr auto value = glz::object(
        "translationTableFont",
        &AppearanceSettings::translationTableFont,
        "style",
        &AppearanceSettings::style,
        "theme",
        &AppearanceSettings::theme,
        "language",
        &AppearanceSettings::language,
        "translationTableFontSize",
        &AppearanceSettings::translationTableFontSize,
        "displayPercents",
        &AppearanceSettings::displayPercents
    );
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

template <>
struct glz::meta<EndpointSettings> {
    static constexpr auto value = glz::object(
        "id",
        &EndpointSettings::id,
        "name",
        &EndpointSettings::name,
        "apiKey",
        &EndpointSettings::apiKey,
        "yandexFolderID",
        &EndpointSettings::yandexFolderID,
        "baseUrl",
        &EndpointSettings::baseUrl,
        "model",
        &EndpointSettings::model,
        "systemPrompt",
        &EndpointSettings::systemPrompt,
        "singleTranslateSystemPrompt",
        &EndpointSettings::singleTranslateSystemPrompt,
        "temperature",
        &EndpointSettings::temperature,
        "frequencyPenalty",
        &EndpointSettings::frequencyPenalty,
        "precensePenalty",
        &EndpointSettings::precensePenalty,
        "topP",
        &EndpointSettings::topP,
        "tokenLimit",
        &EndpointSettings::tokenLimit,
        "outputTokenLimit",
        &EndpointSettings::outputTokenLimit,
        "thinkingBudget",
        &EndpointSettings::thinkingBudget,
        "reasoningEffort",
        &EndpointSettings::reasoningEffort,
        "useGlossary",
        &EndpointSettings::useGlossary,
        "thinking",
        &EndpointSettings::thinking,
        "singleTranslation",
        &EndpointSettings::singleTranslation,
        "type",
        &EndpointSettings::type
    );
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

template <>
struct glz::meta<LanguageToolSettings> {
    static constexpr auto value = glz::object(
        "baseURL",
        &LanguageToolSettings::baseURL,
        "apiKey",
        &LanguageToolSettings::apiKey,
        "username",
        &LanguageToolSettings::username,
        "motherTongue",
        &LanguageToolSettings::motherTongue,
        "preferredVariants",
        &LanguageToolSettings::preferredVariants,
        "dicts",
        &LanguageToolSettings::dicts,
        "enabledRules",
        &LanguageToolSettings::enabledRules,
        "disabledRules",
        &LanguageToolSettings::disabledRules,
        "enabledCategories",
        &LanguageToolSettings::enabledCategories,
        "disabledCategories",
        &LanguageToolSettings::disabledCategories,
        "picky",
        &LanguageToolSettings::picky,
        "enabledOnly",
        &LanguageToolSettings::enabledOnly
    );
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

template <>
struct glz::meta<Replacement> {
    using T = Replacement;
    static constexpr auto value =
        glz::object("source", &T::source, "replacement", &T::replacement, "enabled", &T::enabled);
};

struct SequenceSettings {
    QRgb customColor = 0;
    bool useCustomColor = false;
    bool disabled = false;
};

template <>
struct glz::meta<SequenceSettings> {
    using T = SequenceSettings;
    static constexpr auto value =
        glz::object("customColor", &T::customColor, "useCustomColor", &T::useCustomColor, "disabled", &T::disabled);
};

struct CustomLint {
    QString sequence;
    QString tooltip;
    QRgb customColor = 0;
    bool caseInsensitive = false;
    bool useCustomColor = false;
    bool disabled = false;
};

template <>
struct glz::meta<CustomLint> {
    using T = CustomLint;
    static constexpr auto value = glz::object(
        "sequence",
        &T::sequence,
        "tooltip",
        &T::tooltip,
        "customColor",
        &T::customColor,
        "caseInsensitive",
        &T::caseInsensitive,
        "useCustomColor",
        &T::useCustomColor,
        "disabled",
        &T::disabled
    );
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

    LintFlags lintFlags =
        scast<LintFlags>(ranges::fold_left(magic_enum::enum_values<LintFlags>(), LintFlags{}, std::bit_or<>{}));

    bool displayWordsAndCharacters = false;
};

template <>
struct glz::meta<TranslationSettings> {
    static constexpr auto value = glz::object(
        "languageTool",
        &TranslationSettings::languageTool,
        "textCodes",
        &TranslationSettings::textCodes,
        "notetags",
        &TranslationSettings::notetags,
        "pluginCommands",
        &TranslationSettings::pluginCommands,
        "commentTags",
        &TranslationSettings::commentTags,
        "customLints",
        &TranslationSettings::customLints,
        "endpoints",
        &TranslationSettings::endpoints,
        "lintFlags",
        &TranslationSettings::lintFlags,
        "replacements",
        &TranslationSettings::replacements,
        "whitespaceCharacters",
        &TranslationSettings::whitespaceCharacters,
        "displayWordsAndCharacters",
        &TranslationSettings::displayWordsAndCharacters
    );
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

template <>
struct glz::meta<ControlSettings> {
    static constexpr auto value = glz::object(
        "searchPanel",
        &ControlSettings::searchPanel,
        "tabPanel",
        &ControlSettings::tabPanel,
        "goToRow",
        &ControlSettings::goToRow,
        "batchMenu",
        &ControlSettings::batchMenu,
        "bookmarkMenu",
        &ControlSettings::bookmarkMenu,
        "lintMenu",
        &ControlSettings::lintMenu,
        "glossaryMenu",
        &ControlSettings::glossaryMenu,
        "translationsMenu",
        &ControlSettings::translationsMenu
    );
};

struct Settings {
    TranslationSettings translation;
    ControlSettings controls;
    CoreSettings core;
    AppearanceSettings appearance;
};

template <>
struct glz::meta<Settings> {
    static constexpr auto value = glz::object(
        "core",
        &Settings::core,
        "appearance",
        &Settings::appearance,
        "controls",
        &Settings::controls,
        "translation",
        &Settings::translation
    );
};