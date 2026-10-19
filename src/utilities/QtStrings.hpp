#pragma once

// Requires: "QtAliases.hpp", Qt Core.

#include "QtAliases.hpp"

// Replaces every `needle` in `input`. Looks for hits with `indexOf` (vectorized) and copies whole runs between them,
// since most inputs contain no needle at all.
[[nodiscard]] auto qsvReplace(QStringView input, QL1SV needle, QL1SV replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QStringView needle, QL1SV replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QL1SV needle, QStringView replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QStringView needle, QStringView replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QChar needle, QChar replacement) -> QString;

// `list` joined with `separator`, sized and allocated once. Empty for an empty list.
[[nodiscard]] auto joinQSVList(const QSVList& list, QL1SV separator) -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QStringView separator) -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QChar separator) -> QString;

// What follows the last '/' or '\\' of `path`, or all of `path` if it has none.
[[nodiscard]] auto lastPathComponent(const QString& path) -> QStringView;

// The code point at UTF-16 index `idx` of `string`: a surrogate pair gives one value. A lone surrogate is returned as is.
[[nodiscard]] auto codePointAt(QStringView string, isize idx) -> u32;
