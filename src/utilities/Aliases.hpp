#pragma once

#include <QString>
#include <atomic>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <qtversionchecks.h>
#include <ranges>
#include <rapidhash.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Qt::Literals::StringLiterals;
using namespace std::literals::string_view_literals;

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

using usize = std::size_t;
using isize = std::intptr_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using u128 = __uint128_t;
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
using std::cerr;
using std::cout;
using std::expected;
using std::format;
using std::lock_guard;
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
using std::unique_ptr;
using std::vector;
using std::wstring;
using std::wstring_view;

using QSVList = QList<QStringView>;

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

template <typename T = usize>
constexpr auto range(const T from, const T dest) {
    return views::iota(from, dest);
}

template <ranges::input_range R, typename T>
constexpr auto find_index(const R& range, const T& value) -> isize {
    const auto item = ranges::find(range, value);

    if (item == ranges::end(range)) {
        return -1;
    }

    return ranges::distance(ranges::begin(range), item);
}

// Concatenation compatibility
#if QT_VERSION_MINOR >= 9
constexpr auto operator""_qssv(const char16_t* chr, const size_t size)
    -> QStringView {
    return { chr, isize(size) };
}
#else
constexpr auto operator""_qssv(const char16_t* chr, const size_t size)
    -> QString {
    return QString(
        QStringPrivate(nullptr, const_cast<char16_t*>(chr), qsizetype(size))
    );
}
#endif

struct RapidHasher {
    template <typename T>
    constexpr auto operator()(const T& value) const -> u64 {
        if constexpr (std::is_same_v<T, QString>) {
            return rapidhash(value.utf16(), value.size());
        } else if constexpr (std::is_same_v<T, string>) {
            return rapidhash(value.data(), value.size());
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            return rapidhash(&value, sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for RapidHasher");
        }
    }
};

template <typename K, typename V>
using rapidhashmap = std::unordered_map<K, V, RapidHasher>;
template <typename E>
using rapidhashset = std::unordered_set<E, RapidHasher>;

template <typename K, typename V>
class HashMap : public rapidhashmap<K, V> {
   public:
    using rapidhashmap<K, V>::rapidhashmap;

    [[nodiscard]] auto operator[](const K& key) const -> const V& {
        return rapidhashmap<K, V>::find(key)->second;
    }
};

template <typename E>
class HashSet : public rapidhashset<E> {
   public:
    using rapidhashset<E>::rapidhashset;
};
