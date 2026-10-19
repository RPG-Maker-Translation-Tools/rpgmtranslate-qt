#pragma once

#include "Aliases.hpp"

#include <QDateTime>
#include <QDialog>
#include <QDirListing>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class BackupSelectDialog final : public QDialog {
    Q_OBJECT

   public:
    explicit BackupSelectDialog(const QString& backupPath, QWidget* const parent = nullptr) :
        QDialog(parent),
        list(new QListWidget(this)),
        bottomLabel(new QLabel(this)),
        acceptButton(new QPushButton(tr("Load"), this)) {
        // Closing deletes it after its signals are done, so a slot can't delete it from inside its own emission
        setAttribute(Qt::WA_DeleteOnClose);

        auto* const layout = new QVBoxLayout(this);
        auto* const topLabel = new QLabel(
            tr(
                "Once you click load, your current progress will be LOST! All current changes will be overwritten by a backup and the project will be reloaded."
            ),
            this
        );

        layout->addWidget(topLabel);
        layout->addWidget(list);
        layout->addWidget(bottomLabel);
        layout->addWidget(acceptButton);

        acceptButton->setEnabled(false);

        struct Backup {
            QString name;
            QString path;
            QDateTime created;
        };

        vector<Backup> backups;

        for (const auto& entry : QDirListing(backupPath, { u"*.tar.xz"_s }, QDirListing::IteratorFlag::FilesOnly)) {
            QString name = entry.fileName();
            QDateTime created =
                QDateTime::fromString(QStringView(name).chopped(sizeof(".tar.xz") - 1), u"dd-MM-yyyy_hh-mm-ss");

            backups.emplace_back(std::move(name), entry.filePath(), std::move(created));
        }

        // The names start with the day, so the file system order is not the time order: newest first, and names that
        // are not dates after them
        ranges::sort(backups, [](const Backup& lhs, const Backup& rhs) -> bool {
            if (lhs.created.isValid() != rhs.created.isValid()) {
                return lhs.created.isValid();
            }

            return lhs.created.isValid() ? lhs.created > rhs.created : lhs.name < rhs.name;
        });

        for (const auto& backup : backups) {
            auto* const item = new QListWidgetItem(backup.name, list);
            item->setData(Qt::UserRole, backup.path);
            item->setData(Qt::UserRole + 1, backup.created);
        }

        connect(
            list,
            &QListWidget::currentItemChanged,
            this,
            [this](
                QListWidgetItem* const current,
                QListWidgetItem* const /* previous */
            ) -> void {
            if (current == nullptr) {
                acceptButton->setEnabled(false);
                return;
            }

            const QDateTime created = current->data(Qt::UserRole + 1).toDateTime();

            bottomLabel->setText(
                created.isValid() ? tr("This backup was created %1.").arg(created.toString(Qt::TextDate))
                                  : tr("This backup is %1.").arg(current->text())
            );

            acceptButton->setEnabled(true);
        }
        );

        // Not `pressed`: the load starts on release, so the button can be slid off
        connect(acceptButton, &QPushButton::clicked, this, [this] -> void { accept(); });

        show();
    }

    [[nodiscard]] auto backupPath() const -> QString { return list->currentItem()->data(Qt::UserRole).toString(); };

   private:
    QListWidget* const list;
    QLabel* const bottomLabel;
    QPushButton* const acceptButton;
};
