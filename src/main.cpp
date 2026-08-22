#include "MainWindow.hpp"

#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QLoggingCategory>
#include <QMutex>
#include <QStandardPaths>
#include <QtEnvironmentVariables>
#include <print>

namespace {
QFile logFile;
QTextStream logStream;

// Worker threads (QtConcurrent pools) log too, and the handler is invoked on whichever thread logged.
QMutex logMutex;

[[nodiscard]] auto levelToString(const QtMsgType type) -> QL1SV {
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

[[nodiscard]] auto levelToColor(const QtMsgType type) -> const char* {
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

[[nodiscard]] auto shortFile(const char* const file) -> const char* {
    if (file == nullptr) {
        return "";
    }

    static constexpr const char* marker = "/src/";
    const char* const pos = strstr(file, marker);

    if (pos != nullptr) {
        return pos + strlen(marker);
    }

    if (const char* const slash = strrchr(file, '/')) {
        return slash + 1;
    }

#ifdef Q_OS_WINDOWS
    if (const char* const bslash = strrchr(file, '\\')) {
        return bslash + 1;
    }
#endif

    return file;
}

void messageHandler(const QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    const QString formatted = u"[%1] %2:%3 (%4): %5"_qsv.arg(
        svtostr(levelToString(type)),
        svtostr(QUtf8SV(shortFile(ctx.file))),
        itos(ctx.line).qsv(),
        svtostr(QUtf8SV(ctx.function)),
        msg
    );

    {
        const QMutexLocker locker(&logMutex);

        std::println(stdout, "{}{}\x1b[0m", levelToColor(type), formatted.toLocal8Bit().constData());
        fflush(stdout);

        if (logStream.device() != nullptr) {
            logStream << formatted << '\n';
            logStream.flush();
        }
    }

    if (type == QtFatalMsg) {
        abort();
    }
}
}  // namespace

auto main(i32 argCount, char* args[]) -> i32 {
    const auto app = QApplication(argCount, args);

    const QString lockFilePath = QDir::tempPath() + u"/rpgmtranslate.lock";
    auto lockFile = QLockFile(lockFilePath);

    if (!lockFile.tryLock()) {
        return 0;
    }

    // Use system default locale
    std::locale::global(std::locale(""));

    qApp->setOrganizationName(u"rpg-maker-translation-tools"_s);
    qApp->setApplicationName(u"rpgmtranslate"_s);
    qApp->setWindowIcon(QIcon(u":/icons/rpgmtranslate-logo.png"_s));

    qApp->connect(&app, &QApplication::aboutToQuit, &app, [&lockFile, &lockFilePath] -> void {
        lockFile.unlock();
        QFile::remove(lockFilePath);
    });

    QString dataDir = qEnvironmentVariable("RPGMTRANSLATE_DATA_DIR");
    bool defaultDataDir = false;

    if (dataDir.isEmpty()) {
        dataDir = qApp->applicationDirPath();
        defaultDataDir = true;
    }

    logFile.setFileName(dataDir % u"/rpgmtranslate.log"_qsv);

    if (!logFile.open(QFile::WriteOnly | QFile::Truncate)) {
        const auto clo = [&] -> i32 {
            if (defaultDataDir) {
                std::println(
                    cerr,
                    "{} is not writable. Seeking other directory for RPGMTranslate data.",
                    dataDir.toStdString()
                );

                dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
                logFile.setFileName(dataDir + u"/rpgmtranslate.log");

                if (logFile.open(QFile::WriteOnly | QFile::Truncate)) {
                    return 0;
                }
            }

            std::println(
                cerr,
                "{} is not writable. Nowhere to place the application data, aborting.",
                dataDir.toStdString()
            );
            return 1;
        };

        const i32 res = clo();

        if (res == 1) {
            return res;
        }
    }

    qSetMessagePattern(u"%{file}:%{line}: %{message}"_s);
    logStream.setDevice(&logFile);
    qInstallMessageHandler(messageHandler);

    qApp->setProperty("data-location", dataDir);
    qInfo() << u"Data directory is set to:"_qsv << dataDir;

    auto window = MainWindow();
    window.showMaximized();

    return qApp->exec();
}
