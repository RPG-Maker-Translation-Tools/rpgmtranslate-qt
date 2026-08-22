#pragma once

#include "Aliases.hpp"
#include "rpgmtranslate_rs.h"

#include <QObject>

class FFILogger final : public QObject {
    Q_OBJECT

   public:
    static auto instance() -> FFILogger&;
    static void rustLogCallback(u8 level, FFIString str);

   signals:
    void logReceived(u8 level, const QString& message);

   private:
    explicit FFILogger(QObject* parent = nullptr);

    void dispatchLog(u8 level, const QString& message);
};
