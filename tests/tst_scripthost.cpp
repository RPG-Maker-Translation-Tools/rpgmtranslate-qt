#include "ScriptHost.hpp"

#include "ProjectSettings.hpp"
#include "TestFiles.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>
#include <cstring>

namespace {
// Evaluates `code` and returns the result as text, or `EXCEPTION: ` and what was thrown
auto evalText(ScriptRuntime& runtime, const char* const code) -> QString {
    JSContext* const ctx = runtime.ctx;
    const auto result = JSValueGuard(ctx, JS_Eval(ctx, code, std::strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL));

    if (JS_IsException(result)) {
        return u"EXCEPTION: "_s + jsGetExceptionText(ctx);
    }

    return jsToQString(ctx, result);
}

// A path as a JS string literal. Qt paths use forward slashes, so nothing needs escaping
auto quoted(const QString& path) -> QString {
    return u"\"%1\""_s.arg(path);
}

auto termOf(const QString& term, const QString& translation, const QString& note, const bool enabled) -> Term {
    Term made;
    made.term = term;
    made.translation = translation;
    made.note = note;
    made.enabled = enabled;
    return made;
}

auto projectWithLanguages() -> ProjectSettings {
    ProjectSettings settings;
    settings.sourceLang = u"ja"_s;
    settings.translationLang = u"en"_s;
    settings.hintFontSize = 24;
    return settings;
}

// The text of a failed result, or nothing for a good one. QVERIFY2 evaluates its message even when it passes, and
// `error()` of a value is not defined.
auto errorOf(const Result<QString, QString>& result) -> QString {
    return result.has_value() ? QString() : result.error();
}

constexpr auto SCRIPT_ECHOING_ARGUMENTS = R"(
function rpgmTranslateCallback(text, src, dst, glossary, filename) {
    return [text, src, dst, glossary.length, glossary[0].term, glossary[0].translation, glossary[0].note, filename].join("|");
}
)";
}  // namespace

class TestScriptHost final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() {
        // Scripts look beside themselves for programs: keep that out of the real user data
        QStandardPaths::setTestModeEnabled(true);
    }

    void scriptHashIsHexSha256() {
        QCOMPARE(scriptHash(QStringView()), u"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"_s);
        QCOMPARE(scriptHash(u"abc"_qsv), u"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"_s);
        QVERIFY(scriptHash(u"abc"_qsv) != scriptHash(u"abd"_qsv));
        // The text is hashed as UTF-8, so the same script hashes the same on every platform
        QCOMPARE(scriptHash(u"\u00E9"_qsv).size(), qsizetype{ 64 });
    }

    void approvedScriptsAreNotAskedAgain() {
        QStringList approved{ scriptHash(u"print(1)"_qsv) };

        QVERIFY(confirmScriptAccess(nullptr, approved, u"name"_s, u"print(1)"_s));
        QCOMPARE(approved.size(), qsizetype{ 1 });
    }

    void stringsRoundTripThroughJS() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };
        const QString text = u"plain, éè, 日本語, \U0001F600, \\# and \"quotes\""_s;

        const auto value = JSValueGuard(runtime.ctx, qstringToJS(runtime.ctx, QStringView(text)));

        QVERIFY(JS_IsString(value));
        QCOMPARE(jsToQString(runtime.ctx, value), text);
    }

    void emptyStringRoundTrips() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };
        const auto value = JSValueGuard(runtime.ctx, qstringToJS(runtime.ctx, QStringView()));

        QVERIFY(JS_IsString(value));
        QVERIFY(jsToQString(runtime.ctx, value).isEmpty());
    }

    void latin1StringsAreConverted() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };

        const auto ascii = JSValueGuard(runtime.ctx, qstringToJS(runtime.ctx, "trim"_L1));
        QCOMPARE(jsToQString(runtime.ctx, ascii), u"trim"_s);

        // 0xE9 is a single byte in Latin-1 and two in UTF-8, which is what JS is handed
        const auto accented = JSValueGuard(runtime.ctx, qstringToJS(runtime.ctx, "caf\xE9"_L1));
        QCOMPARE(jsToQString(runtime.ctx, accented), u"café"_s);
    }

    void nonStringsAreConvertedToText() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };

        QCOMPARE(evalText(runtime, "42"), u"42"_s);
        QCOMPARE(evalText(runtime, "undefined"), u"undefined"_s);
        QCOMPARE(evalText(runtime, "[1, 2, 3]"), u"1,2,3"_s);
    }

    void arrayLengthAndExceptions() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };

        const auto array = JSValueGuard(runtime.ctx, JS_Eval(runtime.ctx, "[1, 2, 3, 4]", 12, "<test>", JS_EVAL_TYPE_GLOBAL));
        QCOMPARE(jsArrayLength(runtime.ctx, array), u32{ 4 });

        const auto empty = JSValueGuard(runtime.ctx, JS_Eval(runtime.ctx, "[]", 2, "<test>", JS_EVAL_TYPE_GLOBAL));
        QCOMPARE(jsArrayLength(runtime.ctx, empty), u32{ 0 });

        QVERIFY(evalText(runtime, "throw new Error('boom')").contains(u"boom"_qsv));
        QVERIFY(evalText(runtime, "function (").startsWith(u"EXCEPTION: "_qsv));
    }

    void glossaryKeepsOnlyEnabledTerms() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };
        const Glossary glossary{
            termOf(u"HP"_s, u"体力"_s, u"hit points"_s, true),
            termOf(u"skip"_s, u"x"_s, u"y"_s, false),
            termOf(u"MP"_s, u"魔力"_s, QString(), true),
        };

        const auto array = JSValueGuard(runtime.ctx, glossaryToJS(runtime.ctx, glossary));

        QCOMPARE(jsArrayLength(runtime.ctx, array), u32{ 2 });

        const auto second = JSValueGuard(runtime.ctx, JS_GetPropertyUint32(runtime.ctx, array, 1));
        const auto term = JSValueGuard(runtime.ctx, JS_GetPropertyStr(runtime.ctx, second, "term"));
        const auto translation = JSValueGuard(runtime.ctx, JS_GetPropertyStr(runtime.ctx, second, "translation"));
        const auto note = JSValueGuard(runtime.ctx, JS_GetPropertyStr(runtime.ctx, second, "note"));

        QCOMPARE(jsToQString(runtime.ctx, term), u"MP"_s);
        QCOMPARE(jsToQString(runtime.ctx, translation), u"魔力"_s);
        QVERIFY(JS_IsString(note));
        QVERIFY(jsToQString(runtime.ctx, note).isEmpty());
    }

    void emptyGlossaryIsAnEmptyArray() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };
        const auto array = JSValueGuard(runtime.ctx, glossaryToJS(runtime.ctx, Glossary{}));

        QVERIFY(JS_IsArray(array));
        QCOMPARE(jsArrayLength(runtime.ctx, array), u32{ 0 });
    }

    void translateScriptReceivesEveryArgument() {
        const Glossary glossary{ termOf(u"HP"_s, u"Health"_s, u"stat"_s, true), termOf(u"no"_s, u"no"_s, u"no"_s, false) };

        const auto result = runTranslateScript(
            QString::fromUtf8(SCRIPT_ECHOING_ARGUMENTS),
            u"Hello"_s,
            u"ja"_s,
            u"en"_s,
            glossary,
            u"Map001.txt"_s
        );

        QVERIFY2(result.has_value(), qPrintable(errorOf(result)));
        QCOMPARE(*result, u"Hello|ja|en|1|HP|Health|stat|Map001.txt"_s);
    }

    void translateScriptErrors_data() {
        QTest::addColumn<QString>("script");
        QTest::addColumn<QString>("expected");

        QTest::newRow("no callback") << u"var x = 1;"_s << u"rpgmTranslateCallback"_s;
        QTest::newRow("callback is not a function") << u"var rpgmTranslateCallback = 5;"_s << u"rpgmTranslateCallback"_s;
        QTest::newRow("syntax error") << u"function ("_s << u"SyntaxError"_s;
        QTest::newRow("throws") << u"function rpgmTranslateCallback() { throw new Error('nope'); }"_s << u"nope"_s;
        QTest::newRow("returns a number") << u"function rpgmTranslateCallback() { return 5; }"_s << u"must return a string"_s;
        QTest::newRow("returns nothing") << u"function rpgmTranslateCallback() {}"_s << u"must return a string"_s;
        QTest::newRow("top level throws") << u"throw new Error('at load');"_s << u"at load"_s;
    }

    void translateScriptErrors() {
        QFETCH(const QString, script);
        QFETCH(const QString, expected);

        const auto result = runTranslateScript(script, u"t"_s, u"ja"_s, u"en"_s, Glossary{}, u"f"_s);

        QVERIFY(!result.has_value());
        QVERIFY2(result.error().contains(expected), qPrintable(result.error()));
    }

    void translateScriptHasTheHostApi() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"out.txt"_s);
        const QString script = u"function rpgmTranslateCallback(text) { rpgm.writeFile(%1, text + '!'); return rpgm.readFile(%1); }"_s.arg(quoted(path));

        const auto result = runTranslateScript(script, u"hi"_s, u"ja"_s, u"en"_s, Glossary{}, u"f"_s);

        QVERIFY2(result.has_value(), qPrintable(errorOf(result)));
        QCOMPARE(*result, u"hi!"_s);
    }

    void cellScriptEditsTheRequestedColumn() {
        const auto settings = projectWithLanguages();
        const QString script = u"function rpgmLineCallback(src, translations) { translations[1] = translations[1].toUpperCase(); return translations; }"_s;

        const auto result = runCellScript(script, BatchAction::Custom, u"abc"_s, 2, BatchVariant{ u8{ 0 } }, &settings, Glossary{});

        QVERIFY2(result.has_value(), qPrintable(errorOf(result)));
        QCOMPARE(*result, u"ABC"_s);
    }

    void cellScriptThatLeavesTheCellAloneGivesTheTextBack() {
        const auto settings = projectWithLanguages();
        const BatchVariant variant{ u8{ 0 } };

        const auto nothing = runCellScript(u"function rpgmLineCallback() {}"_s, BatchAction::Custom, u"keep"_s, 1, variant, &settings, Glossary{});
        const auto notAnArray = runCellScript(u"function rpgmLineCallback() { return 'x'; }"_s, BatchAction::Custom, u"keep"_s, 1, variant, &settings, Glossary{});
        const auto notAString = runCellScript(u"function rpgmLineCallback() { return [5]; }"_s, BatchAction::Custom, u"keep"_s, 1, variant, &settings, Glossary{});

        QVERIFY(nothing.has_value());
        QVERIFY(notAnArray.has_value());
        QVERIFY(notAString.has_value());
        QCOMPARE(*nothing, u"keep"_s);
        QCOMPARE(*notAnArray, u"keep"_s);
        QCOMPARE(*notAString, u"keep"_s);
    }

    // Columns before the one being changed are empty and the column itself holds the text
    void cellScriptSeesTheColumnsBeforeIt() {
        const auto settings = projectWithLanguages();
        const QString script = u"function rpgmLineCallback(src, tr) { return [tr.length + ':' + tr.join('|')]; }"_s;

        const auto result = runCellScript(script, BatchAction::Custom, u"text"_s, 1, BatchVariant{ u8{ 0 } }, &settings, Glossary{});
        QVERIFY(result.has_value());
        QCOMPARE(*result, u"1:text"_s);

        const auto third = runCellScript(
            u"function rpgmLineCallback(src, tr) { return [0, 0, tr.length + ':' + tr.join('|')]; }"_s,
            BatchAction::Custom,
            u"text"_s,
            3,
            BatchVariant{ u8{ 0 } },
            &settings,
            Glossary{}
        );
        QVERIFY(third.has_value());
        QCOMPARE(*third, u"3:||text"_s);
    }

    void cellScriptWithoutAColumnReturnsTheText() {
        const auto settings = projectWithLanguages();

        const auto result = runCellScript(
            u"function rpgmLineCallback(src, tr) { return ['changed']; }"_s,
            BatchAction::Custom,
            u"text"_s,
            0,
            BatchVariant{ u8{ 0 } },
            &settings,
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(*result, u"text"_s);
    }

    void cellScriptErrors() {
        const auto settings = projectWithLanguages();
        const BatchVariant variant{ u8{ 0 } };

        const auto missing = runCellScript(u"var x;"_s, BatchAction::Custom, u"t"_s, 1, variant, &settings, Glossary{});
        QVERIFY(!missing.has_value());
        QVERIFY(missing.error().contains(u"rpgmLineCallback"_qsv));

        const auto thrown = runCellScript(u"function rpgmLineCallback() { throw new Error('bad'); }"_s, BatchAction::Custom, u"t"_s, 1, variant, &settings, Glossary{});
        QVERIFY(!thrown.has_value());
        QVERIFY(thrown.error().contains(u"bad"_qsv));

        const auto syntax = runCellScript(u"function ("_s, BatchAction::Custom, u"t"_s, 1, variant, &settings, Glossary{});
        QVERIFY(!syntax.has_value());
    }

    void optionsDescribeTheAction() {
        const auto settings = projectWithLanguages();
        const Glossary glossary{ termOf(u"HP"_s, u"Health"_s, QString(), true) };

        const auto optionsOf = [&](const BatchAction action, const BatchVariant& variant) -> QJsonObject {
            const auto result = runCellScript(
                u"function rpgmLineCallback(src, tr, f, i, options) { return [JSON.stringify(options)]; }"_s,
                action,
                u"t"_s,
                1,
                variant,
                &settings,
                glossary
            );
            if (!result.has_value()) {
                return {};
            }
            return QJsonDocument::fromJson(result->toUtf8()).object();
        };

        const QJsonObject trim = optionsOf(BatchAction::Trim, BatchVariant{ LintFlags::LeadingWhitespace });
        QCOMPARE(trim[u"action"_s].toString(), u"trim"_s);
        QCOMPARE(trim[u"columnIndex"_s].toInt(), 1);
        QCOMPARE(trim[u"sourceLanguage"_s].toString(), u"ja"_s);
        QCOMPARE(trim[u"translationLanguage"_s].toString(), u"en"_s);
        QCOMPARE(trim[u"fontSize"_s].toInt(), 24);
        QCOMPARE(trim[u"glossary"_s].toArray().size(), qsizetype{ 1 });
        QVERIFY(trim[u"trimLeading"_s].toBool());
        QVERIFY(!trim[u"trimTrailing"_s].toBool());
        QVERIFY(!trim.contains(u"wrapLength"_s));

        const QJsonObject both = optionsOf(BatchAction::Trim, BatchVariant{ static_cast<LintFlags>(LintFlags::LeadingWhitespace | LintFlags::TrailingWhitespace) });
        QVERIFY(both[u"trimLeading"_s].toBool());
        QVERIFY(both[u"trimTrailing"_s].toBool());

        const QJsonObject wrap = optionsOf(BatchAction::Wrap, BatchVariant{ u8{ 44 } });
        QCOMPARE(wrap[u"action"_s].toString(), u"wrap"_s);
        QCOMPARE(wrap[u"wrapLength"_s].toInt(), 44);
        QVERIFY(!wrap.contains(u"trimLeading"_s));

        const QJsonObject latinize = optionsOf(BatchAction::Latinize, BatchVariant{ u8{ 0 } });
        QCOMPARE(latinize[u"action"_s].toString(), u"latinize"_s);
        QVERIFY(!latinize.contains(u"wrapLength"_s));
        QVERIFY(!latinize.contains(u"trimLeading"_s));
    }

    // Built-in scripts are trusted but still get no files or processes
    void cellScriptsHaveNoHostApi() {
        const auto settings = projectWithLanguages();

        const auto result = runCellScript(
            u"function rpgmLineCallback() { return [typeof rpgm.log + ',' + typeof rpgm.readFile + ',' + typeof rpgm.writeFile + ',' + typeof rpgm.exec]; }"_s,
            BatchAction::Custom,
            u"t"_s,
            1,
            BatchVariant{ u8{ 0 } },
            &settings,
            Glossary{}
        );

        QVERIFY(result.has_value());
        QCOMPARE(*result, u"function,undefined,undefined,undefined"_s);
    }

    void hostApiIsInstalledOnRequest() {
        ScriptRuntime plain{ QDeadlineTimer(5000) };
        QCOMPARE(evalText(plain, "typeof rpgm.exec"), u"undefined"_s);

        ScriptRuntime trusted{ QDeadlineTimer(5000), nullptr, true };
        QCOMPARE(evalText(trusted, "typeof rpgm.log + typeof rpgm.readFile + typeof rpgm.writeFile + typeof rpgm.exec"), u"functionfunctionfunctionfunction"_s);
    }

    void logKeepsTheLatestLines() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };
        QStringList log;
        runtime.host.log = &log;

        QTest::ignoreMessage(QtInfoMsg, QRegularExpression(u".*"_s));
        evalText(runtime, "rpgm.log('first')");
        QCOMPARE(log, QStringList{ u"first"_s });

        // The rest are logged to the console as well
        for (i32 idx = 0; idx < 40; idx++) {
            QTest::ignoreMessage(QtInfoMsg, QRegularExpression(u".*"_s));
        }
        evalText(runtime, "for (let i = 0; i < 40; i++) rpgm.log('line ' + i)");

        QCOMPARE(log.size(), qsizetype{ 30 });
        QCOMPARE(log.first(), u"line 10"_s);
        QCOMPARE(log.last(), u"line 39"_s);
    }

    void logWithoutArgumentsIsIgnored() {
        ScriptRuntime runtime{ QDeadlineTimer(5000) };
        QStringList log;
        runtime.host.log = &log;

        QCOMPARE(evalText(runtime, "rpgm.log()"), u"undefined"_s);
        QVERIFY(log.isEmpty());
    }

    void filesRoundTrip() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath(u"note.txt"_s);
        const QString quotedPath = quoted(path);

        ScriptRuntime runtime{ QDeadlineTimer(5000), nullptr, true };

        const QString written = evalText(runtime, qPrintable(u"rpgm.writeFile(%1, 'h\\u00E9llo \\u65E5\\u672C'); rpgm.readFile(%1)"_s.arg(quotedPath)));
        QCOMPARE(written, u"héllo 日本"_s);
        QCOMPARE(readFile(path), QByteArray("h\xC3\xA9llo \xE6\x97\xA5\xE6\x9C\xAC"));

        // Overwrites rather than appends
        evalText(runtime, qPrintable(u"rpgm.writeFile(%1, 'x')"_s.arg(quotedPath)));
        QCOMPARE(readFile(path), QByteArray("x"));
    }

    void fileErrorsAreThrown() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ScriptRuntime runtime{ QDeadlineTimer(5000), nullptr, true };

        const QString missing = quoted(dir.filePath(u"missing.txt"_s));
        QVERIFY(evalText(runtime, qPrintable(u"rpgm.readFile(%1)"_s.arg(missing))).contains(u"Cannot read file"_qsv));

        QVERIFY(evalText(runtime, "rpgm.readFile()").contains(u"readFile(path)"_qsv));
        QVERIFY(evalText(runtime, "rpgm.writeFile('only a path')").contains(u"writeFile(path, text)"_qsv));
        QVERIFY(evalText(runtime, "rpgm.exec()").contains(u"exec(program"_qsv));

        const QString inMissingDirectory = quoted(dir.filePath(u"no/such/dir/file.txt"_s));
        QVERIFY(evalText(runtime, qPrintable(u"rpgm.writeFile(%1, 'x')"_s.arg(inMissingDirectory))).contains(u"Cannot write file"_qsv));
    }

    void execReturnsCodeAndOutput() {
#ifdef Q_OS_WIN
        QSKIP("Needs a POSIX shell");
#else
        ScriptRuntime runtime{ QDeadlineTimer(10000), nullptr, true };

        const QString result = evalText(runtime, R"(JSON.stringify(rpgm.exec("sh", ["-c", "echo out; echo err >&2; exit 3"])))");
        const QJsonObject object = QJsonDocument::fromJson(result.toUtf8()).object();

        QCOMPARE(object[u"code"_s].toInt(), 3);
        QCOMPARE(object[u"stdout"_s].toString(), u"out\n"_s);
        QCOMPARE(object[u"stderr"_s].toString(), u"err\n"_s);
#endif
    }

    void execFeedsStdin() {
#ifdef Q_OS_WIN
        QSKIP("Needs a POSIX shell");
#else
        ScriptRuntime runtime{ QDeadlineTimer(10000), nullptr, true };

        QCOMPARE(evalText(runtime, R"(rpgm.exec("cat", [], { stdin: "from é stdin" }).stdout)"), u"from é stdin"_s);
#endif
    }

    void execUsesTheWorkingDirectory() {
#ifdef Q_OS_WIN
        QSKIP("Needs a POSIX shell");
#else
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ScriptRuntime runtime{ QDeadlineTimer(10000), nullptr, true };
        const QString code = u"rpgm.exec('pwd', [], { cwd: \"%1\" }).stdout"_s.arg(dir.path());

        QCOMPARE(QDir(evalText(runtime, qPrintable(code)).trimmed()).canonicalPath(), QDir(dir.path()).canonicalPath());
#endif
    }

    void execFailuresAreThrown() {
#ifdef Q_OS_WIN
        QSKIP("Needs a POSIX shell");
#else
        ScriptRuntime runtime{ QDeadlineTimer(10000), nullptr, true };

        QVERIFY(evalText(runtime, R"(rpgm.exec("definitely-not-a-program-rpgm"))").contains(u"Cannot start process"_qsv));
        QVERIFY(evalText(runtime, R"(rpgm.exec("sleep", ["5"], { timeoutMs: 200 }))").contains(u"timed out"_qsv));
#endif
    }

    void execIsCancelledWithTheTask() {
#ifdef Q_OS_WIN
        QSKIP("Needs a POSIX shell");
#else
        Task task(1, TaskKind::Search);
        ScriptRuntime runtime{ QDeadlineTimer(10000), &task, true };

        // The script is blocked in the process, so the Abort button can only come from another thread
        const std::unique_ptr<QThread> canceller(QThread::create([&task]() -> void {
            QThread::msleep(300);
            task.interrupt();
        }));
        canceller->start();

        QElapsedTimer timer;
        timer.start();
        const QString result = evalText(runtime, R"(rpgm.exec("sleep", ["5"]))");
        canceller->wait();

        QVERIFY2(result.contains(u"cancelled"_qsv), qPrintable(result));
        QVERIFY(timer.elapsed() < 4000);
#endif
    }

    // A script that never returns must not hang the application
    void endlessLoopsAreStoppedByTheDeadline() {
        ScriptRuntime runtime{ QDeadlineTimer(100) };

        const QString result = evalText(runtime, "while (true) {}");

        QVERIFY(result.startsWith(u"EXCEPTION: "_qsv));
        QVERIFY2(result.contains(u"interrupted"_qsv), qPrintable(result));
    }

    void endlessLoopsAreStoppedByTheTask() {
        Task task(1, TaskKind::Search);
        ScriptRuntime runtime{ QDeadlineTimer(60000), &task };

        task.interrupt();

        QVERIFY2(evalText(runtime, "while (true) {}").contains(u"interrupted"_qsv), "task interrupt did not stop the script");
    }

    void runawayMemoryIsRefused() {
        ScriptRuntime runtime{ QDeadlineTimer(10000) };

        const QString result = evalText(runtime, "const a = []; while (true) { a.push('x'.repeat(1 << 20) + a.length); }");

        QVERIFY(result.startsWith(u"EXCEPTION: "_qsv));
    }
};

QTEST_MAIN(TestScriptHost)
#include "tst_scripthost.moc"
