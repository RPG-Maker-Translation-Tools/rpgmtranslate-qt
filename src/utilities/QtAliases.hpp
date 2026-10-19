#pragma once

// Qt side of the aliases. Included by a Qt project's own Aliases.hpp, after the Qt-free base (cpp-headers/Aliases.hpp).
//
// Requires: "Aliases.hpp" (cpp-headers), Qt Core.

#include "Aliases.hpp"

#include <QList>
#include <QString>
#include <qtversionchecks.h>

using namespace Qt::Literals::StringLiterals;

using QUtf8SV = QUtf8StringView;
using QL1SV = QLatin1StringView;
using QSVList = QList<QStringView>;

[[nodiscard]] constexpr auto operator""_qsv(const char16_t* const ptr, const usize len) -> QStringView {
    return { ptr, scast<isize>(len) };
}

// Qt < 6.9 (Debian stable) `arg` takes no string view, so a narrow view becomes a QString first. On 6.9+ this is the
// identity. Never use it for an owning QString; use `.toString()` instead.
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
