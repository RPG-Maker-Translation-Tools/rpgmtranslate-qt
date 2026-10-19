#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "Task.hpp"

#include <QWidget>

class TaskPanel final : public QWidget {
    Q_OBJECT

   public:
    explicit TaskPanel(QWidget* parent = nullptr);

    void addTask(const TaskToken& task);
    void updateTask(u32 taskId, const QString& filename, u32 done, u32 total);
    void removeTask(u32 taskId);

    [[nodiscard]] auto taskCount() const -> usize { return rows.size(); }

   signals:
    void taskCountChanged(i32 count);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    struct Row {
        QString name;
        QString filename;
        QWidget* widget;
        QLabel* label;
        QProgressBar* progressBar;
        u32 taskId;
        TaskKind kind;
    };

    void updateLabel(const Row& row) const;

    [[nodiscard]] auto findRow(u32 taskId) -> Row*;

    vector<Row> rows;

    QVBoxLayout* const layout;
    QLabel* const placeholder;
};
