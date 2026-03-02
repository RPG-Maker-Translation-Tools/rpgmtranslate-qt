#include "MainWindow.hpp"

#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QLoggingCategory>
#include <print>

static QFile logFile;
static QTextStream logStream;

static auto levelToString(const QtMsgType type) -> QLatin1StringView {
    switch (type) {
        case QtDebugMsg:
            return "DEBUG"_L1;
        case QtInfoMsg:
            return "INFO"_L1;
        case QtWarningMsg:
            return "WARN"_L1;
        case QtCriticalMsg:
            return "ERROR"_L1;
        case QtFatalMsg:
            return "FATAL"_L1;
    }

    return "LOG"_L1;
}

static auto levelToColor(QtMsgType type) -> cstr {
    switch (type) {
        case QtDebugMsg:
            return "\033[36m";
        case QtInfoMsg:
            return "\033[32m";
        case QtWarningMsg:
            return "\033[33m";
        case QtCriticalMsg:
            return "\033[31m";
        case QtFatalMsg:
            return "\033[41m";
    }
    return "\033[0m";
}

static void messageHandler(
    const QtMsgType type,
    const QMessageLogContext& ctx,
    const QString& msg
) {
    const QLatin1StringView level = levelToString(type);
    const QString formatted =
        "[%1] %2:%3: %4"_L1.arg(level).arg(ctx.file).arg(ctx.line).arg(msg);

    std::println(
        stdout,
        "{}{}\x1b[0m",
        levelToColor(type),
        formatted.toLocal8Bit().constData()
    );
    fflush(stdout);

    if (logStream.device() != nullptr) {
        logStream << formatted << '\n';
        logStream.flush();
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

auto main(i32 argCount, char* args[]) -> i32 {
    const auto app = QApplication(argCount, args);

    const QString lockFilePath = QDir::tempPath() + u"/rpgmtranslate.lock"_qssv;
    auto lockFile = QLockFile(lockFilePath);

    if (!lockFile.tryLock()) {
        return 0;
    }

    // Use system default locale
    std::locale::global(std::locale(""));

    qApp->setOrganizationName(u"rpg-maker-translation-tools"_s);
    qApp->setApplicationName(u"rpgmtranslate"_s);
    qApp->setWindowIcon(QIcon(u":/icons/rpgmtranslate-logo.png"_s));

    qApp->connect(
        &app,
        &QApplication::aboutToQuit,
        [&lockFile, &lockFilePath] -> void {
        lockFile.unlock();
        QFile::remove(lockFilePath);
    }
    );

    qSetMessagePattern("%{file}:%{line}: %{message}"_L1);

    logFile.setFileName(qApp->applicationDirPath() + "/rpgmtranslate.log"_L1);
    if (!logFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Append)) {
        std::println(stderr, "Failed to open log file");
        return 1;
    }
    logStream.setDevice(&logFile);

    qInstallMessageHandler(messageHandler);

    auto window = MainWindow();
    window.showMaximized();

    return qApp->exec();
}