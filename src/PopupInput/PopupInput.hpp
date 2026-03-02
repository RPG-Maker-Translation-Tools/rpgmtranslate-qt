#pragma once

#include <QLineEdit>

class PopupInput final : public QLineEdit {
   public:
    using QLineEdit::QLineEdit;

   protected:
    void keyPressEvent(QKeyEvent* event) override;
};