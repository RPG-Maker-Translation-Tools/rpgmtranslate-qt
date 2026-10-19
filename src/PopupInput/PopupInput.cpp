#include "PopupInput.hpp"

#include <QKeyEvent>

void PopupInput::keyPressEvent(QKeyEvent* const event) {
    if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
        event->accept();
        return;
    }

    QLineEdit::keyPressEvent(event);
}