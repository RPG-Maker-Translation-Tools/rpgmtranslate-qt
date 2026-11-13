#pragma once

#include "Aliases.hpp"

#include <QList>
#include <QString>
#include <qtversionchecks.h>

using namespace Qt::Literals::StringLiterals;

using QUtf8SV = QUtf8StringView;
using QL1SV = QLatin1StringView;
using QSVList = QList<QStringView>;

[[nodiscard]] constexpr auto operator""_qsv(const char16_t* const ptr, const usize len) -> QStringView {
    return { ptr, static_cast<isize>(len) };
}

// Targeting Debian stable, so Qt may be older than 6.9: `arg` takes a string
// view only from 6.9, before that a narrow view has to become a QString first.
// On 6.9+ this is the identity and returns the view unchanged. Never rely on it
// for an owning QString - use `.toString()` where ownership is needed.
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
[[nodiscard]] inline auto svtostr(const QAnyStringView view) -> QString {
    return view.toString();
}

[[nodiscard]] inline auto operator+(QString lhs, const QStringView rhs) -> QString {
    lhs.append(rhs);
    return lhs;
}
#else
template <typename T>
[[nodiscard]] constexpr auto svtostr(const T view) -> T {
    return view;
}
#endif
