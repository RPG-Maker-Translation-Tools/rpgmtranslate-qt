#include "FFILogger.hpp"

FFILogger::FFILogger(QObject* const parent) : QObject(parent) {}

auto FFILogger::instance() -> FFILogger& {
    static FFILogger bridge;
    return bridge;
}

void FFILogger::rustLogCallback(const u8 level, const FFIString str) {
    QString message = QString::fromUtf8(str.ptr, isize(str.len));

    QMetaObject::invokeMethod(
        &instance(),
        [level, msg = std::move(message)] -> void {
        instance().dispatchLog(level, msg);
    },
        Qt::QueuedConnection
    );
}

void FFILogger::dispatchLog(const u8 level, const QString& message) {
    emit logReceived(level, message);
}