#pragma once

#include <QKeyEvent>
#include <QLineEdit>

class LabelInput final : public QLineEdit {
   public:
    explicit LabelInput(QWidget* const parent = nullptr) : QLineEdit(parent) {
        hide();
        setFrame(false);
    };

   protected:
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

   private:
    QString initialText;
};