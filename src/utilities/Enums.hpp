#pragma once

#include "Aliases.hpp"

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

enum LintType : u8 {
    TextCode,
    NoteTag,
    PluginCommand,
    CommentTag,
    Misc,
    Custom,
    LanguageToolCheck,
};

enum class SearchAction : u8 {
    Search,
    Replace,
    Put,
};

enum class BatchAction : u8 {
    Translate,
    Trim,
    Wrap,
    Latinize,
    Custom,
};

[[nodiscard]] constexpr auto batchActionId(const BatchAction action) -> QL1SV {
    switch (action) {
        case BatchAction::Translate:
            return "translate"_L1;
        case BatchAction::Trim:
            return "trim"_L1;
        case BatchAction::Wrap:
            return "wrap"_L1;
        case BatchAction::Latinize:
            return "latinize"_L1;
        case BatchAction::Custom:
            return "custom"_L1;
    }

    std::unreachable();
}
