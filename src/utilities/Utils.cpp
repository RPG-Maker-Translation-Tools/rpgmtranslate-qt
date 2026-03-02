#include "Utils.hpp"

#include "Constants.hpp"

auto lineParts(
    const QStringView line,
    const u32 lineNumber,
    const QAnyStringView filename
) -> QSVList {
    auto split = line.split(SEPARATOR);

    if (split.size() < 2) {
        qWarning() << QObject::tr("Couldn't split text at line %1 in file %2")
                          .arg(lineNumber)
                          .arg(filename);
        return {};
    }

    return split;
};

auto getTranslation(const QSVList& parts) -> Translation {
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
    const QLatin1StringView needle,
    const QStringView replacement
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
    const QStringView needle,
    const QLatin1StringView replacement
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
    const QLatin1StringView needle,
    const QLatin1StringView replacement
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

    for (isize i = 0; i < size; i++) {
        const QChar chr = data[i];
        result.push_back(chr == needle ? replacement : chr);
    }

    return result;
}

auto joinQSVList(const QSVList& list, const QLatin1StringView separator)
    -> QString {
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

    for (isize i = 0; i < separator.size(); ++i) {
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

    for (const u8 idx : range<u8>(0, separator.size())) {
        result.removeLast();
    }

    return result;
}
