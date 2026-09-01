#include "TranslationTableModel.hpp"

#include "Constants.hpp"
#include "Utils.hpp"

TranslationTableModel::TranslationTableModel(QObject* const parent) : QAbstractItemModel(parent) {}

auto TranslationTableModel::rowCount(const QModelIndex& /* parent */) const -> i32 {
    return scast<i32>(rows.size());
}

auto TranslationTableModel::columnCount(const QModelIndex& /* parent */) const -> i32 {
    return colCount;
}

auto TranslationTableModel::index(const i32 row, const i32 col, const QModelIndex& parent) const -> QModelIndex {
    if (!hasIndex(row, col, parent)) {
        return {};
    }

    return createIndex(row, col);
}

[[nodiscard]] auto TranslationTableModel::parent(const QModelIndex& /* index */) const -> QModelIndex {
    return {};
};

auto TranslationTableModel::data(const QModelIndex& index, const i32 role) const -> QVariant {
    if (!index.isValid()) {
        return {};
    }

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return rows[index.row()][index.column()];
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
    flags.setFlag(Qt::ItemIsSelectable);

    if (index.column() != 0 && editableFlags[index.row()][index.column()]) {
        flags.setFlag(Qt::ItemIsEditable);
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

    QString& text = rows[index.row()][index.column()];

    if (rows[index.row()][0] == BOOKMARK_COMMENT) {
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

void TranslationTableModel::fill(std::span<QStringView> lines, const QString& filename) {
    beginResetModel();

    rows = {};
    editableFlags = {};
    colCount = 0;
    rows.reserve(scast<isize>(lines.size()));
    editableFlags.reserve(scast<isize>(lines.size()));

    for (const auto [row, line] : views::enumerate(lines)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const auto parts = lineParts(line, row, filename);

        if (parts.isEmpty()) {
            continue;
        }

        const QStringView source = getSource(parts);
        const QSVList translations = getTranslations(parts);

        if (source.startsWith(COMMENT_PREFIX)) {
            const bool editable = line.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX) || line.startsWith(BOOKMARK_COMMENT);

            QString commentText = editable ? line.sliced(0, line.indexOf(SEPARATOR)).toString() : line.toString();

            QString counterpartText;

            if (editable) {
                const u32 start = line.indexOf(SEPARATOR) + SEPARATOR.size();
                const isize end = line.indexOf(SEPARATOR, start);

                counterpartText = (end == -1 ? line.sliced(start) : line.sliced(start, end - start)).toString();
            }

            bitset<MAX_COLUMNS> flags;
            flags[1] = editable;

            array<QString, MAX_COLUMNS> newRow;
            newRow[0] = std::move(commentText);
            newRow[1] = std::move(counterpartText);

            colCount = std::max<u8>(colCount, 2);
            rows.append(std::move(newRow));
            editableFlags.append(std::move(flags));
            continue;
        }

        const u8 cols = scast<u8>(1 + translations.size());

        array<QString, MAX_COLUMNS> newRow;
        bitset<MAX_COLUMNS> flags;

        newRow[0] = qsvReplace(source, NEW_LINE, LINE_FEED);

        for (const auto [column, translation] : views::enumerate(translations)) {
            const u32 col = 1 + column;
            newRow[col] = qsvReplace(translation, NEW_LINE, LINE_FEED);
            flags[col] = true;
        }

        colCount = std::max<u8>(colCount, cols);
        rows.append(std::move(newRow));
        editableFlags.append(std::move(flags));
    }

    endResetModel();
}

void TranslationTableModel::clear() {
    beginResetModel();

    rows = {};
    editableFlags = {};
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
    const u8 col = colCount;
    beginInsertColumns(QModelIndex(), col, col);

    for (i32 row = 0; row < rows.size(); row++) {
        rows[row][col] = row < cells.size() ? std::move(cells[row]) : QString();
        editableFlags[row][col] = true;
    }

    colCount++;
    endInsertColumns();
}

void TranslationTableModel::insertRow(const u32 row, QStringList cells) {
    beginInsertRows({}, scast<i32>(row), scast<i32>(row));

    array<QString, MAX_COLUMNS> newRow;
    bitset<MAX_COLUMNS> flags;

    for (i32 col = 0; col < cells.size(); col++) {
        newRow[col] = std::move(cells[col]);
    }

    rows.insert(row, std::move(newRow));
    editableFlags.insert(row, std::move(flags));
    endInsertRows();
}

auto TranslationTableModel::item(const u32 row, const u8 column) -> Cell {
    return { &rows[row][column], editableFlags[row][column] };
};

auto TranslationTableModel::itemFromIndex(const QModelIndex& index) -> Cell {
    return { &rows[index.row()][index.column()], editableFlags[index.row()][index.column()] };
};