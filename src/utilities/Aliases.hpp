#pragma once

#include "enchantum_single_header.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals::string_view_literals;

using namespace enchantum::bitwise_operators;

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

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
using std::deque;
using std::error_code;
using std::expected;
using std::format;
using std::function;
using std::make_shared;
using std::make_unique;
using std::mutex;
using std::nullopt;
using std::optional;
using std::pair;
using std::println;
using std::shared_ptr;
using std::span;
using std::string;
using std::string_view;
using std::tuple;
using std::unique_ptr;
using std::variant;
using std::vector;
using std::wstring;
using std::wstring_view;

using std::clamp;
using std::max;
using std::min;

template <typename T, typename E>
using Result = std::expected<T, E>;
template <typename E>
using Err = std::unexpected<E>;

template <typename O, typename T>
[[nodiscard]] constexpr auto scast(T&& arg) -> O {
    return static_cast<O>(std::forward<T>(arg));
}

template <typename O, typename T>
[[nodiscard]] constexpr auto rcast(T&& arg) -> O {
    return reinterpret_cast<O>(std::forward<T>(arg));
}

template <typename O, typename T>
[[nodiscard]] constexpr auto ccast(T&& arg) -> O {
    return const_cast<O>(std::forward<T>(arg));
}

using FilenameArray = array<char, 16>;

#include "QtAliases.hpp"
