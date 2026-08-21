#include "LintTable.hpp"

#include "Constants.hpp"
#include "Utils.hpp"

#include <QApplication>
#include <QPainter>
#include <QTextLayout>

namespace {
constexpr f32 HIGHLIGHT_ALPHA = 0.85F;
constexpr i32 PAD_X = 4;
constexpr i32 PAD_Y = 4;

constexpr i32 MAX_WRAPPED_COLUMN_WIDTH = 400;

[[nodiscard]] constexpr auto wrappableColumn(const i32 column) -> bool {
    return column == LintTableModel::SourceMatch || column == LintTableModel::TranslationMatch ||
           column == LintTableModel::Info;
}

[[nodiscard]] auto normalizeLineBreaks(const QString& text) -> QString {
    return qsvReplace(QStringView(text), u'\n', LINE_SEPARATOR);
}

[[nodiscard]] auto wrappedText(LintTableModel* const model, const QModelIndex& index) -> QString {
    const auto& row = model->row(index.row());

    switch (index.column()) {
        case LintTableModel::SourceMatch:
            return normalizeLineBreaks(row.sourceText);
        case LintTableModel::TranslationMatch:
            return normalizeLineBreaks(row.translationText);
        case LintTableModel::Info:
            return normalizeLineBreaks(row.info);
        default:
            return {};
    }
}

auto layoutWrappedText(QTextLayout& layout, const i32 width) -> f32 {
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout.setTextOption(textOption);

    layout.beginLayout();
    f32 yPos = 0.0F;

    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(scast<f32>(width));
        line.setPosition(QPointF(0.0F, yPos));
        yPos += scast<f32>(line.height());
    }

    layout.endLayout();
    return yPos;
}
}  // namespace

MatchTableDelegate::MatchTableDelegate(LintTableModel* const model, QObject* const parent) :
    QStyledItemDelegate(parent),
    model(model) {}

void MatchTableDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& opt,
    const QModelIndex& index
) const {
    const bool wrappable = wrappableColumn(index.column());

    auto option = opt;
    initStyleOption(&option, index);

    painter->save();
    const QRect rect = opt.rect.adjusted(PAD_X, PAD_Y, -PAD_X, -PAD_Y);
    option.rect = rect;

    if (wrappable) {
        option.text.clear();
    }

    const QStyle* const style = option.widget->style();
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    if (!wrappable) {
        painter->restore();
        return;
    }

    const auto& row = model->row(index.row());
    const QString text = wrappedText(model, index);

    const bool highlightColumn =
        index.column() == LintTableModel::SourceMatch || index.column() == LintTableModel::TranslationMatch;
    const auto& spans =
        index.column() == LintTableModel::SourceMatch ? row.sourceHighlights : row.translationHighlights;

    QTextCharFormat highlightedFormat;
    QColor highlightedBG = opt.palette.color(QPalette::Highlight);
    highlightedBG.setAlphaF(HIGHLIGHT_ALPHA);
    highlightedFormat.setBackground(highlightedBG);

    auto textLayout = QTextLayout(text, opt.font);

    if (highlightColumn) {
        QList<QTextLayout::FormatRange> formatRanges;
        formatRanges.reserve(scast<isize>(spans.size()));
        for (const auto span : spans) {
            formatRanges.emplace_back(span.start, span.len, highlightedFormat);
        }
        textLayout.setFormats(formatRanges);
    }

    layoutWrappedText(textLayout, rect.width());

    painter->setPen(opt.palette.color(QPalette::Active, QPalette::Text));
    textLayout.draw(painter, QPointF(rect.left(), rect.top()));

    painter->restore();
}

[[nodiscard]] auto MatchTableDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
    -> QSize {
    if (!wrappableColumn(index.column())) {
        return QStyledItemDelegate::sizeHint(opt, index);
    }

    const QString text = wrappedText(model, index);

    const auto* const view = qobject_cast<const QTableView*>(opt.widget);
    const i32 columnWidth = view != nullptr ? view->columnWidth(index.column()) - (PAD_X * 2) : 0;

    auto textLayout = QTextLayout(text, opt.font);
    const f32 height = layoutWrappedText(textLayout, columnWidth > 0 ? columnWidth : MAX_WRAPPED_COLUMN_WIDTH);

    const i32 naturalWidth = scast<i32>(std::ceil(textLayout.maximumWidth()));

    return { std::min(naturalWidth, MAX_WRAPPED_COLUMN_WIDTH) + (PAD_X * 2), scast<i32>(height) + (PAD_Y * 2) };
}

[[nodiscard]] auto LintTableModel::columnCount(const QModelIndex& /* parent */) const -> i32 {
    return ColumnCount;
}

[[nodiscard]] auto LintTableModel::flags(const QModelIndex& index) const -> Qt::ItemFlags {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

[[nodiscard]] auto LintTableModel::data(const QModelIndex& index, const i32 role) const -> QVariant {
    if (!index.isValid()) {
        return {};
    }

    const auto& row = rows[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case Filename:
                return row.filename;
            case Line:
                return row.lineNumber;
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
        return scast<i32>(Qt::AlignTop | Qt::AlignLeft);
    }

    return {};
}

[[nodiscard]] auto
LintTableModel::headerData(const i32 section, const Qt::Orientation /* orientation */, const i32 role) const
    -> QVariant {
    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
        case Filename:
            return tr("File");
        case Line:
            return tr("Line");
        case SourceMatch:
            return tr("Source");
        case TranslationMatch:
            return tr("Translation");
        case Info:
            return tr("Info");
        default:
            return {};
    }
}

void LintTableModel::appendRow(Row row) {
    const i32 newRow = scast<i32>(rows.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    rows.push_back(std::move(row));
    endInsertRows();
}

auto LintTableModel::row(const u32 row) -> const Row& {
    return rows[row];
}

void LintTableModel::clear() {
    beginResetModel();
    rows.clear();
    endResetModel();
}

LintTable::LintTable(QWidget* const parent) :
    QTableView(parent),

    model_(new LintTableModel(this)),
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

    setWordWrap(false);
    setTextElideMode(Qt::ElideNone);

    connect(horHeader, &QHeaderView::sectionResized, this, [this] -> void { resizeRowsToContents(); });
}

[[nodiscard]] auto LintTable::model() const -> LintTableModel* {
    return model_;
}

void LintTable::appendRow(LintTableModel::Row row) {
    model_->appendRow(std::move(row));

    QMetaObject::invokeMethod(this, [this] -> void {
        resizeColumnsToContents();
        resizeRowsToContents();
    }, Qt::QueuedConnection);
}

void LintTable::appendRows(vector<LintTableModel::Row> rows) {
    for (auto& row : rows) {
        model_->appendRow(std::move(row));
    }

    QMetaObject::invokeMethod(this, [this] -> void {
        resizeColumnsToContents();
        resizeRowsToContents();
    }, Qt::QueuedConnection);
}
