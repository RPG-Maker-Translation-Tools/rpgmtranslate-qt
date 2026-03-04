#include "MatchTable.hpp"

#include "Constants.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QApplication>
#include <QPainter>
#include <QTextLayout>

struct SearchMatch {
    const u32 start;
    const u32 len;
    const f32 score;
};

MatchTableDelegate::MatchTableDelegate(
    MatchTableModel* const model,
    MatchTable* const view,
    QObject* const parent
) :
    QStyledItemDelegate(parent),
    model(model) {}

void MatchTableDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& opt,
    const QModelIndex& index
) const {
    const bool highlightColumn =
        index.column() == MatchTableModel::SourceMatch ||
        index.column() == MatchTableModel::TranslationMatch;

    QStyleOptionViewItem option(opt);
    initStyleOption(&option, index);

    painter->save();
    const QRect rect = opt.rect.adjusted(PAD_X, PAD_Y, -PAD_X, -PAD_Y);
    option.rect = rect;

    if (highlightColumn) {
        option.text.clear();
    }

    QStyle* const style = option.widget->style();
    style
        ->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    if (!highlightColumn) {
        painter->restore();
        return;
    }

    const auto& row = model->row(index.row());
    const auto& text = index.column() == MatchTableModel::SourceMatch
                           ? row.sourceText
                           : row.translationText;
    const auto& spans = index.column() == MatchTableModel::SourceMatch
                            ? row.sourceSpans
                            : row.translationSpans;

    QTextCharFormat highlightedFormat;
    QColor highlightedBG = opt.palette.color(QPalette::Highlight);
    highlightedBG.setAlphaF(0.85F);
    highlightedFormat.setBackground(highlightedBG);

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto textLayout = QTextLayout(text, opt.font);
    textLayout.setTextOption(textOption);

    QList<QTextLayout::FormatRange> formatRanges;
    formatRanges.reserve(isize(spans.size()));
    for (const auto span : spans) {
        formatRanges
            .emplace_back(i32(span.start), i32(span.len), highlightedFormat);
    }
    textLayout.setFormats(formatRanges);

    textLayout.beginLayout();
    f32 yPos = 0.0F;

    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(rect.width());
        line.setPosition(QPointF(0.0F, yPos));
        yPos += f32(line.height());
    }

    textLayout.endLayout();

    painter->setPen(opt.palette.color(QPalette::Active, QPalette::Text));
    textLayout.draw(painter, QPointF(rect.left(), rect.top()));

    painter->restore();
}

[[nodiscard]] auto MatchTableDelegate::sizeHint(
    const QStyleOptionViewItem& opt,
    const QModelIndex& index
) const -> QSize {
    const bool highlightColumn =
        index.column() == MatchTableModel::SourceMatch ||
        index.column() == MatchTableModel::TranslationMatch;

    if (!highlightColumn) {
        return QStyledItemDelegate::sizeHint(opt, index);
    }

    const auto fontMentrics = QFontMetrics(opt.font);

    const auto& row = model->row(index.row());
    const auto& text = index.column() == MatchTableModel::SourceMatch
                           ? row.sourceText
                           : row.translationText;
    const i32 height =
        (fontMentrics.lineSpacing() * (1 + i32(text.count(LINE_SEPARATOR)))) +
        (PAD_Y * 2);

    return { opt.rect.width() + (PAD_X * 2), height };
}

[[nodiscard]] auto MatchTableModel::columnCount(const QModelIndex& parent) const
    -> i32 {
    return ColumnCount;
}

[[nodiscard]] auto MatchTableModel::flags(const QModelIndex& index) const
    -> Qt::ItemFlags {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

[[nodiscard]] auto MatchTableModel::data(
    const QModelIndex& index,
    const i32 role
) const -> QVariant {
    if (!index.isValid()) {
        return {};
    }

    const auto& row = rows[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case Filename:
                return row.filename;
            case Line:
                return i32(row.lineNumber);
            case TermOccurrences:
                return row.termOccurrences;
            case TranslationOccurrences:
                return row.translationOccurrences;
            case SourceMatch:
                return row.sourceText;
            case TranslationMatch:
                return row.translationText;
            case Info:
                return row.info;
            default:
                return {};
        }
    } else if (role == Qt::TextAlignmentRole) {
        return i32(Qt::AlignTop | Qt::AlignLeft);
    }

    return {};
}

[[nodiscard]] auto MatchTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
) const -> QVariant {
    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
        case Filename:
            return tr("Filename");
        case Line:
            return tr("Line");
        case TermOccurrences:
            return tr("Term");
        case TranslationOccurrences:
            return tr("Translation");
        case SourceMatch:
            return tr("Source match");
        case TranslationMatch:
            return tr("Translation match");
        case Info:
            return tr("Info");
        default:
            return {};
    }
}

void MatchTableModel::appendRow(Row row) {
    const i32 newRow = i32(rows.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    rows.push_back(std::move(row));
    endInsertRows();
}

auto MatchTableModel::row(const u32 row) -> const Row& {
    return rows[row];
}

void MatchTableModel::clear() {
    beginResetModel();
    rows.clear();
    endResetModel();
}

MatchTable::MatchTable(QWidget* const parent) :
    QTableView(parent),

    model_(new MatchTableModel(this)),
    delegate(new MatchTableDelegate(model_, this)) {
    setModel(model_);
    setItemDelegate(delegate);

    setEditTriggers(QTableView::NoEditTriggers);
    setSortingEnabled(false);
    setDragEnabled(false);
    setAcceptDrops(false);
    setDropIndicatorShown(false);
    setDragDropMode(QTableView::NoDragDrop);

    auto* const horHeader = horizontalHeader();
    horHeader->setSectionsMovable(false);
    horHeader->setStretchLastSection(false);

    verticalHeader()->setVisible(false);
    setSelectionBehavior(QTableView::SelectRows);
    setSelectionMode(QTableView::SingleSelection);

    setWordWrap(true);

    horHeader->setSectionResizeMode(
        MatchTableModel::Filename,
        QHeaderView::ResizeToContents
    );
    horHeader->setSectionResizeMode(
        MatchTableModel::Line,
        QHeaderView::ResizeToContents
    );
    horHeader->setSectionResizeMode(
        MatchTableModel::TermOccurrences,
        QHeaderView::ResizeToContents
    );
    horHeader->setSectionResizeMode(
        MatchTableModel::TranslationOccurrences,
        QHeaderView::ResizeToContents
    );
    horHeader->setSectionResizeMode(
        MatchTableModel::SourceMatch,
        QHeaderView::Stretch
    );
    horHeader->setSectionResizeMode(
        MatchTableModel::TranslationMatch,
        QHeaderView::Stretch
    );
    horHeader->setSectionResizeMode(
        MatchTableModel::Info,
        QHeaderView::ResizeToContents
    );
}

[[nodiscard]] auto MatchTable::model() const -> MatchTableModel* {
    return model_;
}

void MatchTable::appendMatch(
    const QString& filename,
    const u32 lineIndex,
    const QString& termSource,
    const QString& termTranslation,
    const QStringView source,
    const QStringView translation,
    const ByteBuffer matches
) {
    if (matches.len == 0) {
        return;
    }

    const u8* ptr = matches.ptr;

    u32 sourceCount = *ras<const u32*>(ptr);
    ptr += sizeof(u32);

    auto sourceMatches = span(ras<const SearchMatch*>(ptr), sourceCount);
    ptr += sourceCount * sizeof(SearchMatch);

    u32 translationCount = *ras<const u32*>(ptr);
    ptr += sizeof(u32);

    auto translationMatches =
        span(ras<const SearchMatch*>(ptr), translationCount);
    ptr += translationCount * sizeof(SearchMatch);

    QStringList sourceMatchDescs;
    vector<Span> sourceSpans;

    sourceMatchDescs.reserve(sourceCount);
    sourceSpans.reserve(sourceCount);

    for (const auto match : sourceMatches) {
        if (match.score == 0.0F) {
            sourceMatchDescs.append(tr("Exact"));
        } else {
            sourceMatchDescs.append(
                tr("Fuzzy (%1)").arg(QString::number(match.score, 10, 3))
            );
        }

        sourceSpans.emplace_back(match.start, match.len);
    }

    QStringList translationMatchDescs;
    vector<Span> translationSpans;

    translationMatchDescs.reserve(translationCount);
    translationSpans.reserve(translationCount);

    for (const auto match : translationMatches) {
        if (match.score == 0.0F) {
            translationMatchDescs.append(tr("Exact"));
        } else {
            translationMatchDescs.append(
                tr("Fuzzy (%1)").arg(QString::number(match.score, 10, 3))
            );
        }

        translationSpans.emplace_back(match.start, match.len);
    }

    QString termOccurrences = tr("%1, %2 occurrences, %3")
                                  .arg(termSource)
                                  .arg(sourceCount)
                                  .arg(sourceMatchDescs.join(", "_L1));

    QString translationOccurrences =
        tr("%1, %2 occurrences, %3")
            .arg(termTranslation)
            .arg(translationCount)
            .arg(translationMatchDescs.join(", "_L1));

    QString info;
    const bool match = sourceCount <= translationCount;

    if (translation.isEmpty()) {
        info = tr("Translation is empty.");
    } else if (translationCount == 0) {
        info = tr("Term translation is not present.");
    } else if (!match) {
        info = tr(
            "Number of term occurrences doesn't match the number of translation occurrences."
        );
    } else {
        info = tr("Match.");
    }

    MatchTableModel::Row row;
    row.filename = filename;
    row.lineNumber = lineIndex + 1;
    row.termOccurrences = std::move(termOccurrences);
    row.translationOccurrences = std::move(translationOccurrences);

    row.sourceText = qsvReplace(source, '\n', LINE_SEPARATOR);
    row.sourceSpans = std::move(sourceSpans);

    row.translationText = qsvReplace(translation, '\n', LINE_SEPARATOR);
    row.translationSpans = std::move(translationSpans);

    row.info = std::move(info);

    model_->appendRow(std::move(row));

    QMetaObject::invokeMethod(this, [this] -> void {
        resizeColumnsToContents();
        resizeRowsToContents();
    }, Qt::QueuedConnection);
}
