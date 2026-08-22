#include "TaskPanel.hpp"

#include "Utils.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr i32 PANEL_MIN_WIDTH = 320;
constexpr i32 PANEL_MARGIN = 8;
constexpr i32 PROGRESS_BAR_MAX_HEIGHT = 12;
constexpr i32 ABORT_BUTTON_SIZE = 24;
}  // namespace

TaskPanel::TaskPanel(QWidget* const parent) :
    QWidget(parent, Qt::Popup),
    layout(new QVBoxLayout(this)),
    placeholder(new QLabel(tr("No tasks are running."), this)) {
    setAttribute(Qt::WA_WindowPropagation);
    setMinimumWidth(PANEL_MIN_WIDTH);

    layout->setContentsMargins(PANEL_MARGIN, PANEL_MARGIN, PANEL_MARGIN, PANEL_MARGIN);
    layout->setSpacing(4);
    layout->addWidget(placeholder);
}

auto TaskPanel::findRow(const u32 taskId) -> Row* {
    const auto row = ranges::find(rows, taskId, &Row::taskId);
    return row == rows.end() ? nullptr : &*row;
}

void TaskPanel::addTask(const TaskToken& task) {
    placeholder->hide();

    auto* const widget = new QWidget(this);
    auto* const widgetLayout = new QHBoxLayout(widget);
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(4);

    const QString name = taskName(task->kind());

    auto* const label = new QLabel(name, widget);
    auto* const progressBar = new QProgressBar(widget);
    progressBar->setRange(0, 0);
    progressBar->setFormat(u"%p (%v/%m)"_s);
    progressBar->setTextVisible(true);
    progressBar->setMaximumHeight(PROGRESS_BAR_MAX_HEIGHT);

    widgetLayout->addWidget(label, 1);
    widgetLayout->addWidget(progressBar, 1);

    if (taskInterruptible(task->kind())) {
        auto* const abortButton = new QPushButton(QIcon(u":/icons/close.svg"_s), QString(), widget);
        abortButton->setToolTip(tr("Abort"));
        abortButton->setFixedSize(ABORT_BUTTON_SIZE, ABORT_BUTTON_SIZE);

        connect(abortButton, &QPushButton::pressed, this, [task, abortButton] -> void {
            task->interrupt();
            abortButton->setEnabled(false);
        });

        widgetLayout->addWidget(abortButton);
    }

    layout->addWidget(widget);
    rows.emplace_back(task->id(), name, widget, label, progressBar);

    emit taskCountChanged(rows.size());
}

void TaskPanel::updateTask(const u32 taskId, const QString& filename, const u32 done, const u32 total) {
    Row* const row = findRow(taskId);

    if (row == nullptr) {
        return;
    }

    row->label->setText(u"%1: %2"_qsv.arg(row->name, filename));
    row->progressBar->setRange(0, scast<i32>(total));
    row->progressBar->setValue(scast<i32>(done));
}

void TaskPanel::removeTask(const u32 taskId) {
    const Row* const row = findRow(taskId);

    if (row == nullptr) {
        return;
    }

    delete row->widget;
    rows.erase(rows.begin() + (row - rows.data()));

    if (rows.empty()) {
        placeholder->show();
        hide();
    }

    adjustSize();
    emit taskCountChanged(rows.size());
}
