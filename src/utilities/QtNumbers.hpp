#pragma once

#include "Numbers.hpp"
#include "QtAliases.hpp"

template <std::integral T>
[[nodiscard]] inline auto qitos(const T integer, const i32 pad = 0, const char padChar = ' ') -> Digits<char16_t> {
    return itos<char16_t>(integer, pad, padChar);
}

// See `htos`.
template <std::unsigned_integral T>
[[nodiscard]] inline auto qhtos(const T integer, const i32 pad = 0, const bool uppercase = false) -> Digits<char16_t> {
    return htos<char16_t>(integer, pad, uppercase);
}

template <std::floating_point T>
[[nodiscard]] inline auto qftos(const T value, const i32 precision = -1) -> Digits<char16_t> {
    return ftos<char16_t>(value, precision);
}

[[nodiscard]] inline auto qsv(const Digits<char16_t>& digits) -> QStringView {
    return { digits.data(), static_cast<isize>(digits.size()) };
}

[[nodiscard]] inline auto qstr(const Digits<char16_t>& digits) -> QString {
    return QString::fromUtf16(digits.data(), static_cast<isize>(digits.size()));
}

template <typename T>
[[nodiscard]] inline auto stoa(const QStringView text, const i32 base = 10) -> optional<T> {
    return stoa<T>(std::u16string_view(text.utf16(), static_cast<usize>(text.size())), base);
}

template <typename T>
[[nodiscard]] inline auto stoa(const QUtf8SV text, const i32 base = 10) -> optional<T> {
    return stoa<T>(string_view(text.data(), static_cast<usize>(text.size())), base);
}

template <typename T>
[[nodiscard]] inline auto stoa(const QL1SV text, const i32 base = 10) -> optional<T> {
    return stoa<T>(string_view(text.data(), static_cast<usize>(text.size())), base);
}
