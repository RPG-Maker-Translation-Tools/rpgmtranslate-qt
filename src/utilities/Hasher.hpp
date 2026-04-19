#pragma once

#include "rpgmtranslate.h"

#include <unordered_map>
#include <unordered_set>

struct Hasher {
    template <typename T>
    constexpr auto operator()(const T& value) const -> u64 {
        if constexpr (std::is_same_v<T, QString>) {
            return gxhash(value.utf16(), value.size() * 2);
        } else if constexpr (std::is_same_v<T, string>) {
            return gxhash(value.data(), value.size());
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            return gxhash(&value, sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for GXHasher");
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

    [[nodiscard]] auto operator[](const K& key) const -> const V& {
        return hashmap<K, V>::find(key)->second;
    }
};

template <typename E>
class HashSet : public hashset<E> {
   public:
    using hashset<E>::hashset;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
inline QString operator+(QString lhs, QStringView rhs) {
    lhs.append(rhs);
    return lhs;
}

inline QString operator+(QStringView lhs, QString rhs) {
    rhs.prepend(lhs);
    return rhs;
}
#endif