#pragma once

#include "Aliases.hpp"

namespace packed_detail {
template <usize N>
struct fixed_string {
    array<char, N> data;

    // NOLINTNEXTLINE(misc-explicit-constructor, modernize-avoid-c-arrays)
    constexpr fixed_string(const char (&str)[N]) { std::copy_n(str, N, data.data()); }
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

constexpr auto mask_for(usize width) noexcept -> u64 {
    return width >= 64 ? ~u64{ 0 } : (u64{ 1 } << width) - 1;
}

template <usize Bits>
using scalar_storage_t = std::
    conditional_t<(Bits <= 8), u8, std::conditional_t<(Bits <= 16), u16, std::conditional_t<(Bits <= 32), u32, u64>>>;

template <usize TotalBits>
using storage_for_t =
    std::conditional_t<(TotalBits <= 64), scalar_storage_t<TotalBits>, array<u64, (TotalBits + 63) / 64>>;

template <usize N>
constexpr auto extract_bits_words(const array<u64, N>& words, usize off, usize width) noexcept -> u64 {
    const usize word_idx = off / 64;
    const usize bit_idx = off % 64;

    u64 low = words[word_idx] >> bit_idx;
    if (bit_idx + width > 64) {
        low |= words[word_idx + 1] << (64 - bit_idx);
    }
    return low & mask_for(width);
}

template <usize N>
constexpr void insert_bits_words(array<u64, N>& words, usize off, usize width, u64 value) noexcept {
    const usize word_idx = off / 64;
    const usize bit_idx = off % 64;
    const u64 mask = mask_for(width);
    value &= mask;

    words[word_idx] = (words[word_idx] & ~(mask << bit_idx)) | (value << bit_idx);

    if (bit_idx + width > 64) {
        const usize rem = bit_idx + width - 64;
        const u64 hi_mask = mask_for(rem);
        words[word_idx + 1] = (words[word_idx + 1] & ~hi_mask) | (value >> (64 - bit_idx));
    }
}

template <typename T, bool = std::is_enum_v<T>>
struct underlying_or_self {
    using type = T;
};

template <typename T>
struct underlying_or_self<T, true> {
    using type = std::underlying_type_t<T>;
};

template <typename T>
using underlying_or_self_t = underlying_or_self<T>::type;
}  // namespace packed_detail

template <packed_detail::fixed_string Name, typename T, usize Bits>
struct Field {
    static constexpr auto name = Name;
    using type = T;
    static constexpr usize bits = Bits;

    static_assert(Bits >= 1 && Bits <= (sizeof(u64) * CHAR_BIT), "Field width must be in [1, 64]");
    static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "Field type must be integral, bool, or an enum");
};

template <typename... Fields>
class PackedBits {
    static constexpr usize total_bits = (Fields::bits + ...);
    static constexpr bool is_scalar = total_bits <= 64;
    using storage_type = packed_detail::storage_for_t<total_bits>;

    storage_type storage{};

    template <packed_detail::fixed_string Name, typename First, typename... Rest>
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

    template <packed_detail::fixed_string Name>
    static consteval auto offset_of() -> usize {
        return offset_impl<Name, Fields...>();
    }

    template <packed_detail::fixed_string Name, typename First, typename... Rest>
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

    template <packed_detail::fixed_string Name>
    using field_t = decltype(find_field_impl<Name, Fields...>());

   public:
    static constexpr usize bit_count = total_bits;
    static constexpr usize size_bytes = sizeof(storage_type);

    constexpr PackedBits() = default;

    constexpr explicit PackedBits(Fields::type... values) noexcept { (set<Fields::name>(values), ...); }

    template <packed_detail::fixed_string Name>
    [[nodiscard]] constexpr auto get() const noexcept {
        using F = field_t<Name>;
        using T = F::type;
        constexpr usize off = offset_of<Name>();
        constexpr usize width = F::bits;

        u64 raw;
        if constexpr (is_scalar) {
            raw = (static_cast<u64>(storage) >> off) & packed_detail::mask_for(width);
        } else {
            raw = packed_detail::extract_bits_words(storage, off, width);
        }

        if constexpr (std::is_same_v<T, bool>) {
            return raw != 0;
        } else {
            using U = packed_detail::underlying_or_self_t<T>;
            if constexpr (std::is_signed_v<U> && width < 64) {
                if (raw & (u64{ 1 } << (width - 1))) {
                    raw |= (~u64{ 0 } << width);
                }
            }
            return static_cast<T>(static_cast<U>(raw));
        }
    }

    template <packed_detail::fixed_string Name>
    constexpr void set(field_t<Name>::type value) noexcept {
        using F = field_t<Name>;
        constexpr usize off = offset_of<Name>();
        constexpr usize width = F::bits;

        u64 raw;
        if constexpr (std::is_same_v<typename F::type, bool>) {
            raw = value ? 1 : 0;
        } else if constexpr (std::is_enum_v<typename F::type>) {
            raw = static_cast<u64>(static_cast<std::underlying_type_t<typename F::type>>(value)) &
                  packed_detail::mask_for(width);
        } else {
            raw = static_cast<u64>(value) & packed_detail::mask_for(width);
        }

        if constexpr (is_scalar) {
            constexpr u64 field_mask = packed_detail::mask_for(width) << off;
            const u64 merged = (static_cast<u64>(storage) & ~field_mask) | ((raw << off) & field_mask);
            storage = static_cast<storage_type>(merged);
        } else {
            packed_detail::insert_bits_words(storage, off, width, raw);
        }
    }
};
