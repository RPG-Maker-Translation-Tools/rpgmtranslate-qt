#include "TranslationTableModel.hpp"

#include "Constants.hpp"
#include "Utils.hpp"

TranslationTableModel::TranslationTableModel(QObject* const parent) : QAbstractTableModel(parent) {}

auto TranslationTableModel::rowCount(const QModelIndex& /* parent */) const -> i32 {
    return scast<i32>(editableColumns.size());
}

auto TranslationTableModel::columnCount(const QModelIndex& /* parent */) const -> i32 {
    return colCount;
}

auto TranslationTableModel::data(const QModelIndex& index, const i32 role) const -> QVariant {
    if (!index.isValid()) {
        return {};
    }

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return cell(index.row(), index.column());
        case Qt::TextAlignmentRole:
            return scast<i32>(Qt::AlignLeft | Qt::AlignTop);
        default:
            return {};
    }
}

auto TranslationTableModel::headerData(const i32 section, const Qt::Orientation orientation, const i32 role) const
    -> QVariant {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < headers.size()) {
        return headers[section];
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

auto TranslationTableModel::setHeaderData(
    const i32 section,
    const Qt::Orientation orientation,
    const QVariant& value,
    const i32 role
) -> bool {
    if (role != Qt::DisplayRole) {
        return QAbstractItemModel::setHeaderData(section, orientation, value, role);
    }

    if (headers.size() <= section) {
        headers.resize(section + 1);
    }

    headers[section] = value.toString();
    emit headerDataChanged(Qt::Horizontal, section, section);
    return true;
}

auto TranslationTableModel::flags(const QModelIndex& index) const -> Qt::ItemFlags {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags flags;

    flags.setFlag(Qt::ItemIsEnabled);

    if (index.column() != 0 && isEditable(index.row(), scast<u8>(index.column()))) {
        flags.setFlag(Qt::ItemIsEditable);
        flags.setFlag(Qt::ItemIsSelectable);
    }

    return flags;
}

auto TranslationTableModel::setData(const QModelIndex& index, const QVariant& value, const i32 role) -> bool {
    if (!index.isValid()) {
        return false;
    }

    if (role != Qt::EditRole && role != Qt::DisplayRole) {
        return false;
    }

    QString& text = cell(index.row(), index.column());

    if (cell(index.row(), 0) == BOOKMARK_COMMENT) {
        text = value.toString();
        emit dataChanged(index, index, { role });
        emit bookmarkChanged(index.row());
        return true;
    }

    const QString newText = value.toString();
    const bool oldTranslated = !QStringView(text).trimmed().isEmpty();
    const bool newTranslated = !QStringView(newText).trimmed().isEmpty();

    text = newText;
    emit dataChanged(index, index, { role });

    if (oldTranslated != newTranslated) {
        emit translatedChanged(newTranslated ? +1 : -1);
    }

    return true;
}

void TranslationTableModel::restride(const u8 newCount) {
    const usize rowCount = scast<usize>(editableColumns.size());
    const u8 keptCount = min(colCount, newCount);

    QList<QString> resized;
    resized.resize(scast<isize>(rowCount * newCount));

    for (usize row = 0; row < rowCount; row++) {
        for (u8 column = 0; column < keptCount; column++) {
            resized[(row * newCount) + column] = std::move(cell(row, column));
        }
    }

    texts = std::move(resized);
    colCount = newCount;
}

void TranslationTableModel::fill(std::span<QStringView> lines, const QString& filename) {
    static_assert(MAX_COLUMNS <= 16, "`editableColumns` holds a bit per column in a `u16`");

    beginResetModel();

    texts = {};
    editableColumns = {};
    colCount = 0;
    editableColumns.reserve(scast<isize>(lines.size()));
    // Two columns is the smallest table; a wider one is widened below
    texts.reserve(scast<isize>(lines.size()) * 2);

    // Appends a row of `cols` columns; the cells are left empty for the caller to fill
    const auto appendRow = [this](const u8 cols, const u16 editable) -> isize {
        if (cols > colCount) {
            restride(cols);
        }

        const isize base = texts.size();
        texts.resize(base + colCount);
        editableColumns.append(editable);
        return base;
    };

    for (const auto [row, line] : views::enumerate(lines)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const auto parts = lineParts(line, row, filename);

        if (parts.isEmpty() || parts.size() > MAX_COLUMNS) {
            texts[appendRow(2, 0)] = line.toString();
            continue;
        }

        const QStringView source = getSource(parts);
        const QSVList translations = getTranslations(parts);

        if (source.startsWith(COMMENT_PREFIX)) {
            const bool editable = line.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX) || line.startsWith(BOOKMARK_COMMENT);

            // `lineParts` guarantees at least two parts; other comments are kept as the whole line
            const isize base = appendRow(2, editable ? 1 << 1 : 0);
            texts[base] = editable ? source.toString() : line.toString();

            if (editable) {
                texts[base + 1] = parts[1].toString();
            }

            continue;
        }

        const u8 cols = scast<u8>(1 + translations.size());
        const isize base = appendRow(cols, scast<u16>(((1 << cols) - 1) & ~1));

        texts[base] = qsvReplace(source, NEW_LINE, LINE_FEED);

        for (const auto [column, translation] : views::enumerate(translations)) {
            texts[base + 1 + column] = qsvReplace(translation, NEW_LINE, LINE_FEED);
        }
    }

    // A row with fewer parts than the table has columns can still be translated in the others
    const u16 allColumns = scast<u16>(((1 << colCount) - 1) & ~1);

    for (u16& editable : editableColumns) {
        if ((editable & (1 << 1)) != 0) {
            editable = allColumns;
        }
    }

    endResetModel();
}

void TranslationTableModel::clear() {
    beginResetModel();

    texts = {};
    editableColumns = {};
    headers = {};
    colCount = 0;

    endResetModel();
}

void TranslationTableModel::setHeaderLabels(QStringList labels) {
    for (const auto& [idx, label] : views::enumerate(labels)) {
        setHeaderData(scast<i32>(idx), Qt::Horizontal, label);
    }
};

void TranslationTableModel::appendColumn(QStringList cells) {
    if (colCount >= MAX_COLUMNS) {
        return;
    }

    const u8 col = colCount;
    beginInsertColumns(QModelIndex(), col, col);

    restride(col + 1);

    for (isize row = 0; row < editableColumns.size(); row++) {
        if (row < cells.size()) {
            cell(row, col) = std::move(cells[row]);
        }

        // Comments and kept lines get no editable cell
        if (isEditable(row, 1)) {
            editableColumns[row] |= scast<u16>(1 << col);
        }
    }

    endInsertColumns();
}

// Only bookmarks are inserted, so the table already has the two columns of a comment row
void TranslationTableModel::insertRow(const u32 row, QStringList cells) {
    beginInsertRows({}, scast<i32>(row), scast<i32>(row));

    texts.insert(scast<isize>(row) * colCount, colCount, QString());

    for (i32 col = 0; col < min<i32>(cells.size(), colCount); col++) {
        cell(row, col) = std::move(cells[col]);
    }

    // Their cells are editable in every column, as when loaded
    editableColumns.insert(row, scast<u16>(((1 << colCount) - 1) & ~1));
    endInsertRows();
}

auto TranslationTableModel::removeRows(const i32 row, const i32 count, const QModelIndex& parent) -> bool {
    if (parent.isValid() || count <= 0 || row < 0 || row + count > editableColumns.size()) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    texts.remove(scast<isize>(row) * colCount, scast<isize>(count) * colCount);
    editableColumns.remove(row, count);
    endRemoveRows();

    return true;
}

auto TranslationTableModel::item(const u32 row, const u8 column) const -> const QString* {
    return &cell(row, column);
};

auto TranslationTableModel::itemFromIndex(const QModelIndex& index) const -> const QString* {
    return &cell(index.row(), index.column());
};
