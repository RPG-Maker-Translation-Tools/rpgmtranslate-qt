#pragma once

#include "jeaiii_to_text.h"
#include "magic_enum.hpp"
#include "zmij.h"

#include <QString>
#include <atomic>
#include <bitset>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <qtversionchecks.h>
#include <ranges>
#include <string>
#include <vector>

using namespace Qt::Literals::StringLiterals;
using namespace std::literals::string_view_literals;

using namespace magic_enum::bitwise_operators;

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

// Previous fix for MSVC u128, we don't need it anymore
// #if defined(_MSC_VER) && !defined(__clang__)
// #include <__MSVC_Int128.hpp>
// using __uint128_t = std::_Unsigned128;
// #endif

using usize = std::size_t;
using isize = std::intptr_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;
using str = char*;
using cstr = const char*;
using wchar = wchar_t;
using wcstr = const wchar*;

using atomicBool = std::atomic_bool;
using atomicU8 = std::atomic_uint8_t;
using atomicI8 = std::atomic_int8_t;
using atomicU16 = std::atomic_uint16_t;
using atomicI16 = std::atomic_int16_t;
using atomicU32 = std::atomic_uint32_t;
using atomicI32 = std::atomic_int32_t;
using atomicU64 = std::atomic_uint64_t;
using atomicI64 = std::atomic_int64_t;

using fs::path;
using std::array;
using std::bitset;
using std::cerr;
using std::cout;
using std::expected;
using std::format;
using std::make_shared;
using std::make_unique;
using std::mutex;
using std::nullopt;
using std::optional;
using std::println;
using std::shared_ptr;
using std::span;
using std::string;
using std::string_view;
using std::tuple;
using std::unique_ptr;
using std::vector;
using std::wstring;
using std::wstring_view;

using std::clamp;
using std::max;
using std::min;

using QSVList = QList<QStringView>;
using QUtf8SV = QUtf8StringView;
using QL1SV = QLatin1StringView;

template <typename T, typename E>
using result = std::expected<T, E>;
template <typename E>
using Err = std::unexpected<E>;

template <typename O, typename T>
[[nodiscard]] constexpr auto as(T&& arg) -> O {
    return static_cast<O>(std::forward<T>(arg));
}

template <typename O, typename T>
[[nodiscard]] constexpr auto ras(T&& arg) -> O {
    return reinterpret_cast<O>(std::forward<T>(arg));
}

template <typename T, typename U>
using range_common_t = std::conditional_t<
    (std::is_signed_v<T> || std::is_signed_v<U>),
    std::common_type_t<std::make_signed_t<T>, std::make_signed_t<U>>,
    std::common_type_t<T, U>>;

template <auto Step = 1, typename T, typename U>
constexpr auto range(const T start, const U stop) {
    static_assert(Step != 0);

    using Common = range_common_t<T, U>;

    const auto s = static_cast<Common>(start);
    const auto e = static_cast<Common>(stop);

    if constexpr (Step > 0) {
        return views::iota(s, e) | views::stride(Step);
    } else {
        return views::iota(e + 1, s + 1) | views::reverse |
               views::stride(-Step);
    }
}

using FilenameArray = array<char, 16>;

// Integer to string. Currently handles only numbers up to 16 digits.
// Returned array must be manually sliced to the first null.
template <std::integral T>
inline auto itos(const T integer, const u8 pad = 0, const char padChar = ' ')
    -> array<char, 16> {
    array<char, 16> buf{};
    jeaiii::to_text_from_integer(buf.data(), integer);

    if (pad < buf.size() && pad != 0) {
        const u8 contentLen = strlen(buf.data());

        if (contentLen < pad) {
            const u8 padLen = pad - contentLen;

            memmove(buf.data() + padLen, buf.data(), contentLen);
            memset(buf.data(), padChar, padLen);
        }
    }

    return buf;
}

// Float to string. Currently handles only floats.
// Returned array must be manually sliced to the first null.
template <std::floating_point T>
inline auto ftos(const T flt, i8 precision = 0)
    -> array<char, zmij::float_buffer_size> {
    array<char, zmij::float_buffer_size> buf{};
    zmij::write(buf.data(), buf.size(), flt);

    if (precision >= 0) {
        bool afterPoint = false;
        u8 count = 0;

        for (const auto [idx, chr] : views::enumerate(buf)) {
            if (chr == '\0') {
                break;
            }

            if (chr == '.') {
                if (precision == 0) {
                    buf[idx] = '\0';
                    break;
                }

                afterPoint = true;
                continue;
            }

            if (afterPoint) {
                if (count >= precision) {
                    buf[idx] = '\0';
                    break;
                }

                count++;
            }
        }
    }

    return buf;
}