#pragma once

#include "Aliases.hpp"

#include <QStringList>
#include <QVariantMap>

struct Translation {
    QStringView translation;
    u8 index;
};

auto lineParts(QStringView line, u32 lineNumber, QAnyStringView filename)
    -> QSVList;
auto getTranslation(const QSVList& parts) -> struct Translation;
auto getSource(const QSVList& parts) -> QStringView;
auto getSource(QStringView line) -> QStringView;
auto getTranslations(const QSVList& parts) -> QSVList;

auto qsvReplace(
    QStringView input,
    QLatin1StringView needle,
    QLatin1StringView replacement
) -> QString;
auto qsvReplace(
    QStringView input,
    QStringView needle,
    QLatin1StringView replacement
) -> QString;
auto qsvReplace(
    QStringView input,
    QLatin1StringView needle,
    QStringView replacement
) -> QString;
auto qsvReplace(QStringView input, QStringView needle, QStringView replacement)
    -> QString;
auto qsvReplace(QStringView input, QChar needle, QChar replacement) -> QString;

auto joinQSVList(const QSVList& list, QLatin1StringView separator) -> QString;
auto joinQSVList(const QSVList& list, QStringView separator) -> QString;
auto joinQSVList(const QSVList& list, QChar separator) -> QString;
