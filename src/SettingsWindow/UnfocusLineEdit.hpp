#pragma once

#include <QLineEdit>

class UnfocusLineEdit : public QLineEdit {
    Q_OBJECT

   public:
    using QLineEdit::QLineEdit;

    void setUnfocusSignal(const bool enabled) { unfocusSignalEnabled = enabled; }

    [[nodiscard]] auto isUnfocusSignalEnabled() const -> bool { return unfocusSignalEnabled; }

   protected:
    void focusOutEvent(QFocusEvent* const event) override {
        if (unfocusSignalEnabled) {
            emit editingFinished();
        }

        QLineEdit::focusOutEvent(event);
    };

    bool unfocusSignalEnabled = false;
};
