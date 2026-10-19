#pragma once

#include <QLineEdit>

class PopupInput final : public QLineEdit {
    Q_OBJECT

   public:
    using QLineEdit::QLineEdit;

   signals:
    void escapePressed();

   protected:
    void keyPressEvent(QKeyEvent* event) override;
};
