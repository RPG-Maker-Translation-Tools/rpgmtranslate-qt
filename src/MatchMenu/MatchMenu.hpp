#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"
#include "MatchTable.hpp"

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

    void init(QPushButton* clearButton, MatchTable* matchTable);
    void clear();

   private:
    QPushButton* clearButton;
    MatchTable* matchTable;
};