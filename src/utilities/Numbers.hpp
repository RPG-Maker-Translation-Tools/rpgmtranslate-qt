#pragma once

// Number <-> text, on the stack. The only sanctioned way to convert numbers: zmij (integers and floats to text),
// fast_float (text to numbers). No std::to_string/from_chars/snprintf, no Qt conversions.
//
// Requires: "Aliases.hpp", fast_float.h, and a zmij.h that has the integer `zmij::write` overloads (implemented in
// zmij-int.cc, which must be linked in next to zmij.cc), `zmij::write_fixed` and `zmij::buffer_sizes<T>`.

#include "Aliases.hpp"
#include "fast_float.h"
#include "zmij.h"

// A number as text, on the stack, followed by a terminator so `data()` is also a C string (FLTK, C APIs). `C` is the
// character type: `char` for UTF-8, `char16_t` for Qt.
template <typename C = char>
struct Digits {
    // Fits any 64-bit integer with its sign, and the shortest form of any float or double.
    static constexpr usize CAPACITY = 40;

    array<C, CAPACITY + 1> buffer;
    u8 length;

    [[nodiscard]] constexpr auto data() const -> const C* { return buffer.data(); }

    [[nodiscard]] constexpr auto size() const -> usize { return length; }

    [[nodiscard]] constexpr auto view() const -> std::basic_string_view<C> { return { buffer.data(), length }; }

    constexpr operator std::basic_string_view<C>() const { return view(); }
};

// zmij's integer writers store in wide blocks past the last digit; with a smaller buffer they format into a temporary
// and copy, which is slower.
static_assert(Digits<>::CAPACITY >= zmij::int64_buffer_size);

// Decimal text of `integer`, left-padded with `padChar` to `pad` characters (after the minus sign for zeros).
template <typename C = char, std::integral T>
[[nodiscard]] inline auto itos(const T integer, const i32 pad = 0, const char padChar = ' ') -> Digits<C> {
    array<char, Digits<C>::CAPACITY> text;
    const char* const end = zmij::write(text.data(), text.size(), integer);
    const i32 length = scast<i32>(end - text.data());

    const bool signedZeroPad = padChar == '0' && length > 0 && text[0] == '-';
    const i32 padding = min(max(pad - length, 0), scast<i32>(Digits<C>::CAPACITY) - length);

    Digits<C> result;
    result.length = scast<u8>(length + padding);

    i32 out = 0;
    i32 in = 0;

    if (signedZeroPad) {
        result.buffer[out++] = '-';
        in++;
    }

    for ([[maybe_unused]] const i32 count : views::iota(0, padding)) {
        result.buffer[out++] = padChar;
    }

    while (in < length) {
        result.buffer[out++] = text[in++];
    }

    result.buffer[out] = C(0);

    return result;
}

// Hexadecimal text of `value`, without a prefix: lowercase unless `uppercase`, left-padded with zeros to `pad`
// digits (capped at `Digits::CAPACITY`). `"U+" % htos(codePoint, 4, true)` is `U+00E9`.
template <typename C = char, std::unsigned_integral T>
[[nodiscard]] inline auto htos(const T value, const i32 pad = 0, const bool uppercase = false) -> Digits<C> {
    static constexpr string_view LOWER = "0123456789abcdef";
    static constexpr string_view UPPER = "0123456789ABCDEF";
    static constexpr i32 VALUE_BITS = scast<i32>(sizeof(T) * CHAR_BIT);

    const string_view digits = uppercase ? UPPER : LOWER;

    i32 nibbles = 1;

    for (T rest = value >> 4; rest != 0; rest >>= 4) {
        nibbles++;
    }

    const i32 length = min(max(nibbles, pad), scast<i32>(Digits<C>::CAPACITY));

    Digits<C> result;
    result.length = scast<u8>(length);

    for (const i32 idx : views::iota(0, length)) {
        const i32 shift = (length - 1 - idx) * 4;

        // The padding digits are above the value's own bits; shifting by the full width or more would be undefined.
        result.buffer[idx] = shift >= VALUE_BITS ? C('0') : C(digits[(value >> shift) & 0xF]);
    }

    result.buffer[length] = C(0);

    return result;
}

// Text of `value`: the shortest form that reads back as the same number when `precision` is negative, or exactly
// `precision` digits after the point, correctly rounded, otherwise (what `printf("%.*f")` prints).
template <typename C = char, std::floating_point T>
[[nodiscard]] inline auto ftos(const T value, const i32 precision = -1) -> Digits<C> {
    array<char, zmij::buffer_sizes<T>::fixed> text;
    const char* end = nullptr;

    if (precision >= 0) {
        end = zmij::write_fixed(text.data(), text.size(), value, precision);
    }

    // A fixed form that doesn't fit (a huge value) falls back to the shortest one.
    if (end == nullptr || end - text.data() > scast<isize>(Digits<C>::CAPACITY)) {
        end = zmij::write(text.data(), text.size(), value);
    }

    Digits<C> result;
    result.length = scast<u8>(end - text.data());

    for (const i32 idx : views::iota(0, scast<i32>(result.length))) {
        result.buffer[idx] = text[idx];
    }

    result.buffer[result.length] = C(0);

    return result;
}

// The number `text` holds in `base`, or nothing if it holds anything else (or is empty, out of range, or has
// trailing characters). Integers take no sign other than '-', and floats are in decimal.
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

    return result;
}
