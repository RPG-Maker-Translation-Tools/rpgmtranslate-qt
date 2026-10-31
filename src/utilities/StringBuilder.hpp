#pragma once

#include "Aliases.hpp"
#include "Numbers.hpp"

template <typename T>
concept StringViewConvertible = std::convertible_to<T, string_view> && !std::same_as<std::remove_cvref_t<T>, char>;

template <typename T>
concept Arithmetic = (std::integral<std::remove_cvref_t<T>> || std::floating_point<std::remove_cvref_t<T>>) &&
                     !std::same_as<std::remove_cvref_t<T>, bool> && !std::same_as<std::remove_cvref_t<T>, char>;

template <typename T>
concept ExprNode = requires(const T& node, char* out) {
    { node.size() } -> std::convertible_to<usize>;
    { node.write(out) } -> std::same_as<char*>;
};

// string / string_view / const char* / char arrays.
struct StrLeaf {
    string_view sv;

    [[nodiscard]] constexpr auto size() const noexcept -> usize { return sv.size(); }

    constexpr auto write(char* out) const noexcept -> char* {
        return std::char_traits<char>::copy(out, sv.data(), sv.size()) + sv.size();
    }
};

struct CharLeaf {
    char c;

    [[nodiscard]] static constexpr auto size() noexcept -> usize { return 1; }

    constexpr auto write(char* out) const noexcept -> char* {
        *out = c;
        return out + 1;
    }
};

template <Arithmetic T>
struct NumLeaf {
    Digits<> digits;

    explicit NumLeaf(const T val) noexcept :
        digits([&] -> Digits<> {
            if constexpr (std::floating_point<T>) {
                return ftos(val);
            } else {
                return itos(val);
            }
        }()) {}

    [[nodiscard]] auto size() const noexcept -> usize { return digits.size(); }

    auto write(char* out) const noexcept -> char* {
        return std::char_traits<char>::copy(out, digits.data(), digits.size()) + digits.size();
    }
};

template <ExprNode L, ExprNode R>
struct Cat {
    L lhs;
    R rhs;

    [[nodiscard]] constexpr auto size() const noexcept -> usize { return lhs.size() + rhs.size(); }

    constexpr auto write(char* out) const noexcept -> char* { return rhs.write(lhs.write(out)); }
};

template <typename T>
concept IsNode = ExprNode<std::remove_cvref_t<T>>;

template <typename T>
struct BuilderWrap;

template <typename T>
concept IsBuilder = requires { typename std::remove_cvref_t<T>::sb_builder_tag; };

template <typename T>
constexpr auto make_leaf(T&& val) {
    using D = std::remove_cvref_t<T>;
    if constexpr (IsBuilder<D>) {
        return std::forward<T>(val).node;  // unwrap existing chain
    } else if constexpr (IsNode<T>) {
        return std::forward<T>(val);
    } else if constexpr (std::same_as<D, char>) {
        return CharLeaf{ val };
    } else if constexpr (StringViewConvertible<D>) {
        return StrLeaf{ string_view(val) };
    } else if constexpr (Arithmetic<D>) {
        return NumLeaf<D>(val);
    } else {
        static_assert(!sizeof(T*), "sb: unsupported type for string builder");
    }
}

template <typename T>
concept LeafConvertible =
    StringViewConvertible<std::remove_cvref_t<T>> || std::same_as<std::remove_cvref_t<T>, char> || Arithmetic<T>;

template <ExprNode Node>
struct Builder {
    using sb_builder_tag = void;
    Node node;

    [[nodiscard]] auto size() const noexcept -> usize { return node.size(); }

    // Implicit on purpose: `string s = "a" % value;` converts the chain directly.
    // NOLINTNEXTLINE(misc-explicit-constructor)
    [[nodiscard]] operator string() const {
        string result;
        const usize size = node.size();
        result.resize_and_overwrite(size, [&](char* buf, usize) -> auto {
            node.write(buf);
            return size;
        });
        return result;
    }

    [[nodiscard]] auto str() const -> string { return static_cast<string>(*this); }
};

template <typename L, typename R>
    requires(
        (IsBuilder<L> || LeafConvertible<L>) && (IsBuilder<R> || LeafConvertible<R>) &&
        (IsBuilder<L> || IsBuilder<R> || !(Arithmetic<L> && Arithmetic<R>))
    )
[[nodiscard]] constexpr auto operator%(L&& lhs, R&& rhs) {
    auto lhsLeaf = make_leaf(std::forward<L>(lhs));
    auto rhsLeaf = make_leaf(std::forward<R>(rhs));
    using LN = decltype(lhsLeaf);
    using RN = decltype(rhsLeaf);
    return Builder<Cat<LN, RN>>{ Cat<LN, RN>{ std::move(lhsLeaf), std::move(rhsLeaf) } };
}

template <typename... Ts>
[[nodiscard]] auto cat(Ts&&... args) -> string {
    std::tuple leaves{ make_leaf(std::forward<Ts>(args))... };
    const usize total =
        std::apply([](const auto&... leaf) -> auto { return (leaf.size() + ... + usize{ 0 }); }, leaves);

    string result;
    result.resize_and_overwrite(total, [&](char* buf, usize) -> auto {
        char* ptr = buf;
        std::apply([&](const auto&... leaf) -> void { ((ptr = leaf.write(ptr)), ...); }, leaves);
        return total;
    });
    return result;
}

template <typename... Ts>
void append(string& dst, Ts&&... args) {
    std::tuple leaves{ make_leaf(std::forward<Ts>(args))... };
    const usize extra =
        std::apply([](const auto&... leaf) -> auto { return (leaf.size() + ... + usize{ 0 }); }, leaves);
    const usize old = dst.size();
    dst.resize_and_overwrite(old + extra, [&](char* buf, usize) -> auto {
        char* ptr = buf + old;
        std::apply([&](const auto&... leaf) -> auto { ((ptr = leaf.write(ptr)), ...); }, leaves);
        return old + extra;
    });
}
