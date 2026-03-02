#include "PopupInput.hpp"

#include <QKeyEvent>

void PopupInput::keyPressEvent(QKeyEvent* const event) {
    if (event->key() == Qt::Key_Escape) {
        emit inputRejected();
        event->accept();
        return;
    }

    QLineEdit::keyPressEvent(event);
}