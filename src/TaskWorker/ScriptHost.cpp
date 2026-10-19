#include "ScriptHost.hpp"

#include "Enums.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
constexpr usize SCRIPT_MEMORY_LIMIT = 64ULL * 1024 * 1024;
constexpr usize SCRIPT_MAX_STACK_SIZE = 1ULL * 1024 * 1024;

constexpr i32 EXEC_DEFAULT_TIMEOUT_MS = 120000;
constexpr i32 EXEC_START_TIMEOUT_MS = 5000;
constexpr i32 EXEC_POLL_MS = 100;

constexpr isize SCRIPT_LOG_LIMIT = 30;
constexpr isize MAX_APPROVED_SCRIPTS = 256;

[[nodiscard]] auto hostState(JSContext* const ctx) -> ScriptRuntime::HostState* {
    return scast<ScriptRuntime::HostState*>(JS_GetContextOpaque(ctx));
}

// A blocking native call must not eat the JS execution budget, otherwise the interrupt handler would
// fire the moment control returns to the script.
void rearmDeadline(JSContext* const ctx) {
    *hostState(ctx)->deadline = QDeadlineTimer(SCRIPT_TIMEOUT);
}

auto throwError(JSContext* const ctx, const QString& message) -> JSValue {
    const QByteArray utf8 = message.toUtf8();
    return JS_ThrowInternalError(ctx, "%s", utf8.constData());
}

auto hostLog(JSContext* const ctx, JSValueConst /* thisVal */, const i32 argc, JSValueConst* const argv) -> JSValue {
    if (argc > 0) {
        const QString message = jsToQString(ctx, argv[0]);
        qInfo().noquote() << u"[script] %1"_qsv.arg(message);

        if (QStringList* const log = hostState(ctx)->log) {
            // Keeps the latest lines: the one that matters is usually the last
            if (log->size() == SCRIPT_LOG_LIMIT) {
                log->removeFirst();
            }

            log->push_back(message);
        }
    }

    return JS_UNDEFINED;
}

auto hostReadFile(JSContext* const ctx, JSValueConst /* thisVal */, const i32 argc, JSValueConst* const argv)
    -> JSValue {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "rpgm.readFile(path)");
    }

    auto file = QFile(jsToQString(ctx, argv[0]));

    if (!file.open(QFile::ReadOnly)) {
        return throwError(ctx, u"Cannot read file: %1"_qsv.arg(file.errorString()));
    }

    const QByteArray content = file.readAll();
    return JS_NewStringLen(ctx, content.constData(), scast<usize>(content.size()));
}

auto hostWriteFile(JSContext* const ctx, JSValueConst /* thisVal */, const i32 argc, JSValueConst* const argv)
    -> JSValue {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "rpgm.writeFile(path, text)");
    }

    // Atomic, so a failed or interrupted write doesn't destroy the file a script was meant to update
    auto file = QSaveFile(jsToQString(ctx, argv[0]));
    file.setDirectWriteFallback(true);
    const QByteArray content = jsToQString(ctx, argv[1]).toUtf8();

    if (!file.open(QFile::WriteOnly) || file.write(content) != content.size() || !file.commit()) {
        return throwError(ctx, u"Cannot write file: %1"_qsv.arg(file.errorString()));
    }

    return JS_UNDEFINED;
}

// rpgm.exec(program, args?, { stdin?, cwd?, timeoutMs? }) -> { code, stdout, stderr }
// Throws if the process cannot start, is cancelled, or exceeds `timeoutMs`. A non-zero exit code is returned instead.
auto hostExec(JSContext* const ctx, JSValueConst /* thisVal */, const i32 argc, JSValueConst* const argv) -> JSValue {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "rpgm.exec(program, args, options)");
    }

    QStringList args;

    if (argc > 1 && JS_IsArray(argv[1])) {
        const u32 count = jsArrayLength(ctx, argv[1]);
        args.reserve(count);

        for (u32 idx = 0; idx < count; idx++) {
            const auto element = JSValueGuard(ctx, JS_GetPropertyUint32(ctx, argv[1], idx));
            args.push_back(jsToQString(ctx, element));
        }
    }

    QByteArray input;
    QString cwd;
    i32 timeoutMs = EXEC_DEFAULT_TIMEOUT_MS;

    if (argc > 2 && JS_IsObject(argv[2])) {
        const auto stdinValue = JSValueGuard(ctx, JS_GetPropertyStr(ctx, argv[2], "stdin"));
        const auto cwdValue = JSValueGuard(ctx, JS_GetPropertyStr(ctx, argv[2], "cwd"));
        const auto timeoutValue = JSValueGuard(ctx, JS_GetPropertyStr(ctx, argv[2], "timeoutMs"));

        if (JS_IsString(stdinValue)) {
            input = jsToQString(ctx, stdinValue).toUtf8();
        }

        if (JS_IsString(cwdValue)) {
            cwd = jsToQString(ctx, cwdValue);
        }

        if (!JS_IsUndefined(timeoutValue)) {
            JS_ToInt32(ctx, &timeoutMs, timeoutValue);
        }
    }

    // Programs and relative paths in scripts are looked up beside the scripts first, then in PATH
    const QString scriptsDir = dataLocation() % u"/scripts"_qsv;
    const QString program = jsToQString(ctx, argv[0]);
    const QString local = QStandardPaths::findExecutable(program, { scriptsDir });

    if (cwd.isEmpty() && QDir(scriptsDir).exists()) {
        cwd = scriptsDir;
    }

    QProcess process;
    process.setProgram(local.isEmpty() ? program : local);
    process.setArguments(args);

    if (!cwd.isEmpty()) {
        process.setWorkingDirectory(cwd);
    }

    process.start();

    if (!process.waitForStarted(EXEC_START_TIMEOUT_MS)) {
        return throwError(ctx, u"Cannot start process: %1"_qsv.arg(process.errorString()));
    }

    if (!input.isEmpty()) {
        process.write(input);
    }

    process.closeWriteChannel();

    const ScriptRuntime::HostState* const host = hostState(ctx);
    const auto limit = QDeadlineTimer(timeoutMs);

    // waitForFinished also drains stdout/stderr into QProcess's buffers, so a chatty child can't block on a full pipe.
    while (!process.waitForFinished(EXEC_POLL_MS)) {
        if (process.state() == QProcess::NotRunning) {
            break;
        }

        const bool cancelled = host->task != nullptr && host->task->interrupted();

        if (cancelled || limit.hasExpired()) {
            process.kill();
            process.waitForFinished();
            return throwError(ctx, cancelled ? u"Process cancelled."_s : u"Process timed out."_s);
        }
    }

    rearmDeadline(ctx);

    const JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "code", JS_NewInt32(ctx, process.exitCode()));

    const QByteArray out = process.readAllStandardOutput();
    const QByteArray err = process.readAllStandardError();
    JS_SetPropertyStr(ctx, result, "stdout", JS_NewStringLen(ctx, out.constData(), scast<usize>(out.size())));
    JS_SetPropertyStr(ctx, result, "stderr", JS_NewStringLen(ctx, err.constData(), scast<usize>(err.size())));

    return result;
}

// `rpgm.log` is harmless and always there; the functions that touch files and processes need `hostApi`.
void installHostApi(JSContext* const ctx, const bool hostApi) {
    const JSValue global = JS_GetGlobalObject(ctx);
    const JSValue rpgm = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, rpgm, "log", JS_NewCFunction(ctx, hostLog, "log", 1));

    if (hostApi) {
        JS_SetPropertyStr(ctx, rpgm, "readFile", JS_NewCFunction(ctx, hostReadFile, "readFile", 1));
        JS_SetPropertyStr(ctx, rpgm, "writeFile", JS_NewCFunction(ctx, hostWriteFile, "writeFile", 2));
        JS_SetPropertyStr(ctx, rpgm, "exec", JS_NewCFunction(ctx, hostExec, "exec", 3));
    }

    JS_SetPropertyStr(ctx, global, "rpgm", rpgm);
    JS_FreeValue(ctx, global);
}
}  // namespace

auto qstringToJS(JSContext* const ctx, const QStringView qsv) -> JSValue {
    const QByteArray utf8 = qsv.toUtf8();
    return JS_NewStringLen(ctx, utf8.constData(), utf8.size());
}

auto qstringToJS(JSContext* const ctx, const QLatin1StringView qsv) -> JSValue {
    return JS_NewStringLen(ctx, qsv.constData(), qsv.size());
}

auto jsToQString(JSContext* const ctx, const JSValueConst val) -> QString {
    usize len = 0;
    const char* const buf = JS_ToCStringLen(ctx, &len, val);

    if (buf == nullptr) {
        return {};
    }

    QString result = QString::fromUtf8(buf, scast<isize>(len));
    JS_FreeCString(ctx, buf);
    return result;
}

auto glossaryToJS(JSContext* const ctx, const Glossary& glossary) -> JSValue {
    const JSValue terms = JS_NewArray(ctx);
    u32 termIdx = 0;

    for (const Term& term : glossary) {
        if (!term.enabled) {
            continue;
        }

        const JSValue entry = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, entry, "term", qstringToJS(ctx, term.term));
        JS_SetPropertyStr(ctx, entry, "translation", qstringToJS(ctx, term.translation));
        JS_SetPropertyStr(ctx, entry, "note", qstringToJS(ctx, term.note));
        JS_SetPropertyUint32(ctx, terms, termIdx++, entry);
    }

    return terms;
}

auto makeScriptOptions(
    JSContext* const ctx,
    const BatchAction action,
    const u8 columnIndex,
    const BatchVariant& variant,
    const ProjectSettings* const projectSettings,
    const Glossary& glossary
) -> JSValue {
    const JSValue options = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, options, "action", qstringToJS(ctx, batchActionId(action)));
    JS_SetPropertyStr(ctx, options, "columnIndex", JS_NewInt32(ctx, columnIndex));
    JS_SetPropertyStr(ctx, options, "sourceLanguage", qstringToJS(ctx, projectSettings->sourceLang));
    JS_SetPropertyStr(ctx, options, "translationLanguage", qstringToJS(ctx, projectSettings->translationLang));
    JS_SetPropertyStr(ctx, options, "fontSize", JS_NewInt32(ctx, projectSettings->hintFontSize));
    JS_SetPropertyStr(ctx, options, "glossary", glossaryToJS(ctx, glossary));

    if (action == BatchAction::Trim) {
        const LintFlags trimFlags = std::get<LintFlags>(variant);

        JS_SetPropertyStr(
            ctx,
            options,
            "trimLeading",
            JS_NewBool(ctx, (trimFlags & LintFlags::LeadingWhitespace) != 0)
        );
        JS_SetPropertyStr(
            ctx,
            options,
            "trimTrailing",
            JS_NewBool(ctx, (trimFlags & LintFlags::TrailingWhitespace) != 0)
        );
    } else if (action == BatchAction::Wrap) {
        JS_SetPropertyStr(ctx, options, "wrapLength", JS_NewInt32(ctx, std::get<u8>(variant)));
    }

    return options;
}

auto jsArrayLength(JSContext* const ctx, const JSValueConst val) -> u32 {
    u32 len = 0;
    const JSValue prop = JS_GetPropertyStr(ctx, val, "length");
    JS_ToUint32(ctx, &len, prop);
    JS_FreeValue(ctx, prop);
    return len;
}

auto jsGetExceptionText(JSContext* const ctx) -> QString {
    const JSValue exc = JS_GetException(ctx);
    QString msg = jsToQString(ctx, exc);
    JS_FreeValue(ctx, exc);
    return msg;
}

ScriptRuntime::ScriptRuntime(const QDeadlineTimer deadline, const Task* const task, const bool hostApi) :
    deadline_(deadline),
    host{ .task = task, .deadline = &deadline_ } {
    rt_ = JS_NewRuntime();
    JS_SetMemoryLimit(rt_, SCRIPT_MEMORY_LIMIT);
    JS_SetMaxStackSize(rt_, SCRIPT_MAX_STACK_SIZE);
    // Also stops when the task is aborted, so a script stuck in a loop doesn't outlive the Abort button by the timeout
    JS_SetInterruptHandler(rt_, [](JSRuntime*, auto* opaque) -> i32 {
        const auto* const state = scast<const HostState*>(opaque);
        return state->deadline->hasExpired() || (state->task != nullptr && state->task->interrupted()) ? 1 : 0;
    }, &host);
    ctx = JS_NewContext(rt_);
    JS_SetContextOpaque(ctx, &host);
    installHostApi(ctx, hostApi);
}

ScriptRuntime::~ScriptRuntime() {
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt_);
}

auto scriptHash(const QStringView source) -> QString {
    const QByteArray utf8 = source.toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(utf8, QCryptographicHash::Sha256).toHex());
}

auto confirmScriptAccess(QWidget* const parent, QStringList& approved, const QString& name, const QString& source)
    -> bool {
    const QString hash = scriptHash(source);

    if (approved.contains(hash)) {
        return true;
    }

    auto box = QMessageBox(
        QMessageBox::Warning,
        QCoreApplication::translate("ScriptHost", "Allow Script Access"),
        QCoreApplication::translate(
            "ScriptHost",
            "The script \"%1\" can run programs and read or write files on your computer. Only run scripts you trust.\n\nAllow it to run?"
        )
            .arg(name),
        QMessageBox::Yes | QMessageBox::No,
        parent
    );
    box.setDefaultButton(QMessageBox::No);
    // What the approval is for: it is kept for this exact code
    box.setDetailedText(source);

    if (box.exec() != QMessageBox::Yes) {
        qInfo().noquote() << u"Script access was declined by user."_qsv;
        return false;
    }

    qInfo().noquote() << u"Script access was approved by user."_qsv;
    approved.push_back(hash);

    // Hashes of scripts that were edited or deleted since are of no use; the oldest go first
    if (approved.size() > MAX_APPROVED_SCRIPTS) {
        approved.removeFirst();
    }

    return true;
}

auto runTranslateScript(
    const QString& source,
    const QString& text,
    const QString& sourceLanguage,
    const QString& translationLanguage,
    const Glossary& glossary,
    const QString& filename
) -> Result<QString, QString> {
    auto runtime = ScriptRuntime(QDeadlineTimer(SCRIPT_TIMEOUT), nullptr, true);
    JSContext* const ctx = runtime.ctx;

    const QByteArray src = source.toUtf8();
    const auto evalResult =
        JSValueGuard(ctx, JS_Eval(ctx, src.constData(), src.size(), "<user-script>", JS_EVAL_TYPE_GLOBAL));

    if (JS_IsException(evalResult)) {
        return Err(jsGetExceptionText(ctx));
    }

    const JSValue global = JS_GetGlobalObject(ctx);
    const auto callback = JSValueGuard(ctx, JS_GetPropertyStr(ctx, global, "rpgmTranslateCallback"));
    JS_FreeValue(ctx, global);

    if (!JS_IsFunction(ctx, callback)) {
        return Err(
            QCoreApplication::translate(
                "ScriptHost",
                "Script must define function rpgmTranslateCallback(text, sourceLanguage, translationLanguage, glossary, filename)"
            )
        );
    }

    array<JSValue, 5> args = { qstringToJS(ctx, text),
                               qstringToJS(ctx, sourceLanguage),
                               qstringToJS(ctx, translationLanguage),
                               glossaryToJS(ctx, glossary),
                               qstringToJS(ctx, filename) };

    runtime.deadline_ = QDeadlineTimer(SCRIPT_TIMEOUT);
    const auto ret = JSValueGuard(ctx, JS_Call(ctx, callback, JS_UNDEFINED, args.size(), args.data()));

    for (auto& arg : args) {
        JS_FreeValue(ctx, arg);
    }

    if (JS_IsException(ret)) {
        return Err(jsGetExceptionText(ctx));
    }

    if (!JS_IsString(ret)) {
        return Err(QCoreApplication::translate("ScriptHost", "rpgmTranslateCallback must return a string."));
    }

    return jsToQString(ctx, ret);
}

auto runCellScript(
    const QStringView script,
    const BatchAction action,
    const QString& text,
    const u8 columnIndex,
    const BatchVariant& variant,
    const ProjectSettings* const projectSettings,
    const Glossary& glossary
) -> Result<QString, QString> {
    auto runtime = ScriptRuntime(QDeadlineTimer(SCRIPT_TIMEOUT));
    JSContext* const ctx = runtime.ctx;

    const QByteArray src = script.toUtf8();
    const auto evalResult =
        JSValueGuard(ctx, JS_Eval(ctx, src.constData(), src.size(), "<built-in-script>", JS_EVAL_TYPE_GLOBAL));

    if (JS_IsException(evalResult)) {
        return Err(jsGetExceptionText(ctx));
    }

    const JSValue global = JS_GetGlobalObject(ctx);
    const auto callback = JSValueGuard(ctx, JS_GetPropertyStr(ctx, global, "rpgmLineCallback"));
    JS_FreeValue(ctx, global);

    if (!JS_IsFunction(ctx, callback)) {
        return Err(QCoreApplication::translate("ScriptHost", "Script must define function rpgmLineCallback."));
    }

    // Columns before `columnIndex` are empty: the built-in scripts only look at the column they are asked to change.
    const JSValue translations = JS_NewArray(ctx);

    for (u32 idx = 0; idx < columnIndex; idx++) {
        JS_SetPropertyUint32(
            ctx,
            translations,
            idx,
            qstringToJS(ctx, idx + 1 == columnIndex ? QStringView(text) : QStringView())
        );
    }

    array<JSValue, 5> args = { qstringToJS(ctx, QStringView()),
                               translations,
                               qstringToJS(ctx, QStringView()),
                               JS_NewInt32(ctx, 0),
                               makeScriptOptions(ctx, action, columnIndex, variant, projectSettings, glossary) };

    runtime.deadline_ = QDeadlineTimer(SCRIPT_TIMEOUT);
    const auto ret = JSValueGuard(ctx, JS_Call(ctx, callback, JS_UNDEFINED, args.size(), args.data()));

    for (auto& arg : args) {
        JS_FreeValue(ctx, arg);
    }

    if (JS_IsException(ret)) {
        return Err(jsGetExceptionText(ctx));
    }

    if (!JS_IsArray(ret)) {
        return text;
    }

    const auto element = JSValueGuard(ctx, JS_GetPropertyUint32(ctx, ret, columnIndex - 1));

    return JS_IsString(element) ? jsToQString(ctx, element) : text;
}
