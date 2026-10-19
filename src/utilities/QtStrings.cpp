#include "QtStrings.hpp"

namespace {
template <typename Needle, typename Replacement>
[[nodiscard]] auto replaceAll(const QStringView input, const Needle& needle, const Replacement& replacement)
    -> QString {
    isize hit = input.indexOf(needle);

    if (hit == -1) {
        return input.toString();
    }

    QString result;
    result.reserve(input.size());

    isize last = 0;

    do {
        result.append(input.sliced(last, hit - last));
        result.append(replacement);
        last = hit + needle.size();
        hit = input.indexOf(needle, last);
    } while (hit != -1);

    result.append(input.sliced(last));

    return result;
}

[[nodiscard]] constexpr auto separatorSize(const QChar) -> isize {
    return 1;
}

[[nodiscard]] constexpr auto separatorSize(const QStringView separator) -> isize {
    return separator.size();
}

[[nodiscard]] constexpr auto separatorSize(const QL1SV separator) -> isize {
    return separator.size();
}

template <typename Separator>
[[nodiscard]] auto joinAll(const QSVList& list, const Separator separator) -> QString {
    if (list.isEmpty()) {
        return {};
    }

    isize size = separatorSize(separator) * (list.size() - 1);

    for (const QStringView view : list) {
        size += view.size();
    }

    QString result;
    result.reserve(size);

    result.append(list.first());

    for (isize idx = 1; idx < list.size(); idx++) {
        result.append(separator);
        result.append(list[idx]);
    }

    return result;
}
}  // namespace

auto qsvReplace(const QStringView input, const QL1SV needle, const QStringView replacement) -> QString {
    return replaceAll(input, needle, replacement);
}

auto qsvReplace(const QStringView input, const QStringView needle, const QL1SV replacement) -> QString {
    return replaceAll(input, needle, replacement);
}

auto qsvReplace(const QStringView input, const QL1SV needle, const QL1SV replacement) -> QString {
    return replaceAll(input, needle, replacement);
}

auto qsvReplace(const QStringView input, const QStringView needle, const QStringView replacement) -> QString {
    return replaceAll(input, needle, replacement);
}

auto qsvReplace(const QStringView input, const QChar needle, const QChar replacement) -> QString {
    QString result = input.toString();
    result.replace(needle, replacement);

    return result;
}

auto joinQSVList(const QSVList& list, const QL1SV separator) -> QString {
    return joinAll(list, separator);
}

auto joinQSVList(const QSVList& list, const QChar separator) -> QString {
    return joinAll(list, separator);
}

auto joinQSVList(const QSVList& list, const QStringView separator) -> QString {
    return joinAll(list, separator);
}

auto lastPathComponent(const QString& path) -> QStringView {
    for (isize idx = path.size() - 1; idx >= 0; idx--) {
        const QChar chr = path[idx];

        if (chr == u'/' || chr == u'\\') {
            return QStringView(path).mid(idx + 1);
        }
    }

    return path;
}

auto codePointAt(const QStringView string, const isize idx) -> u32 {
    const QChar chr = string.at(idx);

    if (chr.isHighSurrogate() && idx + 1 < string.size()) {
        const QChar low = string.at(idx + 1);

        if (low.isLowSurrogate()) {
            return QChar::surrogateToUcs4(chr, low);
        }
    }

    return chr.unicode();
}
