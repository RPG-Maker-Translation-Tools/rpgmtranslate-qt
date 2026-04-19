#pragma once

#include "Aliases.hpp"

#include <QDialog>
#include <QDirListing>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class BackupSelectDialog : public QDialog {
    Q_OBJECT

   public:
    explicit BackupSelectDialog(
        const QString& backupPath,
        QWidget* const parent = nullptr
    ) :
        QDialog(parent),
        layout(new QVBoxLayout(this)),
        topLabel(new QLabel(this)),
        list(new QListWidget(this)),
        bottomLabel(new QLabel(this)),
        acceptButton(new QPushButton(tr("Load"), this)) {
        topLabel->setText(tr(
            "Once you click load, your current progress will be LOST! All current changed will be overwritten by a backup and the project will be reloaded."
        ));

        layout->addWidget(topLabel);
        layout->addWidget(list);
        layout->addWidget(bottomLabel);
        layout->addWidget(acceptButton);

        acceptButton->setEnabled(false);

        auto listing = QDirListing(
            backupPath,
            { u"*.tar.xz"_s },
            QDirListing::IteratorFlag::FilesOnly
        );

        for (const auto& entry : listing) {
            list->addItem(entry.fileName());
            list->item(list->count() - 1)
                ->setData(Qt::UserRole, entry.filePath());
        }

        connect(
            list,
            &QListWidget::currentItemChanged,
            this,
            [this](
                QListWidgetItem* const current,
                QListWidgetItem* const /* previous */
            ) -> void {
            const QString backupName = current->text();

            const auto dateTime = QDateTime::fromString(
                QStringView(backupName)
                    .slice(0, backupName.size() - (sizeof(".tar.xz") - 1)),
                u"dd-MM-yyyy_hh-mm-ss"
            );
            bottomLabel->setText(tr("This backup was created %1.")
                                     .arg(dateTime.toString(Qt::TextDate)));

            acceptButton->setEnabled(true);
        }
        );

        connect(acceptButton, &QPushButton::pressed, this, [this] -> void {
            accept();
        });

        show();
    }

    [[nodiscard]] auto backupPath() -> QString {
        return list->currentItem()->data(Qt::UserRole).toString();
    };

   private:
    QVBoxLayout* const layout;
    QLabel* const topLabel;
    QListWidget* const list;
    QLabel* const bottomLabel;
    QPushButton* const acceptButton;
};