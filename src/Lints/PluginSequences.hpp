#pragma once

#include "Aliases.hpp"

consteval auto joinedPatternSize(const span<const QStringView> parts) -> usize {
    usize len = 0;

    for (const auto& part : parts) {
        len += scast<usize>(part.size());
    }

    len += parts.size() - 1;
    return len;
}

template <usize N>
consteval auto buildPattern(const span<const QStringView> parts) -> array<char16_t, N> {
    array<char16_t, N> result;
    usize pos = 0;

    for (usize i = 0; i < parts.size(); i++) {
        const auto part = parts[i];

        for (i32 idx = 0; idx < part.size(); idx++) {
            result[pos++] = part[idx].unicode();
        }

        if (i + 1 < parts.size()) {
            result[pos++] = u'|';
        }
    }

    return result;
}

#include "CommentTags.inl"
#include "Notetags.inl"
#include "PluginCommands.inl"
#include "TextCodes.inl"