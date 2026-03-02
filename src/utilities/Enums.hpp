#pragma once

#include "Aliases.hpp"

#include <magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

#define ENUM_CONSTANT(EnumType, Member, NAME, SUFFIX) \
    constexpr auto NAME##_##SUFFIX = \
        static_cast<std::underlying_type_t<EnumType>>(EnumType::Member)

enum BaseFlags : u8 {
    Romanize = 1 << 0,
    Trim = 1 << 1,
    Ignore = 1 << 2,
    CreateIgnore = 1 << 3,
    DisableCustomProcessing = 1 << 4,
    SkipObsolete = 1 << 5,
};

enum class BatchAction : u8 {
    None,
    Trim,
    Translate,
    Wrap,
};

enum class DuplicateMode : u8 {
    Allow,
    Remove,
};

enum class EngineType : u8 {
    New,
    VXAce,
    VX,
    XP,
};

enum FileFlags : u16 {
    Map = 1 << 0,

    Actors = 1 << 1,

    Armors = 1 << 2,

    Classes = 1 << 3,

    CommonEvents = 1 << 4,

    Enemies = 1 << 5,

    Items = 1 << 6,

    Skills = 1 << 7,

    States = 1 << 8,

    Troops = 1 << 9,

    Weapons = 1 << 10,

    System = 1 << 11,

    Scripts = 1 << 12,

    Other = FileFlags::Actors | FileFlags::Armors | FileFlags::Classes |
            FileFlags::CommonEvents | FileFlags::Enemies | FileFlags::Items |
            FileFlags::Skills | FileFlags::States | FileFlags::Troops |
            FileFlags::Weapons,

    All = FileFlags::Map | FileFlags::Other | FileFlags::System |
          FileFlags::Scripts,
};

enum class MatchMode : u8 {
    Exact,
    Fuzzy,
    Both,
};

enum class ReadMode : u8 {
    Default,
    DefaultForce,
    AppendDefault,
    AppendForce,
};

enum class RPGMFileType : u8 {
    Invalid,
    Actors,
    Armors,
    Classes,
    Events,
    Enemies,
    Items,
    Map,
    Skills,
    States,
    System,
    Troops,
    Weapons,
    Scripts,
    Plugins,
};

enum SearchFlags : u8 {
    WholeWord = 1 << 0,
    CaseSensitive = 1 << 1,
    RegExp = 1 << 2,
    Comment = 1 << 3,
    Put = 1 << 4
};

enum SearchLocation : u8 {
    Source = 1 << 0,
    Translation = 1 << 1,
};

enum class Algorithm : i8 {
    None = -1,

    Arabic,
    Armenian,
    Basque,
    Catalan,
    Danish,
    Dutch,
    DutchPorter,
    English,
    Esperanto,
    Estonian,
    Finnish,
    French,
    German,
    Greek,
    Hindi,
    Hungarian,
    Indonesian,
    Irish,
    Italian,
    Lithuanian,
    Lovins,
    Nepali,
    Norwegian,
    Porter,
    Portuguese,
    Romanian,
    Russian,
    Serbian,
    Spanish,
    Swedish,
    Tamil,
    Turkish,
    Yiddish,

    Japanese,
    Chinese,
    Korean,

    Thai,
    Burmese,
    Lao,
    Khmer,
};

enum class TranslationEndpoint : u8 {
    Google,
    Yandex,
    DeepL,
    OpenAI,
    Anthropic,
    DeepSeek,
    Gemini,
    OpenAICompatible,
    Ollama
};

constexpr u8 TRANSLATION_ENDPOINT_COUNT =
    magic_enum::enum_count<TranslationEndpoint>();
