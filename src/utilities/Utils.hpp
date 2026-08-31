#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Notice.hpp"
#include "fast_float.h"
#include "jeaiii_to_text.h"
#include "rpgmtranslate_rs.h"
#include "zmij.h"

#include <QDebug>
#include <concepts>
#include <system_error>

struct Abort;

struct Translation {
    QStringView translation;
    u8 index;
};

[[nodiscard]] auto lineParts(QStringView line, u32 lineNumber, QAnyStringView filename) -> QSVList;
[[nodiscard]] auto getTranslation(const QSVList& parts) -> struct Translation;
[[nodiscard]] auto getSource(const QSVList& parts) -> QStringView;
[[nodiscard]] auto getSource(QStringView line) -> QStringView;
[[nodiscard]] auto getTranslations(const QSVList& parts) -> QSVList;

[[nodiscard]] auto qsvReplace(QStringView input, QL1SV needle, QL1SV replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QStringView needle, QL1SV replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QL1SV needle, QStringView replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QStringView needle, QStringView replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QChar needle, QChar replacement) -> QString;

[[nodiscard]] auto joinQSVList(const QSVList& list, QL1SV separator) -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QStringView separator) -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QChar separator) -> QString;

[[nodiscard]] auto intLen(u32 num) -> i32;
[[nodiscard]] auto intLen(i32 num) -> i32;

[[nodiscard]] auto lastPathComponent(const QString& path) -> QStringView;

[[nodiscard]] auto openForWriting(QWidget* parent, QString& path, QStringView suffix)
    -> result<unique_ptr<QFile>, Abort>;

[[nodiscard]] static constexpr auto strtoffi(const QByteArrayView utf8) -> FFIString {
    return { .ptr = utf8.data(), .len = scast<u32>(utf8.size()) };
}

[[nodiscard]] static constexpr auto ffitostr(const FFIString str) -> QUtf8SV {
    return { str.ptr, scast<isize>(str.len) };
}

[[nodiscard]] auto codePointAt(QStringView string, isize idx) -> u32;

static inline void copyFilenameArray(const char* const src, const usize size, char* const dst) {
    if (size >= sizeof(u64)) {
        u64 tmpA;
        u64 tmpB;
        memcpy(&tmpA, src, 8);
        memcpy(&tmpB, src + size - 8, 8);
        memcpy(dst, &tmpA, 8);
        memcpy(dst + size - 8, &tmpB, 8);
    } else if (size >= sizeof(u32)) {
        u32 tmpA;
        u32 tmpB;
        memcpy(&tmpA, src, 4);
        memcpy(&tmpB, src + size - 4, 4);
        memcpy(dst, &tmpA, 4);
        memcpy(dst + size - 4, &tmpB, 4);
    } else {
        std::unreachable();
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
static inline QString operator+(QString lhs, const QStringView rhs) {
    lhs.append(rhs);
    return lhs;
}
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
static inline auto svtostr(const QAnyStringView view) -> QString {
    return view.toString();
}
#else
template <typename T>
static constexpr auto svtostr(const T view) -> T {
    return view;
}
#endif

[[nodiscard]] static constexpr auto operator""_qsv(const char16_t* const ptr, const usize len) -> QStringView {
    return { ptr, scast<isize>(len) };
}

static constexpr i32 WINDOW_COLOR_COUNT = 32;

auto getWindowColors(const QString& projectPath, EngineType engineType)
    -> result<array<QRgb, WINDOW_COLOR_COUNT>, Notice>;

auto getIcon(i32 iconIndex, const QString& projectPath, EngineType engineType) -> result<QPixmap, Notice>;

[[nodiscard]] constexpr auto serdeFormatExtension(const SerdeFormat format) -> QStringView {
    switch (format) {
        case SerdeFormat::Csv:
            return u".csv";
        case SerdeFormat::Xlsx:
            return u".xlsx";
        case SerdeFormat::Xml:
            return u".xml";
        case SerdeFormat::Json:
            return u".json";
        case SerdeFormat::Yaml:
            return u".yaml";
    }

    std::unreachable();
}

struct Digits {
    array<char16_t, 11> buf{};
    u8 len;

    auto operator[](const isize idx) -> char16_t& { return buf[idx]; }

    auto operator[](const isize idx) const -> char16_t { return buf[idx]; }

    [[nodiscard]] operator QStringView() const { return { buf.data(), len }; }

    [[nodiscard]] auto qsv() const -> QStringView { return QStringView(*this); }
};

// Integer to string. Currently handles only numbers up to 11 digits.
// Returned array must be manually sliced to the first null.
template <std::integral T>
inline auto itos(const T integer, const u8 pad = 0, const char padChar = ' ') -> Digits {
    array<char, 16> buf{};
    const char* end = jeaiii::to_text_from_integer(buf.data(), integer);

    Digits result;
    result.len = end - buf.data();

    if (result.len < pad) {
        const u8 padLen = pad - result.len;

        memmove(buf.data() + padLen, buf.data(), result.len);
        memset(buf.data(), padChar, padLen);
        result.len = pad;
    }

    u8 idx = 0;

    for (const auto chr : span(buf.data(), result.len)) {
        result.buf[idx++] = chr;
    }

    return result;
}

// Float to string. Currently handles only floats.
// Returned array must be manually sliced to the first null.
template <std::floating_point T>
inline auto ftos(const T flt, i8 precision = 0) -> Digits {
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

    Digits result;
    u8 idx = 0;

    for (const auto chr : buf) {
        if (chr == '\0') {
            result.len = idx + 1;
            break;
        }

        result.buf[idx++] = chr;
    }

    return result;
}

template <typename T, typename C>
static constexpr auto stoa(const C* const first, const C* const last, const u8 base, bool* const valid) -> T {
    T result;

    const auto parsed = [&] -> auto {
        if constexpr (std::floating_point<T>) {
            return fast_float::from_chars(first, last, result);
        } else {
            return fast_float::from_chars(first, last, result, base);
        }
    }();

    if (valid != nullptr) {
        *valid = parsed.ec == std::errc{} && parsed.ptr == last;
    }

    return result;
}

template <typename T>
static constexpr auto stoa(const QStringView str, const u8 base = 10, bool* const valid = nullptr) -> T {
    return stoa<T>(str.utf16(), str.utf16() + str.size(), base, valid);
}

template <typename T>
static constexpr auto stoa(const QUtf8StringView str, const u8 base = 10, bool* const valid = nullptr) -> T {
    return stoa<T>(str.data(), str.data() + str.size(), base, valid);
}

template <typename T>
static constexpr auto stoa(const QLatin1StringView str, const u8 base = 10, bool* const valid = nullptr) -> T {
    return stoa<T>(str.data(), str.data() + str.size(), base, valid);
}

template <typename T, typename U>
using range_common_t = std::conditional_t<
    (std::is_signed_v<T> || std::is_signed_v<U>),
    std::common_type_t<std::make_signed_t<T>, std::make_signed_t<U>>,
    std::common_type_t<T, U>>;
