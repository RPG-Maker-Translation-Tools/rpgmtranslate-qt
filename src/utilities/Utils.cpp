#include "Utils.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"

auto lineParts(
    const QStringView line,
    const u32 lineNumber,
    const QAnyStringView filename
) -> QSVList {
    auto split = line.split(SEPARATOR);

    if (split.size() < 2) {
        qWarning() << QObject::tr("Couldn't split text at line %1 in file %2")
                          .arg(
                              QL1SV(itos(lineNumber).data())
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                                  .toString()
#endif
                                  ,
                              filename
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                                  .toString()
#endif
                          );
        return {};
    }

    return split;
};

auto getTranslation(const QSVList& parts) -> struct Translation {
    for (const auto [idx, part] :
         views::enumerate(views::reverse(views::drop(parts, 1)))) {
        if (!part.isEmpty()) {
            return { .translation = part, .index = u8(idx + 1) };
        }
    }

    return { .translation = QStringView(), .index = 0 };

}

auto getSource(const QSVList& parts) -> QStringView {
    return parts[0];
}

auto getSource(const QStringView line) -> QStringView {
    return line.sliced(0, line.indexOf(SEPARATORL1));
}

auto getTranslations(const QSVList& parts) -> QSVList {
    return parts.sliced(1, parts.size() - 1);
}

auto qsvReplace(
    const QStringView input,
    const QL1SV needle,
    const QStringView replacement
) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize &&
            QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            ++i;
        }
    }

    return result;
}

auto qsvReplace(
    const QStringView input,
    const QStringView needle,
    const QL1SV replacement
) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize &&
            QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            ++i;
        }
    }

    return result;
}

auto qsvReplace(
    const QStringView input,
    const QL1SV needle,
    const QL1SV replacement
) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize &&
            QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            ++i;
        }
    }

    return result;
}

auto qsvReplace(
    const QStringView input,
    const QStringView needle,
    const QStringView replacement
) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize &&
            QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            i++;
        }
    }

    return result;
}

auto qsvReplace(
    const QStringView input,
    const QChar needle,
    const QChar replacement
) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize size = input.size();

    for (const auto idx : range(0, size)) {
        const QChar chr = data[idx];
        result.push_back(chr == needle ? replacement : chr);
    }

    return result;
}

auto joinQSVList(const QSVList& list, const QL1SV separator) -> QString {
    u32 size = 0;
    for (const QStringView view : list) {
        size += view.size();
    }

    size += separator.size() * list.size();

    QString result;
    result.reserve(size);

    for (const QStringView view : list) {
        result.append(view);
        result.append(separator);
    }

    for (const auto idx : range(0, separator.size())) {
        result.removeLast();
    }

    return result;
}

auto joinQSVList(const QSVList& list, const QChar separator) -> QString {
    u32 size = 0;

    for (const QStringView view : list) {
        size += view.size();
    }

    size += list.size();

    QString result;
    result.reserve(size);

    for (const QStringView view : list) {
        result.append(view);
        result.append(separator);
    }

    result.removeLast();
    return result;
}

auto joinQSVList(const QSVList& list, const QStringView separator) -> QString {
    u32 size = 0;

    for (const QStringView view : list) {
        size += view.size();
    }

    size += separator.size() * list.size();

    QString result;
    result.reserve(size);
    size = 0;

    for (const QStringView view : list) {
        result.append(view);
        result.append(separator);
    }

    for (const auto idx : range(0, separator.size())) {
        result.removeLast();
    }

    return result;
}

auto intLen(const u32 num) -> u8 {
    constexpr array<u32, 10> powers = { 1,         10,        100,     1000,
                                        10000,     100000,    1000000, 10000000,
                                        100000000, 1000000000 };

    const u8 len =
        ((CHAR_BIT * sizeof(u32)) - std::countl_zero(num)) * 1233 >> 12;
    return len + (num >= powers[len] ? 1 : 0);
}

auto intLen(const i32 num) -> u8 {
    const u32 val = num < 0 ? -u32(num) : u32(num);
    return intLen(val) + (num < 0 ? 1 : 0);
}

auto lastPathComponent(const QString& path) -> QStringView {
    for (const auto idx : range<-1>(path.size() - 1, -1)) {
        const QChar chr = path[idx];

        if (chr == u'/' || chr == u'\\') {
            return QStringView(path).mid(idx + 1);
        }
    }

    return {};
}
