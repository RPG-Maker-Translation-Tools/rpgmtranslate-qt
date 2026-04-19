#pragma once

#include "Aliases.hpp"

enum class BatchAction : u8 {
    None,
    Trim,
    Translate,
    Wrap,
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
