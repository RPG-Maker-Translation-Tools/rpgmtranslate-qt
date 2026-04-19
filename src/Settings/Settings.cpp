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
             { u"backup"_s, backup.toJSON() },
             { u"checkForUpdates"_s, checkForAppUpdates },
             { u"recentProjects"_s,
               QJsonArray::fromStringList(recentProjects) } };
}

auto CoreSettings::fromJSON(const QJsonObject& obj) -> CoreSettings {
    CoreSettings settings;
    settings.projectPath = obj["projectPath"_L1].toString();
    settings.backup = Backup::fromJSON(obj["backup"_L1].toObject());
    settings.checkForAppUpdates =
        obj["updatesEnabled"_L1].toBool(settings.checkForAppUpdates);

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
             { u"displayPercents"_s, displayPercents } };
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

[[nodiscard]] auto EndpointSettings::toJSON() const -> QJsonObject {
    QJsonObject obj;

    obj["name"_L1] = name;
    obj["apiKey"_L1] = apiKey;
    obj["yandexFolderID"_L1] = yandexFolderID;
    obj["baseUrl"_L1] = baseUrl;
    obj["model"_L1] = model;
    obj["systemPrompt"_L1] = systemPrompt;
    obj["singleTranslateSystemPrompt"_L1] = singleTranslateSystemPrompt;

    if (temperature.has_value()) {
        obj["temperature"_L1] = temperature.value();
    }

    if (frequencyPenalty.has_value()) {
        obj["frequencyPenalty"_L1] = frequencyPenalty.value();
    }

    if (precensePenalty.has_value()) {
        obj["precensePenalty"_L1] = precensePenalty.value();
    }

    if (topP.has_value()) {
        obj["topP"_L1] = topP.value();
    }

    obj["tokenLimit"_L1] = tokenLimit;
    obj["outputTokenLimit"_L1] = outputTokenLimit;

    obj["thinkingBudget"_L1] = thinkingBudget;
    obj["reasoningEffort"_L1] = reasoningEffort;
    obj["useGlossary"_L1] = useGlossary;
    obj["thinking"_L1] = thinking;

    obj["singleTranslation"_L1] = singleTranslation;
    obj["type"_L1] = u8(type);

    return obj;
}

auto EndpointSettings::fromJSON(const QJsonObject& obj) -> EndpointSettings {
    EndpointSettings settings;

    settings.name = obj["name"_L1].toString();
    settings.apiKey = obj["apiKey"_L1].toString();
    settings.yandexFolderID = obj["yandexFolderID"_L1].toString();
    settings.baseUrl = obj["baseUrl"_L1].toString();
    settings.model = obj["model"_L1].toString();
    settings.systemPrompt = obj["systemPrompt"_L1].toString();
    settings.singleTranslateSystemPrompt =
        obj["singleTranslateSystemPrompt"_L1].toString();

    if (!obj["temperature"_L1].isUndefined()) {
        settings.temperature = f32(obj["temperature"_L1].toDouble());
    }

    if (!obj["frequencyPenalty"_L1].isUndefined()) {
        settings.frequencyPenalty = f32(obj["frequencyPenalty"_L1].toDouble());
    }

    if (!obj["precensePenalty"_L1].isUndefined()) {
        settings.precensePenalty = f32(obj["precensePenalty"_L1].toDouble());
    }

    if (!obj["topP"_L1].isUndefined()) {
        settings.topP = f32(obj["topP"_L1].toDouble());
    }

    settings.tokenLimit = u16(obj["tokenLimit"_L1].toInt());
    settings.outputTokenLimit = u16(obj["outputTokenLimit"_L1].toInt());

    settings.thinkingBudget = u16(obj["thinkingBudget"_L1].toInt());
    settings.reasoningEffort =
        ReasoningEffort(obj["reasoningEffort"_L1].toInt());

    settings.useGlossary = obj["useGlossary"_L1].toBool();
    settings.thinking = obj["thinking"_L1].toBool();

    settings.singleTranslation = obj["singleTranslation"_L1].toBool();
    settings.type = TranslationEndpoint(obj["type"_L1].toInt());

    return settings;
}

// TODO
[[nodiscard]] auto LanguageToolSettings::toJSON() const -> QJsonObject {
    return { { u"baseURL"_s, baseURL }, { u"enabled"_s, enabled } };
}

// TODO
auto LanguageToolSettings::fromJSON(const QJsonObject& obj)
    -> LanguageToolSettings {
    return { .baseURL = obj["baseURL"_L1].toString(),
             .enabled = obj["enabled"_L1].toBool() };
}

[[nodiscard]] auto TranslationSettings::toJSON() const -> QJsonObject {
    QJsonArray endpointsArray;

    for (const auto& endpoint : endpoints) {
        endpointsArray.append(endpoint.toJSON());
    }

    return { { u"languageTool"_s, languageTool.toJSON() },
             { u"endpoints"_s, endpointsArray },
             { u"miscLints"_s, u8(miscLints) } };
}

auto TranslationSettings::fromJSON(const QJsonObject& obj)
    -> TranslationSettings {
    const QJsonArray endpointsArray = obj["endpoints"_L1].toArray();

    vector<EndpointSettings> endpoints;
    endpoints.reserve(endpointsArray.size());

    for (const auto& endpoint : endpointsArray) {
        endpoints.push_back(EndpointSettings::fromJSON(endpoint.toObject()));
    }

    return { .languageTool = LanguageToolSettings::fromJSON(
                 obj["languageTool"_L1].toObject()
             ),
             .endpoints = std::move(endpoints),
             .miscLints = MiscLints(obj["miscLints"_L1].toInt()) };
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
