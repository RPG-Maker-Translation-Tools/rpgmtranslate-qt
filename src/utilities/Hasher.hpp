#pragma once

// HashMap/HashSet over gxhash. The only sanctioned hasher: no std::hash, no other hash function.
//
// Requires: "Aliases.hpp" and the project's Rust staticlib exporting `gxhash` (see README.md).

#include "Aliases.hpp"

#include <unordered_map>
#include <unordered_set>

extern "C" {
// gxhash64, seed 0 (rapidhash v3 without the Rust `gxhash` feature; the symbol name is kept). `ptr` may be null only when `len` is 0.
auto gxhash(const void* ptr, usize len) -> u64;
}

// Hashes the bytes of the contents, never the object itself (a view's pointer, a QString's d-pointer).
template <typename T>
concept ByteRange = ranges::contiguous_range<const T> && ranges::sized_range<const T> &&
                    std::is_trivially_copyable_v<ranges::range_value_t<const T>>;

struct Hasher {
    template <typename T>
    static constexpr auto operator()(const T& value) -> u64 {
        if constexpr (ByteRange<T>) {
            return gxhash(
                ranges::data(value),
                scast<usize>(ranges::size(value)) * sizeof(ranges::range_value_t<const T>)
            );
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            return gxhash(&value, sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for Hasher");
        }
    }
};

template <typename K, typename V>
using hashmap = std::unordered_map<K, V, Hasher>;
template <typename E>
using hashset = std::unordered_set<E, Hasher>;

template <typename K, typename V>
class HashMap : public hashmap<K, V> {
   public:
    using hashmap<K, V>::hashmap;

    // Lookup only: declaring this hides the base's inserting operator[], so a missing key can never be created
    // by accident. The caller guarantees `key` is present - find() is dereferenced unchecked.
    [[nodiscard]] auto operator[](const K& key) const -> const V& { return hashmap<K, V>::find(key)->second; }
};

template <typename E>
class HashSet : public hashset<E> {
   public:
    using hashset<E>::hashset;
};
