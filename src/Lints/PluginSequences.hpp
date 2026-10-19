#pragma once

#include "Aliases.hpp"

// `needle` found in `part` at `idx`.
consteval auto matchesAt(const QStringView part, const isize idx, const QStringView needle) -> bool {
    if (needle.isEmpty() || idx + needle.size() > part.size()) {
        return false;
    }

    // Raw pointers, Qt's debug build asserts on every `operator[]`, and that is enough steps
    // to exhaust clang's constexpr step limit on the larger pattern lists.
    const char16_t* const lhs = part.utf16() + idx;
    const char16_t* const rhs = needle.utf16();

    for (isize offset = 0; offset < needle.size(); offset++) {
        if (lhs[offset] != rhs[offset]) {
            return false;
        }
    }

    return true;
}

// The parts joined with `|`, with every `needle` in them written as `replacement` instead (nothing is replaced when
// `needle` is empty).
consteval auto joinedPatternSize(
    const span<const QStringView> parts,
    const QStringView needle = {},
    const QStringView replacement = {}
) -> usize {
    usize len = 0;

    for (const auto& part : parts) {
        if (needle.isEmpty()) {
            len += scast<usize>(part.size());
            continue;
        }

        for (isize idx = 0; idx < part.size();) {
            if (matchesAt(part, idx, needle)) {
                len += scast<usize>(replacement.size());
                idx += needle.size();
            } else {
                len++;
                idx++;
            }
        }
    }

    len += parts.size() - 1;
    return len;
}

template <usize N>
consteval auto
buildPattern(const span<const QStringView> parts, const QStringView needle = {}, const QStringView replacement = {})
    -> array<char16_t, N> {
    array<char16_t, N> result;
    usize pos = 0;

    const bool replacing = !needle.isEmpty();

    for (usize i = 0; i < parts.size(); i++) {
        const auto part = parts[i];
        const char16_t* const source = part.utf16();

        for (isize idx = 0; idx < part.size();) {
            if (replacing && matchesAt(part, idx, needle)) {
                for (isize at = 0; at < replacement.size(); at++) {
                    result[pos++] = replacement.utf16()[at];
                }

                idx += needle.size();
            } else {
                result[pos++] = source[idx++];
            }
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