#include "TabListDelegate.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "TabList.hpp"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionProgressBar>
#include <QStyledItemDelegate>

void TabListDelegate::paint(
    QPainter* const painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    painter->save();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();

    if (index.data(TabList::Roles::CompletedRole).toBool()) {
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
        (opt.widget != nullptr) ? opt.widget->style() : QApplication::style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const QString name = index.data(TabList::Roles::NameRole).toString();
    const u32 total = index.data(TabList::Roles::TotalRole).toUInt();
    const u32 translated = index.data(TabList::Roles::TranslatedRole).toUInt();

    const QRect rect = option.rect;
    const u8 margin = 6;
    const u16 textWidth = u16(f32(rect.width()) * 0.4F);

    const QRect textRect =
        rect.adjusted(margin, 0, -rect.width() + textWidth, 0);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    QStyleOptionProgressBar progressBar;
    progressBar.rect = QRect(
        textRect.right() + margin,
        rect.center().y() - 8,
        rect.width() - textWidth - (3 * margin),
        16
    );
    progressBar.minimum = 0;
    progressBar.maximum = total;
    progressBar.progress = translated;
    progressBar.textVisible = true;
    progressBar.textAlignment = Qt::AlignLeft | Qt::AlignTop;

    const auto* const list = as<const TabList*>(option.widget);
    const bool percentMode = list->displayingPercents();

    if (percentMode) {
        const u32 percent =
            u32((f32(translated) / f32(total)) * f32(PERCENT_MULTIPLIER));

        progressBar.text = QString::number(percent) + '%';
    } else {
        progressBar.text =
            QString::number(translated) + '/' + QString::number(total);
    }

    style->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);

    painter->restore();
};

[[nodiscard]] auto TabListDelegate::sizeHint(
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const -> QSize {
    return { option.widget->width(), 24 };
};