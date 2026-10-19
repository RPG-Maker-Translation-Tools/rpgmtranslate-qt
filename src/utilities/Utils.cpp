#include "Utils.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "ProjectSettings.hpp"
#include "Types.hpp"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
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
    -> Result<unique_ptr<QSaveFile>, Abort> {
    while (true) {
        auto file = make_unique<QSaveFile>(path);
        // A writable file in a read-only directory can't be replaced via rename; write it in place as before
        file->setDirectWriteFallback(true);

        if (file->open(QFile::WriteOnly)) {
            return file;
        }

        const auto outcome = handleFileOpenError(parent, path, file->errorString());

        switch (outcome.index()) {
            case 0:  // ContinueAnyway
                return nullptr;
            case 1: {
                const QString& dir = std::get<1>(outcome).s;

                // A cancelled picker gives an empty folder: keep the path, so the same error comes back
                if (dir.isEmpty()) {
                    continue;
                }

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

auto commitFile(QWidget* const parent, QSaveFile& file) -> bool {
    if (file.commit()) {
        return true;
    }

    present(
        parent,
        Notice(
            NOTICE_IN("Utils", "Failed to save file %1: %2"),
            Severity::Warning,
            Delivery::Modal,
            { file.fileName(), file.errorString() }
        )
    );

    return false;
}

void keepBackup(const QString& path) {
    if (QFileInfo(path).size() == 0) {
        return;
    }

    // The copy is made beside the backup and only then replaces it, so a failed copy leaves the old backup in place
    const QString backupPath = path % BACKUP_SUFFIX;
    const QString copyPath = backupPath % u".tmp"_qsv;

    QFile::remove(copyPath);

    if (!QFile::copy(path, copyPath)) {
        QFile::remove(copyPath);
        qWarning().noquote() << u"Failed to back up %1 to %2"_qsv.arg(path, backupPath);
        return;
    }

    QFile::remove(backupPath);

    if (!QFile::rename(copyPath, backupPath)) {
        qWarning().noquote() << u"Failed to back up %1 to %2"_qsv.arg(path, backupPath);
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
}  // namespace

auto lineParts(const QStringView line, const u32 lineNumber, const QAnyStringView filename) -> QSVList {
    auto split = line.split(SEPARATOR);

    if (split.size() < 2) {
        qWarning().noquote() << u"Couldn't split text at line %1 in file %2"_qsv.arg(
            qsv(qitos(lineNumber)),
            svtostr(filename)
        );
        return {};
    }

    return split;
};

auto getTranslation(const QSVList& parts) -> struct Translation {
    // The column is counted from the source column (0), as the other columns of a search are
    for (isize column = parts.size(); column-- > 1;) {
        if (!parts[column].isEmpty()) {
            return { parts[column], scast<u8>(column) };
        }
    }

    return { QStringView(), 0 };

}

auto getSource(const QSVList& parts) -> QStringView {
    return parts[0];
}

auto getTranslations(const QSVList& parts) -> QSVList {
    return parts.sliced(1, parts.size() - 1);
}

auto hasTranslation(const QSVList& parts) -> bool {
    // Whitespace alone is not a translation, as in `TranslationTableModel::setData`
    return ranges::any_of(views::drop(parts, 1), [](const QStringView part) -> bool {
        return !part.trimmed().isEmpty();
    });
}

auto isRowLine(const QStringView line) -> bool {
    return !line.startsWith(COMMENT_PREFIX) || line.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX);
}

namespace {
// Text colors of RPG Maker 2000/2003 live in its system graphic: 20 cells of 16x16 pixels, ten per row, from (0, 48).
// A cell is a vertical gradient, so the pixel in its middle stands for the color.
constexpr i32 RM2K_COLOR_COUNT = 20;
constexpr i32 RM2K_COLORS_PER_ROW = 10;
constexpr i32 RM2K_COLOR_CELL_SIZE = 16;
constexpr i32 RM2K_COLOR_ORIGIN_Y = 48;

// A failure is remembered for a while too: a lint asks for the colors for each color code it meets, and would otherwise
// load the missing image, and log the failure, for each of them.
constexpr i64 COLORS_FAILURE_RETRY_MS = 10000;

struct CachedColors {
    QString key;
    array<QRgb, WINDOW_COLOR_COUNT> colors;
    // Modification time of the image the colors were read from
    i64 modified = 0;
    optional<Notice> failure;
    i64 failedAt = 0;
};

struct CachedSystemName {
    QString path;
    QString name;
    i64 modified;
};

CachedColors cachedColors;
CachedSystemName cachedSystemName;
mutex cachedColorsMutex;

// Where the system graphics (Window, IconSet, the RM2K system graphic) of the project are.
[[nodiscard]] auto systemGraphicsDirectory(const ProjectSettings* const sets) -> QString {
    switch (sets->engineType) {
        case EngineType::MVMZ:
            return sets->projectPath %
                   (QFile::exists(sets->projectPath % u"/www"_qsv) ? u"/www/img/system"_qsv : u"/img/system"_qsv);
        case EngineType::RM2K:
            return sets->projectPath % u"/System"_qsv;
        default:
            return sets->projectPath % u"/Graphics/System"_qsv;
    }
}

// RPG Maker 2000/2003 picks its system graphic in the database. The database is big and asked for by every color lint,
// so the name is cached by the file's modification time.
[[nodiscard]] auto rm2kSystemGraphicName(const ProjectSettings* const sets) -> Result<QString, Notice> {
    const QString path = sets->rm2kDatabasePath();
    const qint64 modified = QFileInfo(path).lastModified().toMSecsSinceEpoch();

    {
        const std::scoped_lock lock(cachedColorsMutex);

        if (cachedSystemName.path == path && cachedSystemName.modified == modified) {
            return cachedSystemName.name;
        }
    }

    const QByteArray pathUtf8 = path.toUtf8();
    const QByteArray encodingUtf8 = sets->readEncoding.toUtf8();
    FFIString out;

    if (!rpgm_get_rm2k_system_graphic_name(strtoffi(pathUtf8), strtoffi(encodingUtf8), &out)) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to read the system graphic name from %1: %2"),
            Severity::Warning,
            Delivery::Inline,
            { path, ffitostr(rpgm_error()) }
        ));
    }

    QString name = ffitostr(out).toString();
    rpgm_string_free(out);

    const std::scoped_lock lock(cachedColorsMutex);
    cachedSystemName = { .path = path, .name = name, .modified = modified };

    return name;
}

// Finds `name` in the system graphics directory, with the first image format that exists there.
[[nodiscard]] auto findSystemImage(const QString& name, const ProjectSettings* const sets) -> Result<QString, Notice> {
    static constexpr array<QStringView, 4> EXTENSIONS = { u"png"_qsv, u"rpgmvp"_qsv, u"png_"_qsv, u"bmp"_qsv };

    const QString basePath = systemGraphicsDirectory(sets) % u'/' % name % u'.';

    for (const auto& ext : EXTENSIONS) {
        const QString candidate = basePath % ext;

        if (QFile::exists(candidate)) {
            return candidate;
        }
    }

    return Err(Notice(
        NOTICE_IN("Utils", "Failed to find the system graphic %1"),
        Severity::Warning,
        Delivery::Inline,
        { basePath }
    ));
}

// Decodes the image at `imagePath`, decrypting it first when the project keeps its images encrypted.
[[nodiscard]] auto decodeSystemImage(const QString& imagePath) -> Result<QImage, Notice> {
    auto file = QFile(imagePath);

    if (!file.open(QFile::ReadOnly)) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to open file %1: %2"),
            Severity::Warning,
            Delivery::Inline,
            { imagePath, file.errorString() }
        ));
    }

    const QByteArray imageData = file.readAll();

    const QString extension = imagePath.sliced(imagePath.lastIndexOf(u'.') + 1).toLower();
    const bool encrypted = extension == u"rpgmvp"_qsv || extension == u"png_"_qsv;

    QImage image;

    if (!encrypted) {
        image = QImage::fromData(imageData);
    } else {
        const QByteArray pathUtf8 = imagePath.toUtf8();
        ByteBuffer out{};

        if (!rpgm_decrypt_asset(FFIString{ .ptr = pathUtf8.data(), .len = scast<u32>(pathUtf8.size()) }, &out)) {
            return Err(Notice(
                NOTICE_IN("Utils", "Failed to decrypt the system graphic %1: %2"),
                Severity::Warning,
                Delivery::Inline,
                { imagePath, ffitostr(rpgm_error()) }
            ));
        }

        // A file shorter than the header it was encrypted with has no picture behind it
        if (out.len > HEADER_LENGTH) {
            image = QImage::fromData(out.ptr + HEADER_LENGTH, out.len - HEADER_LENGTH);
        }

        rpgm_buffer_free(out);
    }

    // Qt returns a null image, without an error, for bytes it can't decode. There is no sheet to cut icons from.
    if (image.isNull()) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to decode the system graphic %1"),
            Severity::Warning,
            Delivery::Inline,
            { imagePath }
        ));
    }

    return image;
}
}  // namespace

auto getWindowColors(const ProjectSettings* const sets) -> Result<array<QRgb, WINDOW_COLOR_COUNT>, Notice> {
    const bool rm2k = sets->engineType == EngineType::RM2K;

    QString name = u"Window"_s;

    if (rm2k) {
        auto systemName = rm2kSystemGraphicName(sets);

        if (!systemName) {
            return Err(std::move(systemName.error()));
        }

        name = std::move(systemName.value());
    }

    const QString key = systemGraphicsDirectory(sets) % u'/' % name;
    const auto imagePath = findSystemImage(name, sets);

    // The modification time is part of the cache, so an edited image is read again
    const i64 modified = imagePath ? QFileInfo(*imagePath).lastModified().toMSecsSinceEpoch() : 0;

    {
        const std::scoped_lock lock(cachedColorsMutex);

        if (key == cachedColors.key) {
            if (!cachedColors.failure) {
                if (imagePath && modified == cachedColors.modified) {
                    return cachedColors.colors;
                }
            } else if (QDateTime::currentMSecsSinceEpoch() - cachedColors.failedAt < COLORS_FAILURE_RETRY_MS) {
                return Err(*cachedColors.failure);
            }
        }
    }

    if (!imagePath) {
        const std::scoped_lock lock(cachedColorsMutex);
        cachedColors = { .key = key, .failure = imagePath.error(), .failedAt = QDateTime::currentMSecsSinceEpoch() };

        return Err(imagePath.error());
    }

    const auto image = decodeSystemImage(*imagePath);

    if (!image) {
        const std::scoped_lock lock(cachedColorsMutex);
        cachedColors = { .key = key, .failure = image.error(), .failedAt = QDateTime::currentMSecsSinceEpoch() };

        return Err(image.error());
    }

    array<QRgb, WINDOW_COLOR_COUNT> colors;

    if (rm2k) {
        for (i32 color = 0; color < RM2K_COLOR_COUNT; color++) {
            const i32 offsetX = ((color % RM2K_COLORS_PER_ROW) * RM2K_COLOR_CELL_SIZE) + (RM2K_COLOR_CELL_SIZE / 2);
            const i32 offsetY = RM2K_COLOR_ORIGIN_Y + ((color / RM2K_COLORS_PER_ROW) * RM2K_COLOR_CELL_SIZE) +
                                (RM2K_COLOR_CELL_SIZE / 2);

            colors[color] = image->pixel(offsetX, offsetY);
        }

        // Not colors of the game; transparent, so that nothing mistakes them for one.
        std::fill(colors.begin() + RM2K_COLOR_COUNT, colors.end(), QRgb(0));
    } else {
        const bool newer = sets->engineType == EngineType::MVMZ;
        const i32 windowOffsetX = newer ? NEWER_WINDOW_COLOR_OFFSET_X : OLDER_WINDOW_COLOR_OFFSET_X;
        const i32 windowOffsetY = newer ? NEWER_WINDOW_COLOR_OFFSET_Y : OLDER_WINDOW_COLOR_OFFSET_Y;
        const i32 windowSquareSize = newer ? NEWER_WINDOW_COLOR_SQUARE_SIZE : OLDER_WINDOW_COLOR_SQUARE_SIZE;

        for (i32 row = 0; row < 4; row++) {
            for (i32 col = 0; col < 8; col++) {
                const i32 offsetX = windowOffsetX + (col * windowSquareSize);
                const i32 offsetY = windowOffsetY + (row * windowSquareSize);
                colors[(row * WINDOW_COLOR_ROW_COUNT) + col] = image->pixel(offsetX, offsetY);
            }
        }
    }

    const std::scoped_lock lock(cachedColorsMutex);
    cachedColors = { .key = key, .colors = colors, .modified = modified };

    return colors;
}

auto getIcon(const i32 iconIndex, const ProjectSettings* const sets) -> Result<QPixmap, Notice> {
    const i32 iconSize = sets->engineType == EngineType::MVMZ ? ICONSET_NEWER_ICON_SIZE : ICONSET_OLDER_ICON_SIZE;

    const auto iconPath = findSystemImage(u"IconSet"_s, sets);

    if (!iconPath) {
        return Err(iconPath.error());
    }

    const auto iconset = decodeSystemImage(*iconPath);

    if (!iconset) {
        return Err(iconset.error());
    }

    // 64-bit, so that a big index can't overflow. Icons fill the sheet left to right, so the icon starts at pixel
    // (`column`, `row`).
    const i64 offset = i64{ iconSize } * iconIndex;
    const i64 sheetWidth = iconset->width();
    const i64 row = offset / sheetWidth;
    const i64 column = offset % sheetWidth;

    if (offset < 0 || row + iconSize > iconset->height() || column + iconSize > sheetWidth) {
        return Err(Notice(
            NOTICE_IN("Utils", "Icon index %1 out of bounds for the icon set"),
            Severity::Warning,
            Delivery::Inline,
            { QString(qsv(qitos(iconIndex))) }
        ));
    }

    const auto iconRect = QRect(scast<i32>(column), scast<i32>(row), iconSize, iconSize);

    return QPixmap::fromImage(iconset->copy(iconRect));
}

auto saveLocation(const ProjectSettings* const sets) -> SaveLocation {
    const QString& base = sets->projectPath;

    switch (sets->engineType) {
        case EngineType::MVMZ:
            return { .directory = base % (QFile::exists(base % u"/www"_qsv) ? u"/www/save"_qsv : u"/save"_qsv),
                     .filter = { u"*.rpgsave"_s, u"*.rmmzsave"_s },
                     .recursive = true };
        case EngineType::RM2K:
            return { .directory = base, .filter = { u"Save*.lsd"_s }, .recursive = false };
        default:
            return { .directory = base,
                     .filter = { u"Save*.%1"_qsv.arg(sets->engineExtension()) },
                     .recursive = false };
    }
}

namespace {
struct CachedSave {
    QString path;
    shared_ptr<const SaveSummary> summary;
    qint64 modified;
};

CachedSave cachedSave;
mutex cachedSaveMutex;
}  // namespace

auto getSaveSummary(const ProjectSettings* const sets) -> Result<shared_ptr<const SaveSummary>, Notice> {
    const QString path = sets->projectPath % u'/' % sets->activeSave;
    const qint64 modified = QFileInfo(path).lastModified().toMSecsSinceEpoch();

    {
        const std::scoped_lock lock(cachedSaveMutex);

        if (cachedSave.path == path && cachedSave.modified == modified) {
            return cachedSave.summary;
        }
    }

    const QByteArray pathUtf8 = path.toUtf8();
    const QByteArray encodingUtf8 = sets->readEncoding.toUtf8();
    FFIString out;

    if (!rpgm_get_save_summary(strtoffi(pathUtf8), sets->engineType, strtoffi(encodingUtf8), &out)) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to read the save %1: %2"),
            Severity::Warning,
            Delivery::Inline,
            { path, ffitostr(rpgm_error()) }
        ));
    }

    auto summary = make_shared<SaveSummary>();
    const auto error = glz::read<glz::opts{ .error_on_unknown_keys = false }>(*summary, string_view(out.ptr, out.len));
    rpgm_string_free(out);

    if (error) {
        return Err(Notice(
            NOTICE_IN("Utils", "Failed to read the save %1: %2"),
            Severity::Warning,
            Delivery::Inline,
            { path, QString::fromStdString(glz::format_error(error)) }
        ));
    }

    const std::scoped_lock lock(cachedSaveMutex);
    cachedSave = { .path = path, .summary = summary, .modified = modified };

    return summary;
}

[[nodiscard]] auto dataLocation() -> QString {
    return qApp->property("data-location").toString();
};

auto entryNameOfLine(const QStringView line) -> optional<QStringView> {
    if (!line.startsWith(NAME_COMMENT)) {
        return nullopt;
    }

    QStringView name = line.sliced(NAME_COMMENT.size());

    // `<!>NAMES` and the like are not entry names.
    if (!name.isEmpty() && !name.startsWith(SEPARATOR)) {
        return nullopt;
    }

    name = name.sliced(min<isize>(SEPARATOR.size(), name.size()));

    const isize end = name.indexOf(SEPARATOR);
    return end == -1 ? name : name.first(end);
}

auto entryNameFor(const QSVList& lines, const usize rowIndex) -> QStringView {
    for (usize idx = min(rowIndex + 1, scast<usize>(lines.size())); idx-- > 0;) {
        if (const auto name = entryNameOfLine(lines[idx])) {
            return *name;
        }

        if (lines[idx].startsWith(ID_COMMENT)) {
            break;
        }
    }

    return {};
}

auto countText(const QString& text, const u32 lines) -> TextCounts {
    const QByteArray utf8 = text.toUtf8();
    TextCounts counts = rpgm_count_text(strtoffi(utf8));

    // A line feed is a whitespace character, so the one that ends each line is in `characters`, not in
    // `characters_no_whitespace`.
    counts.characters -= min(counts.characters, lines);
    return counts;
}

auto translatableSource(const QStringView source) -> optional<QStringView> {
    if (!source.startsWith(COMMENT_PREFIX)) {
        return source;
    }

    if (source.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX)) {
        return source.sliced(MAP_DISPLAY_NAME_COMMENT_PREFIX.size());
    }

    return nullopt;
}
