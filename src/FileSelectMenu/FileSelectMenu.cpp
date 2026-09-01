#include "FileSelectMenu.hpp"

#include "PersistentMenu.hpp"
#include "TabListModel.hpp"
#include "ui_FileSelectMenu.h"

#include <QCheckBox>
#include <QMouseEvent>

FileSelectMenu::FileSelectMenu(QWidget* const parent) : PersistentMenu(parent), ui(setupUi()) {
    installEventFilter(this);

    connect(ui->selectAllButton, &QPushButton::pressed, this, [this] -> void {
        for (auto* const widget : views::drop(ui->gridWidget->children(), 1)) {
            auto* const checkbox = scast<QCheckBox*>(widget);
            checkbox->setChecked(true);
        }
    });

    connect(ui->deselectAllButton, &QPushButton::pressed, this, [this] -> void {
        for (auto* const widget : views::drop(ui->gridWidget->children(), 1)) {
            auto* const checkbox = scast<QCheckBox*>(widget);
            checkbox->setChecked(false);
        }
    });
};

FileSelectMenu::~FileSelectMenu() {
    delete ui;
}

void FileSelectMenu::init(const vector<TabListItem>& files) {
    for (const auto& file : files) {
        addFile(file.name);
    }
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

    const i32 index = ui->gridLayout->count();
    const i32 row = index / 3;
    const i32 col = index % 3;

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

    for (i32 idx = 0; idx < ui->gridLayout->count(); idx++) {
        const auto* const checkbox = scast<const QCheckBox*>(ui->gridLayout->itemAt(idx)->widget());

        if (checkbox->isChecked() ^ skipped) {
            const QString file = checkbox->text();

            if (file == u"actors"_qsv) {
                selected.flags |= FileFlags_Actors;
            } else if (file == u"armors"_qsv) {
                selected.flags |= FileFlags_Armors;
            } else if (file == u"classes"_qsv) {
                selected.flags |= FileFlags_Classes;
            } else if (file == u"commonevents"_qsv) {
                selected.flags |= FileFlags_CommonEvents;
            } else if (file == u"enemies"_qsv) {
                selected.flags |= FileFlags_Enemies;
            } else if (file == u"items"_qsv) {
                selected.flags |= FileFlags_Items;
            } else if (file == u"skills"_qsv) {
                selected.flags |= FileFlags_Skills;
            } else if (file == u"states"_qsv) {
                selected.flags |= FileFlags_States;
            } else if (file == u"troops"_qsv) {
                selected.flags |= FileFlags_Troops;
            } else if (file == u"weapons"_qsv) {
                selected.flags |= FileFlags_Weapons;
            } else if (file == u"system"_qsv) {
                selected.flags |= FileFlags_System;
            } else if (file == u"scripts"_qsv) {
                selected.flags |= FileFlags_Scripts;
            } else if (file == u"terms"_qsv) {
                selected.flags |= FileFlags_Database;
            } else if (file.startsWith(u"map"_qsv)) {
                const u16 index = stoa<u16>(QStringView(file).sliced(3));
                selected.validIndices[index] = true;
                selected.mapIndices[selected.mapCount++] = true;
            }
        }
    }

    return selected;
}

auto FileSelectMenu::selectedCount() const -> u16 {
    u16 count = 0;

    for (i32 idx = 0; idx < ui->gridLayout->count(); idx++) {
        const auto* const checkbox = scast<const QCheckBox*>(ui->gridLayout->itemAt(idx)->widget());

        if (checkbox->isChecked()) {
            count++;
        }
    }

    return count;
}

auto FileSelectMenu::empty() const -> bool {
    return ui->gridLayout->count() == 0;
}

auto FileSelectMenu::eventFilter(QObject* const obj, QEvent* const event) -> bool {
    auto* const checkbox = qobject_cast<QCheckBox*>(obj);

    if (checkbox != nullptr) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                const auto* const mouseEvent = scast<const QMouseEvent*>(event);

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
        const auto* const mouseEvent = scast<const QMouseEvent*>(event);
        QWidget* const widgetUnderMouse = childAt(mouseEvent->pos());
        auto* const hoveredCheckbox = qobject_cast<QCheckBox*>(widgetUnderMouse);

        if (hoveredCheckbox != nullptr && !dragTouched.contains(hoveredCheckbox)) {
            dragTouched.insert(hoveredCheckbox);
            hoveredCheckbox->setChecked(!hoveredCheckbox->isChecked());
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        const auto* const mouseEvent = scast<const QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            leftButtonDown = false;
            dragTouched.clear();
        }
    }

    return PersistentMenu::eventFilter(obj, event);
}