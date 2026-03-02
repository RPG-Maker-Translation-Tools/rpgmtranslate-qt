#include "FileSelectMenu.hpp"

#include "PersistentMenu.hpp"
#include "ui_FileSelectMenu.h"

#include <QCheckBox>
#include <QMouseEvent>

FileSelectMenu::FileSelectMenu(QWidget* const parent) :
    PersistentMenu(parent, Qt::FramelessWindowHint),
    ui(setupUi()) {
    installEventFilter(this);

    connect(ui->selectAllButton, &QPushButton::pressed, this, [this] -> void {
        for (auto* const widget : views::drop(ui->gridWidget->children(), 1)) {
            auto* const checkbox = as<QCheckBox*>(widget);
            checkbox->setChecked(true);
        }
    });

    connect(ui->deselectAllButton, &QPushButton::pressed, this, [this] -> void {
        for (auto* const widget : views::drop(ui->gridWidget->children(), 1)) {
            auto* const checkbox = as<QCheckBox*>(widget);
            checkbox->setChecked(false);
        }
    });
};

FileSelectMenu::~FileSelectMenu() {
    delete ui;
}

auto FileSelectMenu::setupUi() -> Ui::FileSelectMenu* {
    auto* const ui_ = new Ui::FileSelectMenu();
    ui_->setupUi(this);
    return ui_;
};

void FileSelectMenu::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    PersistentMenu::changeEvent(event);
};

void FileSelectMenu::addFile(const QString& filename, const bool checked) {
    auto* const checkbox = new QCheckBox(ui->gridWidget);
    checkbox->setChecked(checked);
    checkbox->setText(filename);
    checkbox->installEventFilter(this);
    checkbox->setFocusPolicy(Qt::NoFocus);

    const u16 index = ui->gridLayout->count();
    const u16 row = index / 3;
    const u8 col = index % 3;

    ui->gridLayout->addWidget(checkbox, row, col);
}

void FileSelectMenu::clear() {
    while (const auto* const item = ui->gridLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

auto FileSelectMenu::selected(const bool skipped) -> Selected {
    Selected selected;

    for (const u16 idx : range<u16>(0, ui->gridLayout->count())) {
        const auto* const checkbox =
            as<const QCheckBox*>(ui->gridLayout->itemAt(idx)->widget());

        if (checkbox->isChecked() ^ skipped) {
            const QString file = checkbox->text();

            if (file == "actors"_L1) {
                selected.flags |= FileFlags::Actors;
            } else if (file == "armors"_L1) {
                selected.flags |= FileFlags::Armors;
            } else if (file == "classes"_L1) {
                selected.flags |= FileFlags::Classes;
            } else if (file == "commonevents"_L1) {
                selected.flags |= FileFlags::CommonEvents;
            } else if (file == "enemies"_L1) {
                selected.flags |= FileFlags::Enemies;
            } else if (file == "items"_L1) {
                selected.flags |= FileFlags::Items;
            } else if (file == "skills"_L1) {
                selected.flags |= FileFlags::Skills;
            } else if (file == "states"_L1) {
                selected.flags |= FileFlags::States;
            } else if (file == "troops"_L1) {
                selected.flags |= FileFlags::Troops;
            } else if (file == "weapons"_L1) {
                selected.flags |= FileFlags::Weapons;
            } else if (file == "system"_L1) {
                selected.flags |= FileFlags::System;
            } else if (file == "scripts"_L1) {
                selected.flags |= FileFlags::Scripts;
            } else if (file.startsWith("map"_L1)) {
                const u16 index = QStringView(file).sliced(3).toUInt();
                selected.validIndices[index] = true;
                selected.mapIndices[selected.mapCount++] = true;
            }
        }
    }

    return selected;
}

auto FileSelectMenu::selectedCount() const -> u16 {
    u16 count = 0;

    for (const u16 idx : range<u16>(0, ui->gridLayout->count())) {
        const auto* const checkbox =
            as<const QCheckBox*>(ui->gridLayout->itemAt(idx)->widget());

        if (checkbox->isChecked()) {
            count++;
        }
    }

    return count;
}

auto FileSelectMenu::empty() const -> bool {
    return ui->gridLayout->count() == 0;
}

auto FileSelectMenu::eventFilter(QObject* const obj, QEvent* const event)
    -> bool {
    auto* const checkbox = qobject_cast<QCheckBox*>(obj);

    if (checkbox != nullptr) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                const auto* const mouseEvent = as<const QMouseEvent*>(event);

                if (mouseEvent->button() == Qt::LeftButton) {
                    leftButtonDown = true;
                    dragTouched.clear();
                    dragTouched.insert(checkbox);
                    checkbox->setChecked(!checkbox->isChecked());
                    return true;
                }
                break;
            }

            default:
                break;
        }
    }

    if (obj == this && event->type() == QEvent::MouseMove && leftButtonDown) {
        const auto* const mouseEvent = as<const QMouseEvent*>(event);
        QWidget* const widgetUnderMouse = childAt(mouseEvent->pos());
        auto* const hoveredCheckbox =
            qobject_cast<QCheckBox*>(widgetUnderMouse);

        if (hoveredCheckbox != nullptr &&
            !dragTouched.contains(hoveredCheckbox)) {
            dragTouched.insert(hoveredCheckbox);
            hoveredCheckbox->setChecked(!hoveredCheckbox->isChecked());
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        const auto* const mouseEvent = as<const QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            leftButtonDown = false;
            dragTouched.clear();
        }
    }

    return PersistentMenu::eventFilter(obj, event);
}