#include "TranslationTableHeader.hpp"

#include "LabelInput.hpp"

#include <QAbstractItemModel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

TranslationTableHeader::TranslationTableHeader(QWidget* const parent) :
    QHeaderView(Qt::Horizontal, parent) {
    setSectionsClickable(true);
    setHighlightSections(true);

    labelInput = new LabelInput(viewport());

    connect(
        labelInput,
        &LabelInput::editingFinished,
        this,
        &TranslationTableHeader::commitEdit
    );
}

TranslationTableHeader::~TranslationTableHeader() {}

void TranslationTableHeader::paintSection(
    QPainter* const painter,
    const QRect& rect,
    const i32 logicalIndex
) const {
    QHeaderView::paintSection(painter, rect, logicalIndex);
}

void TranslationTableHeader::paintEvent(QPaintEvent* const event) {
    QHeaderView::paintEvent(event);

    auto painter = QPainter(viewport());
    const QRect buttonRect = getButtonRect();

    if (buttonRect.intersects(viewport()->rect())) {
        QStyleOptionButton buttonOpt;
        buttonOpt.rect = buttonRect;
        buttonOpt.icon = QIcon(u":/icons/add.svg"_s);

        buttonOpt.state = QStyle::State_Enabled;

        if (addButtonPressed) {
            buttonOpt.state |= QStyle::State_Sunken;
        } else {
            buttonOpt.state |= QStyle::State_Raised;
        }

        if (addButtonHovered) {
            buttonOpt.state |= QStyle::State_MouseOver;
        }

        style()->drawControl(QStyle::CE_PushButton, &buttonOpt, &painter, this);
    }
}

void TranslationTableHeader::mousePressEvent(QMouseEvent* const event) {
    if (labelInput->isVisible() &&
        !labelInput->geometry().contains(event->pos())) {
        commitEdit();
    }

    const QRect buttonRect = getButtonRect();
    if (buttonRect.contains(event->pos())) {
        addButtonPressed = true;
        viewport()->update();
        return;
    }

    QHeaderView::mousePressEvent(event);
}

void TranslationTableHeader::mouseReleaseEvent(QMouseEvent* const event) {
    const QRect buttonRect = getButtonRect();

    if (addButtonPressed && buttonRect.contains(event->pos())) {
        addButtonPressed = false;
        viewport()->update();
        emit addButtonClicked();
        return;
    }

    addButtonPressed = false;
    viewport()->update();
    QHeaderView::mouseReleaseEvent(event);
}

void TranslationTableHeader::mouseMoveEvent(QMouseEvent* const event) {
    const QRect buttonRect = getButtonRect();
    const bool nowHovered = buttonRect.contains(event->pos());

    if (nowHovered != addButtonHovered) {
        addButtonHovered = nowHovered;
        viewport()->update();

        if (addButtonHovered) {
            setCursor(Qt::PointingHandCursor);
        } else {
            unsetCursor();
        }
    }

    QHeaderView::mouseMoveEvent(event);
}

void TranslationTableHeader::mouseDoubleClickEvent(QMouseEvent* const event) {
    if (getButtonRect().contains(event->pos())) {
        event->accept();
        return;
    }

    const i32 idx = logicalIndexAt(event->pos());
    if (idx < 0) {
        QHeaderView::mouseDoubleClickEvent(event);
        return;
    }

    if (idx >= 1) {
        beginEditSection(idx);
        event->accept();
        return;
    }

    QHeaderView::mouseDoubleClickEvent(event);
}

void TranslationTableHeader::leaveEvent(QEvent* const event) {
    if (addButtonHovered) {
        addButtonHovered = false;
        viewport()->update();
        unsetCursor();
    }

    QHeaderView::leaveEvent(event);
}

[[nodiscard]] auto TranslationTableHeader::sizeHint() const -> QSize {
    QSize size = QHeaderView::sizeHint();
    size.setWidth(size.width() + 32);
    return size;
}

[[nodiscard]] auto TranslationTableHeader::getButtonRect() const -> QRect {
    const i32 xPos =
        sectionViewportPosition(count() - 1) + sectionSize(count() - 1);
    const i32 buttonWidth = 32;
    return { xPos, 0, buttonWidth, height() };
}

void TranslationTableHeader::beginEditSection(const i32 logicalIndex) {
    if (labelInput->isVisible()) {
        commitEdit();
    }

    editingSection = logicalIndex;

    const auto rect = QRect(
        sectionViewportPosition(logicalIndex),
        0,
        sectionSize(logicalIndex),
        height()
    );

    const QString text =
        model()
            ->headerData(logicalIndex, orientation(), Qt::DisplayRole)
            .toString();

    labelInput->setText(text);
    labelInput->setGeometry(rect.adjusted(4, 2, -4, -2));
    labelInput->show();
    labelInput->setFocus(Qt::MouseFocusReason);
    labelInput->selectAll();
}

void TranslationTableHeader::commitEdit() {
    const QString newText = labelInput->text().trimmed();
    const i32 idx = editingSection;

    labelInput->hide();
    editingSection = -1;

    model()->setHeaderData(idx, orientation(), newText, Qt::EditRole);
    viewport()->update();
}
