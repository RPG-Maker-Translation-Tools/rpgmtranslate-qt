#pragma once

#include "Aliases.hpp"
#include "fast_float.h"
#include "zmij.h"

#include <cmath>

template <typename C = char>
struct Digits {
    static constexpr usize CAPACITY = zmij::double_buffer_size;

    array<C, CAPACITY + 1> buffer;
    u8 length;

    [[nodiscard]] constexpr auto data() const -> const C* { return buffer.data(); }

    [[nodiscard]] constexpr auto size() const -> usize { return length; }

    [[nodiscard]] constexpr auto view() const -> std::basic_string_view<C> { return { buffer.data(), length }; }

    // Implicit on purpose: `%` and string_view parameters take a Digits directly.
    // NOLINTNEXTLINE(misc-explicit-constructor)
    constexpr operator std::basic_string_view<C>() const { return view(); }
};

template <typename C = char, std::integral T>
[[nodiscard]] inline auto itos(const T integer, const i32 pad = 0, const char padChar = ' ') -> Digits<C> {
    array<char, Digits<C>::CAPACITY> text;
    const char* const end = zmij::write(text.data(), text.size(), integer);
    const i32 length = static_cast<i32>(end - text.data());

    const bool signedZeroPad = padChar == '0' && length > 0 && text[0] == '-';
    const i32 padding = min(max(pad - length, 0), static_cast<i32>(Digits<C>::CAPACITY) - length);

    Digits<C> result;
    result.length = static_cast<u8>(length + padding);

    i32 out = 0;
    i32 source = 0;

    if (signedZeroPad) {
        result.buffer[out++] = '-';
        source++;
    }

    for ([[maybe_unused]] const i32 count : views::iota(0, padding)) {
        result.buffer[out++] = padChar;
    }

    while (source < length) {
        result.buffer[out++] = text[source++];
    }

    result.buffer[out] = C(0);

    return result;
}

template <typename C = char, std::unsigned_integral T>
[[nodiscard]] inline auto htos(const T value, const i32 pad = 0, const bool uppercase = false) -> Digits<C> {
    static constexpr string_view LOWER = "0123456789abcdef";
    static constexpr string_view UPPER = "0123456789ABCDEF";
    static constexpr i32 VALUE_BITS = static_cast<i32>(sizeof(T) * CHAR_BIT);

    const string_view digits = uppercase ? UPPER : LOWER;

    i32 nibbles = 1;

    for (T rest = value >> 4; rest != 0; rest >>= 4) {
        nibbles++;
    }

    const i32 length = min(max(nibbles, pad), static_cast<i32>(Digits<C>::CAPACITY));

    Digits<C> result;
    result.length = static_cast<u8>(length);

    for (const i32 idx : views::iota(0, length)) {
        const i32 shift = (length - 1 - idx) * 4;

        // The padding digits are above the value's own bits; shifting by the full
        // width or more would be undefined.
        result.buffer[idx] = shift >= VALUE_BITS ? C('0') : C(digits[(value >> shift) & 0xF]);
    }

    result.buffer[length] = C(0);

    return result;
}

template <typename C = char, std::floating_point T>
[[nodiscard]] inline auto ftos(const T value, const i32 precision = -1) -> Digits<C> {
    array<char, zmij::buffer_sizes<T>::fixed> text;
    const char* end = nullptr;

    if (precision >= 0) {
        end = zmij::write_fixed(text.data(), text.size(), value, precision);
    }

    // A fixed form that doesn't fit (a huge value) falls back to the shortest
    // one.
    if (end == nullptr || end - text.data() > static_cast<isize>(Digits<C>::CAPACITY)) {
        end = zmij::write(text.data(), text.size(), value);
    }

    Digits<C> result;
    result.length = static_cast<u8>(end - text.data());

    for (const i32 idx : views::iota(0, static_cast<i32>(result.length))) {
        result.buffer[idx] = text[idx];
    }

    result.buffer[result.length] = C(0);

    return result;
}

template <typename T, typename C>
[[nodiscard]] constexpr auto stoa(const std::basic_string_view<C> text, const i32 base = 10) -> optional<T> {
    T result;
    const C* const end = text.data() + text.size();

    const auto parsed = [&] -> auto {
        if constexpr (std::floating_point<T>) {
            return fast_float::from_chars(text.data(), end, result);
        } else {
            return fast_float::from_chars(text.data(), end, result, base);
        }
    }();

    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return nullopt;
    }

    if constexpr (std::floating_point<T>) {
        if (!std::isfinite(result)) {
            return nullopt;
        }
    }

    return result;
}
