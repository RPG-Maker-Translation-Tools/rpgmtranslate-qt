#include "PersistentMenu.hpp"

#include "Aliases.hpp"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>

PersistentMenu::PersistentMenu(
    QWidget* const parent,
    const Qt::WindowFlags flags
) :
    QWidget(parent, flags) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("PersistentMenu { background-color: %1 }"_L1.arg(
        qApp->palette().color(QPalette::Window).name()
    ));
    hide();
}

void PersistentMenu::resetMouseMovedFlag() noexcept {
    setMouseMovedInternal(false);
}

void PersistentMenu::setDragMoveEnabled(const bool enabled) noexcept {
    if (dragMoveEnabled_ == enabled) {
        return;
    }

    dragMoveEnabled_ = enabled;
    emit dragMoveEnabledChanged(dragMoveEnabled_);
}

auto PersistentMenu::event(QEvent* const event) -> bool {
    switch (event->type()) {
        case QEvent::Show:
            emit shown();
            break;
        case QEvent::Hide:
            emit hidden();
            break;
        default:
            break;
    }
    return QWidget::event(event);
}

auto PersistentMenu::canStartDragAt(const QPoint& /* posInWidget */) const
    -> bool {
    return true;
}

void PersistentMenu::mousePressEvent(QMouseEvent* const event) {
    if (dragMoveEnabled_ && event->button() == Qt::LeftButton &&
        canStartDragAt(event->position().toPoint())) {
        dragging_ = true;
        pressPosGlobal_ = event->globalPosition().toPoint();
        startTopLeftGlobal_ = frameGeometry().topLeft();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void PersistentMenu::mouseMoveEvent(QMouseEvent* const event) {
    if (dragging_ && dragMoveEnabled_) {
        const QPoint curGlobal = event->globalPosition().toPoint();
        const QPoint delta = curGlobal - pressPosGlobal_;

        if (!mouseMoved_ &&
            delta.manhattanLength() >= QApplication::startDragDistance()) {
            setMouseMovedInternal(true);
        }

        move(startTopLeftGlobal_ + delta);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void PersistentMenu::mouseReleaseEvent(QMouseEvent* const event) {
    if (dragging_ && event->button() == Qt::LeftButton) {
        dragging_ = false;
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void PersistentMenu::setMouseMovedInternal(const bool moved) noexcept {
    if (mouseMoved_ == moved) {
        return;
    }

    mouseMoved_ = moved;
    emit mouseMovedChanged(mouseMoved_);
}
