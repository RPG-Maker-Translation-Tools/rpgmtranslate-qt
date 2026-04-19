#include "MainWindow.hpp"

#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QtEnvironmentVariables>
#include <print>

static QFile logFile;
static QTextStream logStream;

[[nodiscard]] static auto levelToString(const QtMsgType type) -> QL1SV {
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

[[nodiscard]] static auto levelToColor(const QtMsgType type) -> cstr {
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

[[nodiscard]] static auto shortFile(const cstr file) -> cstr {
    if (file == nullptr) {
        return "";
    }

    constexpr cstr marker = "/src/";
    const cstr pos = strstr(file, marker);

    if (pos != nullptr) {
        return pos + strlen(marker);
    }

    if (const cstr slash = strrchr(file, '/')) {
        return slash + 1;
    }

#ifdef Q_OS_WINDOWS
    if (const cstr bslash = strrchr(file, '\\')) {
        return bslash + 1;
    }
#endif

    return file;
}

static void messageHandler(
    const QtMsgType type,
    const QMessageLogContext& ctx,
    const QString& msg
) {
    const QString formatted = "[%1] %2:%3 (%4): %5"_L1.arg(
        levelToString(type),
        QUtf8SV(shortFile(ctx.file)),
        QL1SV(itos(ctx.line).data()),
        QUtf8SV(ctx.function),
        msg
    );

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

    qApp->connect(
        &app,
        &QApplication::aboutToQuit,
        &app,
        [&lockFile, &lockFilePath] -> void {
        lockFile.unlock();
        QFile::remove(lockFilePath);
    }
    );

    qSetMessagePattern("%{file}:%{line}: %{message}"_L1);

    logFile.setFileName(qApp->applicationDirPath() + u"/rpgmtranslate.log");
    if (!logFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Append)) {
        std::println(
            stderr,
            "{} is not writable. Seeking other directory for RPGMTranslate data.",
            qApp->applicationDirPath().toStdString()
        );

        const QString dir = qEnvironmentVariable("RPGMTRANSLATE_DATA_DIR");

        if (dir.isEmpty()) {
            const QString standardDataLocation =
                QStandardPaths::writableLocation(
                    QStandardPaths::AppLocalDataLocation
                );

            const string dirStd = standardDataLocation.toStdString();

            std::println(
                stdout,
                "RPGMTRANSLATE_DATA_DIR is not set. Falling back to {} to store RPGMTranslate data.",
                dirStd
            );

            logFile.setFileName(standardDataLocation + u"/rpgmtranslate.log");

            if (!logFile.open(
                    QFile::WriteOnly | QFile::Truncate | QFile::Append
                )) {
                std::println(
                    stderr,
                    "{} is not writable. Nowhere to place the application data, aborting.",
                    dirStd
                );

                return 1;
            }

            qApp->setProperty("data-location", standardDataLocation);
        } else {
            const string dirStd = dir.toStdString();

            std::println(
                stdout,
                "Found RPGMTRANSLATE_DATA_DIR: {}. Using it to store RPGMTranslate data.",
                dirStd
            );

            logFile.setFileName(dir + u"/rpgmtranslate.log");

            if (!logFile.open(
                    QFile::WriteOnly | QFile::Truncate | QFile::Append
                )) {
                std::println(
                    stderr,
                    "{} is not writable. Nowhere to place the application data, aborting.",
                    dirStd
                );

                return 1;
            }

            qApp->setProperty("data-location", dir);
        }
    }

    qApp->setProperty("data-location", qApp->applicationDirPath());

    logStream.setDevice(&logFile);
    qInstallMessageHandler(messageHandler);

    auto window = MainWindow();
    window.showMaximized();

    return qApp->exec();
}