#include "LabelInput.hpp"

void LabelInput::keyPressEvent(QKeyEvent* const event) {
    if (event->key() == Qt::Key_Escape) {
        setText(initialText);
        initialText = QString();
        clearFocus();
        event->accept();
        return;
    }

    QLineEdit::keyPressEvent(event);
}

void LabelInput::focusInEvent(QFocusEvent* const event) {
    initialText = text();
    QLineEdit::focusInEvent(event);
}
