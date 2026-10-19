#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>

// Words and characters of every file of the project, with the totals in the last row.
class StatisticsDialog final : public QDialog {
    Q_OBJECT

   public:
    explicit StatisticsDialog(QWidget* parent = nullptr);

    void fill(const vector<FileStatistics>& files) const;

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void retranslate();

    QTableWidget* const table;
};
