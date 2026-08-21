#include "TabListDelegate.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "TabList.hpp"
#include "TabListModel.hpp"
#include "Utils.hpp"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionProgressBar>

namespace {
constexpr i32 PROGRESS_BAR_HEIGHT = 16;
constexpr i32 ROW_HEIGHT = 24;
constexpr i32 PERCENT_MULTIPLIER = 100;
}  // namespace

void TabListDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    painter->save();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();

    const auto fontMetrics = QFontMetrics(opt.font);
    const auto* const model = rcast<const TabListModel*>(index.model());

    if (maxCachedNameWidth == 0 || maxCachedProgressWidth == 0) {
        for (const auto idx : range(0, model->rowCount())) {
            const TabListItem& tab = model->tab(idx);

            const i32 spaceAdvance = fontMetrics.horizontalAdvance(u' ');
            maxCachedNameWidth =
                max(maxCachedNameWidth, fontMetrics.horizontalAdvance(tab.name) + (spaceAdvance * 2) + 2);

            const QString progressSample = itos(tab.translated).qsv() % u'/' % itos(tab.total).qsv();

            maxCachedProgressWidth =
                max(maxCachedProgressWidth, fontMetrics.horizontalAdvance(progressSample) + (spaceAdvance * 2) + 2);
        }
    }

    const TabListItem& tab = model->tab(index.row());
    if (tab.completed) {
        opt.backgroundBrush = QBrush(QColor(Qt::green));

        opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Text));
        opt.palette.setColor(QPalette::WindowText, opt.palette.color(QPalette::Text));
        opt.palette.setColor(QPalette::ButtonText, opt.palette.color(QPalette::Text));
    }

    const QStyle* const style = (opt.widget != nullptr) ? opt.widget->style() : qApp->style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const QString& name = tab.name;
    const i32 total = tab.total;
    const i32 translated = tab.translated;

    const QRect rect = opt.rect;
    static constexpr i32 MARGIN = 8;

    const QRect textRect = rect.adjusted(rect.left() + MARGIN, 0, -rect.width() + maxCachedNameWidth, 0);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    QStyleOptionProgressBar progressBar;
    progressBar.rect = QRect(
        textRect.right() + MARGIN,
        rect.center().y() - (PROGRESS_BAR_HEIGHT / 2),
        rect.width() - maxCachedNameWidth - maxCachedProgressWidth - (MARGIN * 2),
        PROGRESS_BAR_HEIGHT
    );
    progressBar.minimum = 0;
    progressBar.maximum = total;
    progressBar.progress = translated;

    const auto* const list = scast<const TabList*>(option.widget);
    const bool percentMode = list->displayingPercents();

    QString progressText;

    if (percentMode) {
        const u32 percent = scast<u32>((scast<f32>(translated) / scast<f32>(total)) * scast<f32>(PERCENT_MULTIPLIER));

        progressText = itos(percent).qsv() % u'%';
    } else {
        progressText = itos(translated).qsv() % u'/' % itos(total).qsv();
    }

    style->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);

    const auto progressTextRect = QRect(
        progressBar.rect.right() + MARGIN,
        progressBar.rect.top(),
        maxCachedProgressWidth,
        progressBar.rect.height()
    );

    painter->setPen(opt.palette.color(QPalette::Text));
    painter->drawText(progressTextRect, Qt::AlignVCenter | Qt::AlignLeft, progressText);

    painter->restore();
};

[[nodiscard]] auto TabListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& /* index */) const
    -> QSize {
    return { option.widget->width(), ROW_HEIGHT };
};