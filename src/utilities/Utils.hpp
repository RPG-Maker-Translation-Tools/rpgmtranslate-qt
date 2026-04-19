#pragma once

#include "Aliases.hpp"
#include "rpgmtranslate.h"

#include <QStringList>
#include <QVariantMap>

struct Translation {
    QStringView translation;
    u8 index;
};

[[nodiscard]] auto
lineParts(QStringView line, u32 lineNumber, QAnyStringView filename) -> QSVList;
[[nodiscard]] auto getTranslation(const QSVList& parts) -> struct Translation;
[[nodiscard]] auto getSource(const QSVList& parts) -> QStringView;
[[nodiscard]] auto getSource(QStringView line) -> QStringView;
[[nodiscard]] auto getTranslations(const QSVList& parts) -> QSVList;

[[nodiscard]] auto
qsvReplace(QStringView input, QL1SV needle, QL1SV replacement) -> QString;
[[nodiscard]] auto
qsvReplace(QStringView input, QStringView needle, QL1SV replacement) -> QString;
[[nodiscard]] auto
qsvReplace(QStringView input, QL1SV needle, QStringView replacement) -> QString;
[[nodiscard]] auto
qsvReplace(QStringView input, QStringView needle, QStringView replacement)
    -> QString;
[[nodiscard]] auto
qsvReplace(QStringView input, QChar needle, QChar replacement) -> QString;

[[nodiscard]] auto joinQSVList(const QSVList& list, QL1SV separator) -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QStringView separator)
    -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QChar separator) -> QString;

[[nodiscard]] auto intLen(u32 num) -> u8;
[[nodiscard]] auto intLen(i32 num) -> u8;

[[nodiscard]] auto lastPathComponent(const QString& path) -> QStringView;

[[nodiscard]] inline auto strtoffi(const QByteArrayView utf8) -> FFIString {
    return { .ptr = utf8.data(), .len = u32(utf8.size()) };
}

[[nodiscard]] inline auto ffitostr(const FFIString str) -> QUtf8SV {
    return { str.ptr, isize(str.len) };
}