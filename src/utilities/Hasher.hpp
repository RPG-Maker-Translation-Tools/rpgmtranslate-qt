#pragma once

#include "Aliases.hpp"

#include <unordered_map>
#include <unordered_set>

extern "C" {
auto gxhash(const void* ptr, usize len) -> u64;
}

template <typename T>
concept ByteRange = ranges::contiguous_range<const T> && ranges::sized_range<const T> &&
                    std::has_unique_object_representations_v<ranges::range_value_t<const T>>;

struct Hasher {
    template <typename T>
    static constexpr auto operator()(const T& value) -> u64 {
        if constexpr (ByteRange<T>) {
            return gxhash(
                ranges::data(value),
                static_cast<usize>(ranges::size(value)) * sizeof(ranges::range_value_t<const T>)
            );
        } else if constexpr (std::has_unique_object_representations_v<T>) {
            return gxhash(&value, sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for Hasher: padding or floating-point bytes");
        }
    }
};

template <typename K, typename V>
class HashMap : public std::unordered_map<K, V, Hasher> {
    using Base = std::unordered_map<K, V, Hasher>;

   public:
    using Base::Base;

    [[nodiscard]] auto operator[](this auto&& self, const K& key) -> decltype(auto) { return self.find(key)->second; }
};

template <typename E>
using HashSet = std::unordered_set<E, Hasher>;
