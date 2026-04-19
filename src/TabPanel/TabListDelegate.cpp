#include "TabListDelegate.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "TabList.hpp"
#include "TabListModel.hpp"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionProgressBar>

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
    const auto* const model = ras<const TabListModel*>(index.model());

    if (maxCachedNameWidth == 0 || maxCachedProgressWidth == 0) {
        for (const auto idx : range(0, model->rowCount())) {
            const TabListItem& tab = model->tab(idx);

            const u16 spaceAdvance = fontMetrics.horizontalAdvance(u' ');
            maxCachedNameWidth = max<u16>(
                maxCachedNameWidth,
                fontMetrics.horizontalAdvance(tab.name) + (spaceAdvance * 2) + 2
            );

            const QString progressSample = QL1SV(itos(tab.translated).data()) %
                                           u'/' % QL1SV(itos(tab.total).data());

            maxCachedProgressWidth = max<u16>(
                maxCachedProgressWidth,
                fontMetrics.horizontalAdvance(progressSample) +
                    (spaceAdvance * 2) + 2
            );
        }
    }

    const TabListItem& tab = model->tab(index.row());
    if (tab.completed) {
        opt.backgroundBrush = QBrush(QColor(Qt::green));

        opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Text));
        opt.palette.setColor(
            QPalette::WindowText,
            opt.palette.color(QPalette::Text)
        );
        opt.palette.setColor(
            QPalette::ButtonText,
            opt.palette.color(QPalette::Text)
        );
    }

    const QStyle* const style =
        (opt.widget != nullptr) ? opt.widget->style() : qApp->style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const QString& name = tab.name;
    const u32 total = tab.total;
    const u32 translated = tab.translated;

    const QRect rect = opt.rect;
    constexpr u8 MARGIN = 8;

    const QRect textRect = rect.adjusted(
        rect.left() + MARGIN,
        0,
        -rect.width() + maxCachedNameWidth,
        0
    );
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    QStyleOptionProgressBar progressBar;
    progressBar.rect = QRect(
        textRect.right() + MARGIN,
        rect.center().y() - 8,
        rect.width() - maxCachedNameWidth - maxCachedProgressWidth -
            (MARGIN * 2),
        16
    );
    progressBar.minimum = 0;
    progressBar.maximum = i32(total);
    progressBar.progress = i32(translated);

    const auto* const list = as<const TabList*>(option.widget);
    const bool percentMode = list->displayingPercents();

    QString progressText;

    if (percentMode) {
        const u32 percent =
            u32((f32(translated) / f32(total)) * f32(PERCENT_MULTIPLIER));

        progressText = QL1SV(itos(percent).data()) + u'%';
    } else {
        progressText =
            QL1SV(itos(translated).data()) % u'/' % QL1SV(itos(total).data());
    }

    style->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);

    const auto progressTextRect = QRect(
        progressBar.rect.right() + MARGIN,
        progressBar.rect.top(),
        maxCachedProgressWidth,
        progressBar.rect.height()
    );

    painter->setPen(opt.palette.color(QPalette::Text));
    painter->drawText(
        progressTextRect,
        Qt::AlignVCenter | Qt::AlignLeft,
        progressText
    );

    painter->restore();
};

[[nodiscard]] auto TabListDelegate::sizeHint(
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const -> QSize {
    return { option.widget->width(), 24 };
};