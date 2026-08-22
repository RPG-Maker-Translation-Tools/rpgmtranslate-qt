#pragma once

#include "Aliases.hpp"
#include "UnfocusLineEdit.hpp"

#include <QAction>
#include <QIcon>
#include <QLineEdit>

class PasswordLineEdit final : public UnfocusLineEdit {
    Q_OBJECT

   public:
    explicit PasswordLineEdit(QWidget* const parent = nullptr) :
        UnfocusLineEdit(parent),
        toggleAction(new QAction(this)) {
        setEchoMode(QLineEdit::Password);
        setInputMethodHints(Qt::ImhHiddenText | Qt::ImhNoPredictiveText);

        showIcon = QIcon(u":/icons/visibility_off.svg"_s);
        hideIcon = QIcon(u":/icons/visibility.svg"_s);

        toggleAction->setIcon(showIcon);
        toggleAction->setCheckable(true);

        addAction(toggleAction, QLineEdit::TrailingPosition);

        connect(toggleAction, &QAction::toggled, this, &PasswordLineEdit::onToggled);
    }

   private:
    void onToggled(const bool checked) {
        if (checked) {
            setEchoMode(QLineEdit::Normal);
            toggleAction->setIcon(hideIcon);
        } else {
            setEchoMode(QLineEdit::Password);
            toggleAction->setIcon(showIcon);
        }
    }

    QAction* toggleAction;
    QIcon showIcon;
    QIcon hideIcon;
};
