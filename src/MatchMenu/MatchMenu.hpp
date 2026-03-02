#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDockWidget>

class MatchMenu final : public QDockWidget {
    Q_OBJECT

   public:
    explicit MatchMenu(QWidget* parent = nullptr);

    void appendMatch(
        const QString& filename,
        u32 lineNumber,
        const QString& termSource,
        const QString& termTranslation,
        QStringView source,
        QStringView translation,
        ByteBuffer matches
    );

    void init(QPushButton* clearButton, QTableWidget* matchTable);
    void clear();

   private:
    QPushButton* clearButton;
    QTableWidget* matchTable;
};