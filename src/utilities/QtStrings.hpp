#pragma once

#include "QtAliases.hpp"

[[nodiscard]] auto qsvReplace(QStringView input, QStringView needle, QStringView replacement) -> QString;
[[nodiscard]] auto qsvReplace(QStringView input, QChar needle, QChar replacement) -> QString;

[[nodiscard]] auto joinQSVList(const QSVList& list, QStringView separator) -> QString;
[[nodiscard]] auto joinQSVList(const QSVList& list, QChar separator) -> QString;

[[nodiscard]] auto lastPathComponent(const QString& path) -> QStringView;
auto lastPathComponent(QString&&) -> QStringView = delete;

[[nodiscard]] auto codePointAt(QStringView string, isize idx) -> u32;
