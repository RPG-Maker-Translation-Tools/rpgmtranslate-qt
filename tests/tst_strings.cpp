#include "QtStrings.hpp"

#include "QtAliases.hpp"
#include "StringBuilder.hpp"

#include <QtTest>

class TestStrings final : public QObject {
    Q_OBJECT

   private slots:
    void replaceView_data() {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("needle");
        QTest::addColumn<QString>("replacement");
        QTest::addColumn<QString>("expected");

        QTest::newRow("no match") << u"abc"_s << u"x"_s << u"y"_s << u"abc"_s;
        QTest::newRow("single") << u"abc"_s << u"b"_s << u"XX"_s << u"aXXc"_s;
        QTest::newRow("every occurrence") << u"a-b-c"_s << u"-"_s << u"+"_s << u"a+b+c"_s;
        QTest::newRow("adjacent") << u"aaaa"_s << u"aa"_s << u"b"_s << u"bb"_s;
        QTest::newRow("overlapping needle is not rescanned") << u"aaa"_s << u"aa"_s << u"b"_s << u"ba"_s;
        QTest::newRow("longer replacement is not rescanned") << u"aa"_s << u"a"_s << u"aa"_s << u"aaaa"_s;
        QTest::newRow("delete") << u"a<#>b<#>c"_s << u"<#>"_s << QString() << u"abc"_s;
        QTest::newRow("at both ends") << u"--x--"_s << u"--"_s << u"+"_s << u"+x+"_s;
        QTest::newRow("whole input") << u"abc"_s << u"abc"_s << u"z"_s << u"z"_s;
        QTest::newRow("empty needle is a no-op") << u"abc"_s << QString() << u"z"_s << u"abc"_s;
        QTest::newRow("empty input") << QString() << u"a"_s << u"z"_s << QString();
        QTest::newRow("needle longer than input") << u"ab"_s << u"abc"_s << u"z"_s << u"ab"_s;
    }

    void replaceView() {
        QFETCH(const QString, input);
        QFETCH(const QString, needle);
        QFETCH(const QString, replacement);
        QFETCH(const QString, expected);

        QCOMPARE(qsvReplace(QStringView(input), QStringView(needle), QStringView(replacement)), expected);
    }

    void replaceChar() {
        QCOMPARE(qsvReplace(u"a/b/c"_qsv, u'/', u'\\'), u"a\\b\\c"_s);
        QCOMPARE(qsvReplace(u"abc"_qsv, u'x', u'y'), u"abc"_s);
        QCOMPARE(qsvReplace(QStringView(), u'x', u'y'), QString());
    }

    void joinView() {
        const QSVList parts{ u"a"_qsv, u"bb"_qsv, u"ccc"_qsv };

        QCOMPARE(joinQSVList(parts, u", "_qsv), u"a, bb, ccc"_s);
        QCOMPARE(joinQSVList(parts, QStringView()), u"abbccc"_s);
        QCOMPARE(joinQSVList(QSVList{ u"only"_qsv }, u", "_qsv), u"only"_s);
        QCOMPARE(joinQSVList(QSVList{}, u", "_qsv), QString());
    }

    void joinChar() {
        QCOMPARE(joinQSVList(QSVList{ u"a"_qsv, u"b"_qsv, u"c"_qsv }, u'|'), u"a|b|c"_s);
        QCOMPARE(joinQSVList(QSVList{ u"only"_qsv }, u'|'), u"only"_s);
        QCOMPARE(joinQSVList(QSVList{}, u'|'), QString());
        // Empty entries still get their separators
        QCOMPARE(joinQSVList(QSVList{ u""_qsv, u""_qsv, u""_qsv }, u'|'), u"||"_s);
    }

    void lastComponent_data() {
        QTest::addColumn<QString>("path");
        QTest::addColumn<QString>("expected");

        QTest::newRow("file only") << u"file.txt"_s << u"file.txt"_s;
        QTest::newRow("unix") << u"a/b/file.txt"_s << u"file.txt"_s;
        QTest::newRow("windows") << u"C:\\a\\b\\file.txt"_s << u"file.txt"_s;
        QTest::newRow("mixed") << u"a\\b/file.txt"_s << u"file.txt"_s;
        QTest::newRow("trailing slash") << u"a/b/"_s << QString();
        QTest::newRow("root file") << u"/file"_s << u"file"_s;
        QTest::newRow("empty") << QString() << QString();
    }

    void lastComponent() {
        QFETCH(const QString, path);
        QFETCH(const QString, expected);

        QCOMPARE(lastPathComponent(path).toString(), expected);
    }

    void codePointAt_() {
        const QString text = u"a\u00E9\U0001F600z"_s;  // a, e-acute, grinning face (surrogate pair), z

        QCOMPARE(codePointAt(text, 0), u32{ 'a' });
        QCOMPARE(codePointAt(text, 1), u32{ 0xE9 });
        QCOMPARE(codePointAt(text, 2), u32{ 0x1F600 });
        // The low half of a pair on its own is returned as is
        QCOMPARE(codePointAt(text, 3), u32{ QChar(text[3]).unicode() });
        QCOMPARE(codePointAt(text, 4), u32{ 'z' });
    }

    void codePointAtLoneSurrogates() {
        // A high surrogate with nothing after it, or followed by a non-surrogate, is not combined
        const QString trailing = QString(QChar(0xD83D));
        QCOMPARE(codePointAt(trailing, 0), u32{ 0xD83D });

        const QString broken = QString(QChar(0xD83D)) + u"a"_s;
        QCOMPARE(codePointAt(broken, 0), u32{ 0xD83D });
    }

    // StringBuilder.hpp
    void builderConcatenation() {
        const string text = "abc";
        const string_view view = "def";
        const char* const ptr = "ghi";

        QCOMPARE(QString::fromStdString(string("x" % text % view % ptr % '!')), u"xabcdefghi!"_s);
    }

    void builderNumbers() {
        QCOMPARE(QString::fromStdString(string(string_view("n=") % 42)), u"n=42"_s);
        QCOMPARE(QString::fromStdString(string(string_view("n=") % -7 % ";")), u"n=-7;"_s);
        QCOMPARE(QString::fromStdString(string(string_view("n=") % 0)), u"n=0"_s);
        QCOMPARE(QString::fromStdString(string(string_view("n=") % u64{ 18446744073709551615ULL })), u"n=18446744073709551615"_s);
        QCOMPARE(QString::fromStdString(string(7 % string_view("th"))), u"7th"_s);
    }

    void builderSizeMatchesOutput() {
        const auto chain = "abc" % string("de") % 123 % '!';

        QCOMPARE(chain.size(), chain.str().size());
        QCOMPARE(chain.size(), usize{ 9 });
    }

    void builderChainsCompose() {
        const auto left = "a" % string("b");
        const auto right = "c" % string("d");

        QCOMPARE(QString::fromStdString((left % right).str()), u"abcd"_s);
        QCOMPARE(QString::fromStdString((left % '-' % right).str()), u"ab-cd"_s);
    }

    void catFreeFunction() {
        QCOMPARE(QString::fromStdString(cat()), QString());
        QCOMPARE(QString::fromStdString(cat("a", 1, 'b', string("c"))), u"a1bc"_s);
    }

    void appendKeepsPrefix() {
        string dst = "head:";

        append(dst, "a", 1, '/', string_view("z"));
        QCOMPARE(QString::fromStdString(dst), u"head:a1/z"_s);

        append(dst);
        QCOMPARE(QString::fromStdString(dst), u"head:a1/z"_s);

        string empty;
        append(empty, "x");
        QCOMPARE(QString::fromStdString(empty), u"x"_s);
    }

    void builderEmbeddedNul() {
        const string_view with_nul("a\0b", 3);

        QCOMPARE(cat(with_nul, 'c').size(), usize{ 4 });
    }
};

QTEST_APPLESS_MAIN(TestStrings)
#include "tst_strings.moc"
