#include "Settings.hpp"

[[nodiscard]] auto Backup::toJSON() const -> QJsonObject {
    return { { u"enabled"_s, enabled },
             { u"period"_s, period },
             { u"max"_s, max } };
}

auto Backup::fromJSON(const QJsonObject& obj) -> Backup {
    Backup backup;
    backup.enabled = obj["enabled"_L1].toBool(backup.enabled);
    backup.period = obj["period"_L1].toInt(backup.period);
    backup.max = obj["max"_L1].toInt(backup.max);
    return backup;
}

[[nodiscard]] auto CoreSettings::toJSON() const -> QJsonObject {
    return { { u"projectPath"_s, projectPath },
             { u"firstLaunch"_s, firstLaunch },
             { u"backup"_s, backup.toJSON() },
             { u"checkForUpdates"_s, checkForUpdates },
             { u"recentProjects"_s,
               QJsonArray::fromStringList(recentProjects) } };
}

auto CoreSettings::fromJSON(const QJsonObject& obj) -> CoreSettings {
    CoreSettings settings;
    settings.projectPath = obj["projectPath"_L1].toString();
    settings.firstLaunch = obj["firstLaunch"_L1].toBool(settings.firstLaunch);
    settings.backup = Backup::fromJSON(obj["backup"_L1].toObject());
    settings.checkForUpdates =
        obj["updatesEnabled"_L1].toBool(settings.checkForUpdates);

    for (const auto& value : obj["recentProjects"_L1].toArray()) {
        settings.recentProjects.append(value.toString());
    }

    return settings;
}

[[nodiscard]] auto AppearanceSettings::toJSON() const -> QJsonObject {
    return { { u"translationTableFont"_s, translationTableFont },
             { u"translationTableFontSize"_s, translationTableFontSize },
             { u"style"_s, style },
             { u"theme"_s, u8(theme) },
             { u"language"_s, u8(language) },
             { u"displayPercents"_s, displayPercents },
             { u"displayTrailingWhitespace"_s, displayTrailingWhitespace },
             { u"displayWordsAndCharacters"_s, displayWordsAndCharacters } };
}

auto AppearanceSettings::fromJSON(const QJsonObject& obj)
    -> AppearanceSettings {
    AppearanceSettings settings;
    settings.translationTableFont = obj["translationTableFont"_L1].toString();
    settings.translationTableFontSize =
        obj["translationTableFontSize"_L1].toInt();
    settings.style = obj["style"_L1].toString();
    settings.theme =
        Qt::ColorScheme(obj["theme"_L1].toInt(u8(Qt::ColorScheme::Unknown)));
    settings.language =
        QLocale::Language(obj["language"_L1].toInt(u8(settings.language)));
    settings.displayPercents = obj["displayPercents"_L1].toBool(false);
    return settings;
}

[[nodiscard]] auto GoogleEndpointSettings::toJSON() const -> QJsonObject {
    return { { u"singleTranslation"_s, singleTranslation } };
}

auto GoogleEndpointSettings::fromJSON(const QJsonObject& obj)
    -> GoogleEndpointSettings {
    return { .singleTranslation = obj["singleTranslation"_L1].toBool() };
}

[[nodiscard]] auto YandexEndpointSettings::toJSON() const -> QJsonObject {
    return { { u"apiKey"_s, apiKey },
             { u"folderId"_s, folderId },
             { u"singleTranslation"_s, singleTranslation } };
}

auto YandexEndpointSettings::fromJSON(const QJsonObject& obj)
    -> YandexEndpointSettings {
    return { .apiKey = obj["apiKey"_L1].toString(),
             .folderId = obj["folderId"_L1].toString(),
             .singleTranslation = obj["singleTranslation"_L1].toBool() };
}

[[nodiscard]] auto DeepLEndpointSettings::toJSON() const -> QJsonObject {
    return { { u"apiKey"_s, apiKey },
             { u"useGlossary"_s, useGlossary },
             { u"singleTranslation"_s, singleTranslation } };
}

auto DeepLEndpointSettings::fromJSON(const QJsonObject& obj)
    -> DeepLEndpointSettings {
    return { .apiKey = obj["apiKey"_L1].toString(),
             .useGlossary = obj["useGlossary"_L1].toBool(),
             .singleTranslation = obj["singleTranslation"_L1].toBool() };
}

[[nodiscard]] auto LLMSettings::toJSON() const -> QJsonObject {
    return {
        { u"apiKey"_s, apiKey },
        { u"baseUrl"_s, baseUrl },
        { u"model"_s, model },
        { u"systemPrompt"_s, systemPrompt },
        { u"singleTranslateSystemPrompt"_s, singleTranslateSystemPrompt },

        { u"temperature"_s, temperature },
        { u"tokenLimit"_s, tokenLimit },
        { u"outputTokenLimit"_s, outputTokenLimit },

        { u"useGlossary"_s, useGlossary },
        { u"thinking"_s, thinking },

        { u"singleTranslation"_s, singleTranslation },
    };
}

auto LLMSettings::fromJSON(const QJsonObject& obj) -> LLMSettings {
    return { .apiKey = obj["apiKey"_L1].toString(),
             .baseUrl = obj["baseUrl"_L1].toString(),
             .model = obj["model"_L1].toString(),
             .systemPrompt = obj["systemPrompt"_L1].toString(),

             .temperature =
                 f32(obj["temperature"_L1].toDouble(DEFAULT_TEMPERATURE)),
             .tokenLimit = u16(obj["tokenLimit"_L1].toInt(DEFAULT_TOKEN_LIMIT)),

             .useGlossary = obj["useGlossary"_L1].toBool(),
             .thinking = obj["thinking"_L1].toBool(),

             .singleTranslation = obj["singleTranslation"_L1].toBool() };
}

[[nodiscard]] auto TranslationSettings::toJSON() const -> QJsonObject {
    return {
        { u"google"_s, google.toJSON() },
        { u"yandex"_s, yandex.toJSON() },
        { u"deepl"_s, deepl.toJSON() },
        { u"chatgpt"_s, chatgpt.toJSON() },
        { u"claude"_s, claude.toJSON() },
        { u"gemini"_s, gemini.toJSON() },
        { u"deepseek"_s, deepseek.toJSON() },
        { u"openaiCompatible"_s, openaiCompatible.toJSON() },
        { u"ollama"_s, ollama.toJSON() },
    };
}

auto TranslationSettings::fromJSON(const QJsonObject& obj)
    -> TranslationSettings {
    return {
        .google = GoogleEndpointSettings::fromJSON(obj["google"_L1].toObject()),
        .yandex = YandexEndpointSettings::fromJSON(obj["yandex"_L1].toObject()),
        .deepl = DeepLEndpointSettings::fromJSON(obj["deepl"_L1].toObject()),
        .chatgpt = LLMSettings::fromJSON(obj["chatgpt"_L1].toObject()),
        .claude = LLMSettings::fromJSON(obj["claude"_L1].toObject()),
        .gemini = LLMSettings::fromJSON(obj["gemini"_L1].toObject()),
        .deepseek = LLMSettings::fromJSON(obj["deepseek"_L1].toObject()),
        .openaiCompatible =
            LLMSettings::fromJSON(obj["openaiCompatible"_L1].toObject()),
        .ollama = LLMSettings::fromJSON(obj["ollama"_L1].toObject()),
    };
}

[[nodiscard]] auto ControlSettings::toJSON() const -> QJsonObject {
    return {
        { u"searchPanel"_s, searchPanel },
        { u"tabPanel"_s, tabPanel },
        { u"goToRow"_s, goToRow },
        { u"batchMenu"_s, batchMenu },
        { u"bookmarkMenu"_s, bookmarkMenu },
        { u"matchMenu"_s, matchMenu },
        { u"glossaryMenu"_s, glossaryMenu },
        { u"translationsMenu"_s, translationsMenu },
    };
}

auto ControlSettings::fromJSON(const QJsonObject& obj) -> ControlSettings {
    return { .searchPanel = obj["searchPanel"_L1].toString(),
             .tabPanel = obj["tabPanel"_L1].toString(),
             .goToRow = obj["goToRow"_L1].toString(),
             .batchMenu = obj["batchMenu"_L1].toString(),
             .bookmarkMenu = obj["bookmarkMenu"_L1].toString(),
             .matchMenu = obj["matchMenu"_L1].toString(),
             .glossaryMenu = obj["glossaryMenu"_L1].toString(),
             .translationsMenu = obj["translationsMenu"_L1].toString() };
}

[[nodiscard]] auto Settings::toJSON() const -> QJsonObject {
    return { { u"core"_s, core.toJSON() },
             { u"appearance"_s, appearance.toJSON() },
             { u"controls"_s, controls.toJSON() },
             { u"translation"_s, translation.toJSON() } };
}

auto Settings::fromJSON(const QJsonObject& obj) -> Settings {
    Settings settings;

    if (obj.contains("core"_L1)) {
        settings.core = CoreSettings::fromJSON(obj["core"_L1].toObject());
    }

    if (obj.contains("appearance"_L1)) {
        settings.appearance =
            AppearanceSettings::fromJSON(obj["appearance"_L1].toObject());
    }

    if (obj.contains("controls"_L1)) {
        settings.controls =
            ControlSettings::fromJSON(obj["controls"_L1].toObject());
    }

    if (obj.contains("translation"_L1)) {
        settings.translation =
            TranslationSettings::fromJSON(obj["translation"_L1].toObject());
    }

    return settings;
}
