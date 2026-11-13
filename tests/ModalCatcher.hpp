#pragma once

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <functional>

// Answers the modal box that a call is about to open, keeping what it said. A call that opens none leaves `text` empty.
// `answer` is the button to press, or none to accept the box as it is.
//
// A file dialog is answered with `file` (cancelled while that is empty), a color dialog with `color` (cancelled while
// that is invalid). Their titles are collected in `dialogs`. Under the offscreen platform they are Qt's own widgets.
class ModalCatcher {
   public:
    explicit ModalCatcher(const QMessageBox::StandardButton button = QMessageBox::NoButton) : answer(button) {
        // A native file dialog (Windows) is not a widget the poll can answer
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        poll.setInterval(5);
        QObject::connect(&poll, &QTimer::timeout, &poll, [this] {
            QWidget* const modal = QApplication::activeModalWidget();

            if (auto* const fileDialog = qobject_cast<QFileDialog*>(modal)) {
                dialogs.append(fileDialog->windowTitle());

                if (file.isEmpty()) {
                    fileDialog->reject();
                } else {
                    fileDialog->selectFile(file);
                    // `QFileDialog` makes it protected; the virtual call still reaches its own
                    static_cast<QDialog*>(fileDialog)->accept();
                }
            } else if (auto* const colorDialog = qobject_cast<QColorDialog*>(modal)) {
                dialogs.append(colorDialog->windowTitle());

                if (color.isValid()) {
                    colorDialog->setCurrentColor(color);
                    colorDialog->accept();
                } else {
                    colorDialog->reject();
                }
            } else if (auto* const confirm = askWithDetailsOf(modal)) {
                // The question with a scrollable list under it (`askWithDetails`): Yes only when the box is told to
                // press it
                const auto* const view = confirm->findChild<QPlainTextEdit*>();
                seen.append(view->toPlainText());
                dialogs.append(confirm->windowTitle());

                if (text.isEmpty()) {
                    text = view->toPlainText();
                    title = confirm->windowTitle();
                }

                auto* const buttons = confirm->findChild<QDialogButtonBox*>();
                buttons->button(answer == QMessageBox::Yes ? QDialogButtonBox::Yes : QDialogButtonBox::No)->click();
            } else if (auto* const box = qobject_cast<QMessageBox*>(modal)) {
                seen.append(box->text());

                if (text.isEmpty()) {
                    text = box->text();
                    title = box->windowTitle();
                }

                if (beforeAnswer) {
                    beforeAnswer();
                }

                if (tickBox && box->checkBox() != nullptr) {
                    box->checkBox()->setChecked(true);
                }

                QAbstractButton* named = nullptr;

                for (QAbstractButton* const candidate : box->buttons()) {
                    if (!press.isEmpty() && candidate->text() == press) {
                        named = candidate;
                    }
                }

                if (named != nullptr) {
                    named->click();
                } else if (answer != QMessageBox::NoButton && box->button(answer) != nullptr) {
                    box->button(answer)->click();
                } else {
                    box->accept();
                }
            }
        });
        poll.start();

        // A box that is never answered would hang the test
        QTimer::singleShot(20000, &poll, [this] {
            if (auto* const widget = QApplication::activeModalWidget()) {
                text = QStringLiteral("<unanswered: ") + QString::fromLatin1(widget->metaObject()->className()) + QStringLiteral(" ") + widget->windowTitle() + u'>';
                widget->close();
            }
        });
    }

    // What the boxes are answered with, which a test can change as the code under test goes on
    QMessageBox::StandardButton answer;
    // `text` holds what a box says, and the details a question with details lists
    QString text;
    // Every box answered, the first one included
    QStringList seen;
    QString title;

    // What the file and color dialogs are answered with, and the titles of the ones that were answered
    QString file;
    QColor color;
    QStringList dialogs;

    // A box with buttons of its own is answered by the one with this text; the check box it has is ticked first
    QString press;
    bool tickBox = false;
    // Run when a message box has come up and not yet been answered, to change what the code that opened it will find
    std::function<void()> beforeAnswer;

   private:
    // The dialog of `askWithDetails`, told by its read-only list and its Yes and No buttons: the other dialogs of the
    // application have buttons of their own
    [[nodiscard]] static auto askWithDetailsOf(QWidget* const widget) -> QDialog* {
        auto* const dialog = qobject_cast<QDialog*>(widget);

        if (dialog == nullptr || qobject_cast<QMessageBox*>(dialog) != nullptr) {
            return nullptr;
        }

        const auto* const buttons = dialog->findChild<QDialogButtonBox*>();
        const auto* const view = dialog->findChild<QPlainTextEdit*>();
        const auto yesNo = QDialogButtonBox::Yes | QDialogButtonBox::No;

        return buttons != nullptr && view != nullptr && view->isReadOnly() && buttons->standardButtons() == yesNo
                   ? dialog
                   : nullptr;
    }

    QTimer poll;
};
