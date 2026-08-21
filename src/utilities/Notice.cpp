#include "Notice.hpp"

#include "Utils.hpp"

#include <QCoreApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QStatusBar>

namespace {
constexpr i32 STATUS_MESSAGE_TIMEOUT_MS = 8000;
constexpr i32 MAX_PLACEHOLDER_DIGITS = 2;

// `n` is only consulted for a literal %n left in `pattern` - the translated path never sees one, since
// QCoreApplication::translate() already substitutes %n itself once it's given the numerus argument; this only
// matters for the untranslated form (the English literal used for logging), which bypasses translate() entirely.
[[nodiscard]] auto substitute(const QString& pattern, const QStringList& args, const i32 n) -> QString {
    if (args.isEmpty() && n < 0) {
        return pattern;
    }

    QString out;
    out.reserve(pattern.size() + (args.size() * 16));

    for (isize pos = 0; pos < pattern.size(); pos++) {
        const QChar chr = pattern[pos];

        if (chr != u'%') {
            out += chr;
            continue;
        }

        if (n >= 0 && pos + 1 < pattern.size() && pattern[pos + 1] == u'n') {
            out += itos(n).qsv();
            pos += 1;
            continue;
        }

        isize digitEnd = pos + 1;
        isize index = 0;

        while (digitEnd < pattern.size() && digitEnd - pos <= MAX_PLACEHOLDER_DIGITS && pattern[digitEnd].isDigit()) {
            index = (index * 10) + (pattern[digitEnd].unicode() - u'0');
            ++digitEnd;
        }

        if (index >= 1 && index <= args.size()) {
            out += args[index - 1];
            pos = digitEnd - 1;
        } else {
            out += chr;
        }
    }

    return out;
}

[[nodiscard]] auto messageBoxIcon(const Severity severity) -> QMessageBox::Icon {
    switch (severity) {
        case Severity::Debug:
        case Severity::Info:
            return QMessageBox::Information;
        case Severity::Warning:
            return QMessageBox::Warning;
        case Severity::Critical:
            return QMessageBox::Critical;
    }

    std::unreachable();
}

[[nodiscard]] auto messageBoxTitle(const Severity severity) -> QString {
    switch (severity) {
        case Severity::Debug:
        case Severity::Info:
            return QCoreApplication::translate("Notice", "Information");
        case Severity::Warning:
            return QCoreApplication::translate("Notice", "Warning");
        case Severity::Critical:
            return QCoreApplication::translate("Notice", "Error");
    }

    std::unreachable();
}

[[nodiscard]] auto statusBarOf(QWidget* const parent) -> QStatusBar* {
    if (parent == nullptr) {
        return nullptr;
    }

    auto* const window = qobject_cast<QMainWindow*>(parent->window());
    return window == nullptr ? nullptr : window->statusBar();
}
}  // namespace

Notice::Notice(
    const char* const context,
    const char* const source,
    const Severity severity,
    const Delivery delivery,
    const std::initializer_list<QAnyStringView> args,
    const i32 n,
    const std::source_location location
) :
    context_(context),
    source_(source),
    severity_(severity),
    delivery_(delivery),
    n_(n) {
    args_.reserve(scast<isize>(args.size()));

    for (const QAnyStringView arg : args) {
        args_.emplace_back(arg.toString());
    }

    const auto logger = QMessageLogger(location.file_name(), scast<i32>(location.line()), location.function_name());
    const QString message = english();

    switch (severity_) {
        case Severity::Debug:
            logger.debug().noquote() << message;
            break;
        case Severity::Info:
            logger.info().noquote() << message;
            break;
        case Severity::Warning:
            logger.warning().noquote() << message;
            break;
        case Severity::Critical:
            logger.critical().noquote() << message;
            break;
    }
}

auto Notice::translated() const -> QString {
    return substitute(QCoreApplication::translate(context_, source_, nullptr, n_), args_, n_);
}

auto Notice::english() const -> QString {
    return substitute(QString::fromUtf8(source_), args_, n_);
}

void present(QWidget* const parent, const Notice& notice) {
    switch (notice.delivery()) {
        case Delivery::Inline:
            return;
        case Delivery::Status: {
            if (QStatusBar* const bar = statusBarOf(parent)) {
                bar->showMessage(notice.translated(), STATUS_MESSAGE_TIMEOUT_MS);
                return;
            }

            break;
        }
        case Delivery::Modal:
            break;
    }

    const Severity severity = notice.severity();
    auto messageBox = QMessageBox(parent);
    messageBox.setIcon(messageBoxIcon(severity));
    messageBox.setWindowTitle(messageBoxTitle(severity));
    messageBox.setText(notice.translated());
    messageBox.exec();
}
