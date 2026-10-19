#include "ScrollableMessageBox.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace {
constexpr i32 DIALOG_WIDTH = 520;
constexpr i32 DIALOG_HEIGHT = 360;
}  // namespace

auto askWithDetails(QWidget* const parent, const QString& title, const QString& text, const QString& details) -> bool {
    auto dialog = QDialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(DIALOG_WIDTH, DIALOG_HEIGHT);

    auto* const iconLabel = new QLabel(&dialog);
    const i32 iconSize = dialog.style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
    iconLabel->setPixmap(
        dialog.style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(QSize(iconSize, iconSize))
    );
    iconLabel->setAlignment(Qt::AlignTop);

    auto* const textLabel = new QLabel(text, &dialog);
    textLabel->setWordWrap(true);

    auto* const detailsView = new QPlainTextEdit(details, &dialog);
    detailsView->setReadOnly(true);
    detailsView->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No, &dialog);
    buttons->button(QDialogButtonBox::No)->setDefault(true);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto* const header = new QHBoxLayout();
    header->addWidget(iconLabel);
    header->addWidget(textLabel, 1);

    auto* const layout = new QVBoxLayout(&dialog);
    layout->addLayout(header);
    layout->addWidget(detailsView, 1);
    layout->addWidget(buttons);

    return dialog.exec() == QDialog::Accepted;
}
