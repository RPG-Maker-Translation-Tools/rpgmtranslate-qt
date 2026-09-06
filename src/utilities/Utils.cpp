#include "Utils.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Types.hpp"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

namespace {
[[nodiscard]] auto handleFileOpenError(QWidget* const parent, const QString& path, const QString& error)
    -> ControlFlow {
    qWarning().noquote() << u"Failed to save file %1: %2"_qsv.arg(path, error);

    auto messageBox = QMessageBox(parent);
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.setWindowTitle(QObject::tr("Warning"));
    messageBox.setText(QObject::tr("Saving file failed"));
    messageBox.setInformativeText(
        QObject::tr(
            "Unable to save file %1: %2. You may try to save the file to a custom location. It's strongly advised to you to better close the program and fix the underlying issue before continuing your work."
        )
            .arg(path, error)
    );

    const QPushButton* const continueBtn =
        messageBox.addButton(QObject::tr("Continue anyway"), QMessageBox::AcceptRole);
    QPushButton* const retryBtn = messageBox.addButton(QObject::tr("Retry"), QMessageBox::ResetRole);
    const QPushButton* const saveButton =
        messageBox.addButton(QObject::tr("Save to custom location"), QMessageBox::ActionRole);
    QPushButton* const abortBtn = messageBox.addButton(QObject::tr("Abort"), QMessageBox::RejectRole);

    messageBox.setDefaultButton(retryBtn);
    messageBox.setEscapeButton(abortBtn);

    messageBox.exec();
    const auto* const clicked = messageBox.clickedButton();

    if (clicked == continueBtn) {
        return ContinueAnyway();
    }

    if (clicked == retryBtn) {
        return Retry();
    }

    if (clicked == saveButton) {
        const QString dir = QFileDialog::getExistingDirectory(parent);

        return Continue(dir);
    }

    return Abort();
}
}  // namespace

auto openForWriting(QWidget* const parent, QString& path, const QStringView suffix)
    -> result<unique_ptr<QFile>, Abort> {
    while (true) {
        auto file = make_unique<QFile>(path);

        if (file->open(QFile::WriteOnly | QFile::Truncate)) {
            return file;
        }

        const auto outcome = handleFileOpenError(parent, path, file->errorString());

        switch (outcome.index()) {
            case 0:  // ContinueAnyway
                return nullptr;
            case 1: {
                const QString& dir = std::get<1>(outcome).s;
                path = dir + suffix;
                continue;
            }
            case 2:  // Abort
                return Err(Abort());
            case 3:  // Retry
                continue;
            default:
                std::unreachable();
        }
    }
}

namespace {
// Getting colors for \C[n] from img/system/Window.png
constexpr i32 OLDER_WINDOW_COLOR_OFFSET_X = 64;
constexpr i32 OLDER_WINDOW_COLOR_OFFSET_Y = 96;
constexpr i32 NEWER_WINDOW_COLOR_OFFSET_X = 96;
constexpr i32 NEWER_WINDOW_COLOR_OFFSET_Y = 144;
// Color square takes 12x12 space
constexpr i32 OLDER_WINDOW_COLOR_SQUARE_SIZE = 8;
constexpr i32 NEWER_WINDOW_COLOR_SQUARE_SIZE = 12;
// 8 colors per row
constexpr i32 WINDOW_COLOR_ROW_COUNT = 8;

// Getting icons for \I[n] from img/system/IconSet.rpgmvp
constexpr i32 ICONSET_OLDER_ICON_SIZE = 24;
constexpr i32 ICONSET_NEWER_ICON_SIZE = 32;
constexpr i32 ICONSET_ROW_ICON_COUNT = 16;

std::pair<QString, array<QRgb, WINDOW_COLOR_COUNT>> cachedWindowPath;
mutex cachedWindowPathMutex;
}  // namespace

auto lineParts(const QStringView line, const u32 lineNumber, const QAnyStringView filename) -> QSVList {
    auto split = line.split(SEPARATOR);

    if (split.size() < 2) {
        qWarning().noquote() << u"Couldn't split text at line %1 in file %2"_qsv.arg(
            itos(lineNumber).qsv(),
            svtostr(filename)
        );
        return {};
    }

    return split;
};

auto getTranslation(const QSVList& parts) -> struct Translation {
    for (const auto [idx, part] : views::enumerate(views::reverse(views::drop(parts, 1)))) {
        if (!part.isEmpty()) {
            return { .translation = part, .index = scast<u8>(idx + 1) };
        }
    }

    return { .translation = QStringView(), .index = 0 };

}

auto getSource(const QSVList& parts) -> QStringView {
    return parts[0];
}

auto getSource(const QStringView line) -> QStringView {
    return line.sliced(0, line.indexOf(SEPARATOR));
}

auto getTranslations(const QSVList& parts) -> QSVList {
    return parts.sliced(1, parts.size() - 1);
}

auto qsvReplace(const QStringView input, const QL1SV needle, const QStringView replacement) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize && QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            i++;
        }
    }

    return result;
}

auto qsvReplace(const QStringView input, const QStringView needle, const QL1SV replacement) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize && QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            i++;
        }
    }

    return result;
}

auto qsvReplace(const QStringView input, const QL1SV needle, const QL1SV replacement) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize && QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            i++;
        }
    }

    return result;
}

auto qsvReplace(const QStringView input, const QStringView needle, const QStringView replacement) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize inputSize = input.size();
    const isize needleSize = needle.size();

    for (isize i = 0; i < inputSize;) {
        if (i + needleSize <= inputSize && QStringView(data + i, needleSize) == needle) {
            result.append(replacement);
            i += needleSize;
        } else {
            result.append(data[i]);
            i++;
        }
    }

    return result;
}

auto qsvReplace(const QStringView input, const QChar needle, const QChar replacement) -> QString {
    QString result;
    result.reserve(input.size());

    const QChar* const data = input.data();
    const isize size = input.size();

    for (i32 idx = 0; idx < size; idx++) {
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

    for (i32 idx = 0; idx < separator.size(); idx++) {
        Q_UNUSED(idx);
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

    for (const QStringView view : list) {
        result.append(view);
        result.append(separator);
    }

    for (i32 idx = 0; idx < separator.size(); idx++) {
        Q_UNUSED(idx);
        result.removeLast();
    }

    return result;
}

auto intLen(const u32 num) -> i32 {
    static constexpr array<u32, 10> powers = {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
    };

    const u8 len = ((CHAR_BIT * sizeof(u32)) - std::countl_zero(num)) * 1233 >> 12;
    return len + (num >= powers[len] ? 1 : 0);
}

auto intLen(const i32 num) -> i32 {
    const u32 val = num < 0 ? -scast<u32>(num) : scast<u32>(num);
    return intLen(val) + (num < 0 ? 1 : 0);
}

auto lastPathComponent(const QString& path) -> QStringView {
    for (i32 idx = scast<i32>(path.size() - 1); idx >= 0; idx--) {
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

auto getWindowColors(const QString& projectPath, const EngineType engineType)
    -> result<array<QRgb, WINDOW_COLOR_COUNT>, Notice> {
    const QString windowImagePath = projectPath % u"/Graphics/System/Window.png";

    {
        const std::scoped_lock lock(cachedWindowPathMutex);

        if (windowImagePath == cachedWindowPath.first) {
            return cachedWindowPath.second;
        }
    }

    auto file = QFile(windowImagePath);

    if (!file.open(QFile::ReadOnly)) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to open file %1: %2"),
            Severity::Warning,
            Delivery::Inline,
            { windowImagePath, file.errorString() }
        ));
    }

    const QByteArray imageData = file.readAll();
    const QImage img = QImage::fromData(imageData);

    array<QRgb, WINDOW_COLOR_COUNT> colors;

    const i32 windowOffsetX =
        engineType == EngineType::MVMZ ? NEWER_WINDOW_COLOR_OFFSET_X : OLDER_WINDOW_COLOR_OFFSET_X;
    const i32 windowOffsetY =
        engineType == EngineType::MVMZ ? NEWER_WINDOW_COLOR_OFFSET_Y : OLDER_WINDOW_COLOR_OFFSET_Y;
    const i32 windowSquareSize =
        engineType == EngineType::MVMZ ? NEWER_WINDOW_COLOR_SQUARE_SIZE : OLDER_WINDOW_COLOR_SQUARE_SIZE;

    for (i32 row = 0; row < 4; row++) {
        for (i32 col = 0; col < 8; col++) {
            const i32 offsetX = windowOffsetX + (col * windowSquareSize);
            const i32 offsetY = windowOffsetY + (row * windowSquareSize);
            const u32 pixel = img.pixel(QPoint(offsetX, offsetY));
            colors[(row * WINDOW_COLOR_ROW_COUNT) + col] = pixel;
        }
    }

    const std::scoped_lock lock(cachedWindowPathMutex);
    cachedWindowPath = { windowImagePath, colors };
    return colors;
}

auto getIcon(const i32 iconIndex, const QString& projectPath, const EngineType engineType) -> result<QPixmap, Notice> {
    i32 iconSize;

    if (engineType == EngineType::MVMZ) {
        iconSize = ICONSET_NEWER_ICON_SIZE;
    } else {
        iconSize = ICONSET_OLDER_ICON_SIZE;
    }

    static constexpr array<QStringView, 3> ICONSET_EXTENSIONS = { u"png"_qsv, u"rpgmvp"_qsv, u"png_"_qsv };

    const QString basePath = projectPath % u"/Graphics/System/IconSet."_qsv;

    QString iconsetPath;
    for (const auto& ext : ICONSET_EXTENSIONS) {
        const QString candidate = basePath % ext;
        if (QFile::exists(candidate)) {
            iconsetPath = candidate;
            break;
        }
    }

    if (iconsetPath.isEmpty()) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to find icon set in %1"),
            Severity::Warning,
            Delivery::Inline,
            { basePath }
        ));
    }

    auto file = QFile(iconsetPath);

    if (!file.open(QFile::ReadOnly)) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to open file %1: %2"),
            Severity::Warning,
            Delivery::Inline,
            { iconsetPath, file.errorString() }
        ));
    }

    const QByteArray imageData = file.readAll();

    const QString extension = iconsetPath.sliced(iconsetPath.lastIndexOf(u'.') + 1).toLower();
    const bool encrypted = extension == u"rpgmvp"_qsv || extension == u"png_"_qsv;

    QPixmap pixmap;

    if (encrypted) {
        const QByteArray pathUtf8 = iconsetPath.toUtf8();
        ByteBuffer out{};

        const bool success =
            rpgm_decrypt_asset(FFIString{ .ptr = pathUtf8.data(), .len = scast<u32>(pathUtf8.size()) }, &out);

        if (!success) {
            return Err(Notice(
                NOTICE_IN("Utils", "Failed to decrypt icon set %1: %2"),
                Severity::Warning,
                Delivery::Inline,
                { iconsetPath, ffitostr(rpgm_error()) }
            ));
        }

        pixmap.loadFromData(out.ptr + HEADER_LENGTH, out.len - HEADER_LENGTH);
        rpgm_buffer_free(out);
    } else {
        pixmap.loadFromData(imageData);
    }

    i32 offset = iconSize * iconIndex;
    const i32 boundedOffset = offset % pixmap.width();
    i32 row = 0;

    while (offset >= pixmap.width()) {
        offset -= pixmap.width();
        row++;
    }

    const auto iconRect = QRect(boundedOffset, row, iconSize, iconSize);
    const auto pixmapRect = QRect(0, 0, pixmap.width(), pixmap.height());

    if (!pixmapRect.contains(iconRect)) {
        return Err(Notice(
            NOTICE_IN("Utils", "Icon index %1 out of bounds for icon set %2"),
            Severity::Warning,
            Delivery::Inline,
            { QString::number(iconIndex), iconsetPath }
        ));
    }

    return pixmap.copy(iconRect);
}