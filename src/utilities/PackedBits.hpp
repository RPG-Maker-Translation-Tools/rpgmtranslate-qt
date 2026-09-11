#pragma once

#include "Aliases.hpp"

template <usize N>
struct fixed_string {
    char data[N];

    constexpr fixed_string(const char (&str)[N]) { std::copy_n(str, N, data); }
};

template <usize N, usize M>
constexpr auto operator==(const fixed_string<N>& lhs, const fixed_string<M>& rhs) noexcept -> bool {
    if constexpr (N != M) {
        return false;
    } else {
        for (usize i = 0; i < N; ++i) {
            if (lhs.data[i] != rhs.data[i]) {
                return false;
            }
        }
        return true;
    }
}

template <usize N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template <fixed_string Name, typename T, usize Bits>
struct Field {
    static constexpr auto name = Name;
    using type = T;
    static constexpr usize bits = Bits;

    static_assert(Bits >= 1 && Bits <= (sizeof(u64) * CHAR_BIT), "Field width must be in [1, 64]");
    static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "Field type must be integral, bool, or an enum");
};

constexpr auto mask_for(usize w) noexcept -> u64 {
    return w >= 64 ? ~u64{ 0 } : (u64{ 1 } << w) - 1;
}

template <usize Bits>
using scalar_storage_t = std::
    conditional_t<(Bits <= 8), u8, std::conditional_t<(Bits <= 16), u16, std::conditional_t<(Bits <= 32), u32, u64>>>;

template <usize TotalBits>
using storage_for_t =
    std::conditional_t<(TotalBits <= 64), scalar_storage_t<TotalBits>, array<u64, (TotalBits + 63) / 64>>;

template <usize N>
constexpr auto extract_bits_words(const array<u64, N>& words, usize off, usize w) noexcept -> u64 {
    const usize word_idx = off / 64;
    const usize bit_idx = off % 64;

    u64 lo = words[word_idx] >> bit_idx;
    if (bit_idx + w > 64) {
        lo |= words[word_idx + 1] << (64 - bit_idx);
    }
    return lo & mask_for(w);
}

template <usize N>
constexpr void insert_bits_words(array<u64, N>& words, usize off, usize w, u64 value) noexcept {
    const usize word_idx = off / 64;
    const usize bit_idx = off % 64;
    const u64 m = mask_for(w);
    value &= m;

    words[word_idx] = (words[word_idx] & ~(m << bit_idx)) | (value << bit_idx);

    if (bit_idx + w > 64) {
        const usize rem = bit_idx + w - 64;
        const u64 hi_mask = mask_for(rem);
        words[word_idx + 1] = (words[word_idx + 1] & ~hi_mask) | (value >> (64 - bit_idx));
    }
}

template <typename... Fields>
class PackedBits {
    static constexpr usize total_bits = (Fields::bits + ...);
    static constexpr bool is_scalar = total_bits <= 64;
    using storage_type = storage_for_t<total_bits>;

    storage_type storage{};

    template <fixed_string Name, typename First, typename... Rest>
    static consteval auto offset_impl() -> usize {
        if constexpr (First::name == Name) {
            return 0;
        } else if constexpr (sizeof...(Rest) > 0) {
            return First::bits + offset_impl<Name, Rest...>();
        } else {
            static_assert(First::name == Name, "PackedBits: no field with that name");
            return 0;
        }
    }

    template <fixed_string Name>
    static consteval auto offset_of() -> usize {
        return offset_impl<Name, Fields...>();
    }

    template <fixed_string Name, typename First, typename... Rest>
    static consteval auto find_field_impl() {
        if constexpr (First::name == Name) {
            return First{};
        } else if constexpr (sizeof...(Rest) > 0) {
            return find_field_impl<Name, Rest...>();
        } else {
            static_assert(First::name == Name, "PackedBits: no field with that name");
            return First{};
        }
    }

    template <fixed_string Name>
    using field_t = decltype(find_field_impl<Name, Fields...>());

   public:
    static constexpr usize bit_count = total_bits;
    static constexpr usize size_bytes = sizeof(storage_type);

    constexpr PackedBits() = default;

    constexpr explicit PackedBits(typename Fields::type... values) noexcept { (set<Fields::name>(values), ...); }

    template <typename T, bool = std::is_enum_v<T>>
    struct underlying_or_self {
        using type = T;
    };

    template <typename T>
    struct underlying_or_self<T, true> {
        using type = std::underlying_type_t<T>;
    };

    template <typename T>
    using underlying_or_self_t = typename underlying_or_self<T>::type;

    template <fixed_string Name>
    [[nodiscard]] constexpr auto get() const noexcept {
        using F = field_t<Name>;
        using T = typename F::type;
        constexpr usize off = offset_of<Name>();
        constexpr usize w = F::bits;

        u64 raw;
        if constexpr (is_scalar) {
            raw = (scast<u64>(storage) >> off) & mask_for(w);
        } else {
            raw = extract_bits_words(storage, off, w);
        }

        if constexpr (std::is_same_v<T, bool>) {
            return raw != 0;
        } else {
            using U = underlying_or_self_t<T>;
            if constexpr (std::is_signed_v<U> && w < 64) {
                if (raw & (u64{ 1 } << (w - 1))) {
                    raw |= (~u64{ 0 } << w);
                }
            }
            return scast<T>(scast<U>(raw));
        }
    }

    template <fixed_string Name>
    constexpr void set(typename field_t<Name>::type value) noexcept {
        using F = field_t<Name>;
        constexpr usize off = offset_of<Name>();
        constexpr usize w = F::bits;

        u64 raw;
        if constexpr (std::is_same_v<typename F::type, bool>) {
            raw = value ? 1 : 0;
        } else if constexpr (std::is_enum_v<typename F::type>) {
            raw = scast<u64>(scast<std::underlying_type_t<typename F::type>>(value)) & mask_for(w);
        } else {
            raw = scast<u64>(value) & mask_for(w);
        }

        if constexpr (is_scalar) {
            constexpr u64 field_mask = mask_for(w) << off;
            const u64 v = (scast<u64>(storage) & ~field_mask) | ((raw << off) & field_mask);
            storage = scast<storage_type>(v);
        } else {
            insert_bits_words(storage, off, w, raw);
        }
    }
};