#include "Notice.hpp"

#include <QtTest>

namespace {
struct Logged {
    QtMsgType type;
    QString text;
};

QList<Logged> logged;
QtMessageHandler previousHandler = nullptr;

void captureHandler(const QtMsgType type, const QMessageLogContext&, const QString& message) {
    logged.append(Logged{ type, message });
}
}  // namespace

class TestNotice final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() { previousHandler = qInstallMessageHandler(captureHandler); }

    void cleanupTestCase() { qInstallMessageHandler(previousHandler); }

    void init() { logged.clear(); }

    void noPlaceholdersIsReturnedAsIs() {
        const Notice notice("Test", "Nothing to fill, 100% plain", Severity::Info, Delivery::Inline);

        QCOMPARE(notice.english(), u"Nothing to fill, 100% plain"_s);
    }

    void argumentsFillTheirPlaceholders() {
        const Notice notice("Test", "Failed to open %1: %2", Severity::Info, Delivery::Inline, { "a.txt", "denied" });

        QCOMPARE(notice.english(), u"Failed to open a.txt: denied"_s);
    }

    void placeholdersMayRepeatAndReorder() {
        const Notice notice("Test", "%2 %1 %2", Severity::Info, Delivery::Inline, { "one", "two" });

        QCOMPARE(notice.english(), u"two one two"_s);
    }

    // Substituted text is never scanned again, so a path holding `%1` stays as it is
    void substitutedTextIsNotRescanned() {
        const Notice notice("Test", "%1|%2", Severity::Info, Delivery::Inline, { "%2", "x" });

        QCOMPARE(notice.english(), u"%2|x"_s);
    }

    void unknownPlaceholdersAreKept() {
        const Notice notice("Test", "%1 %2 %0 %x 50%", Severity::Info, Delivery::Inline, { "a" });

        QCOMPARE(notice.english(), u"a %2 %0 %x 50%"_s);
    }

    void twoDigitPlaceholders() {
        const Notice notice(
            "Test",
            "%12|%1",
            Severity::Info,
            Delivery::Inline,
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "twelve" }
        );

        QCOMPARE(notice.english(), u"twelve|1"_s);
    }

    void countReplacesPercentN() {
        const Notice notice("Test", "%n file(s) in %1", Severity::Info, Delivery::Inline, { "dir" }, 3);

        QCOMPARE(notice.english(), u"3 file(s) in dir"_s);
    }

    void zeroIsACount() {
        const Notice notice("Test", "%n left", Severity::Info, Delivery::Inline, {}, 0);

        QCOMPARE(notice.english(), u"0 left"_s);
    }

    void negativeCountLeavesPercentN() {
        const Notice notice("Test", "%n left, %1", Severity::Info, Delivery::Inline, { "a" });

        QCOMPARE(notice.english(), u"%n left, a"_s);
    }

    void translatedFallsBackToEnglish() {
        const Notice notice("Test", "Saved %1", Severity::Info, Delivery::Inline, { "x" });

        QCOMPARE(notice.translated(), u"Saved x"_s);

        const Notice counted("Test", "%n item(s)", Severity::Info, Delivery::Inline, {}, 5);
        QCOMPARE(counted.translated(), u"5 item(s)"_s);
    }

    void accessors() {
        const Notice notice("Test", "x", Severity::Warning, Delivery::Modal);

        QCOMPARE(notice.severity(), Severity::Warning);
        QCOMPARE(notice.delivery(), Delivery::Modal);
    }

    // A notice logs itself, in English, when it is made
    void constructionLogsAtTheRightLevel_data() {
        QTest::addColumn<Severity>("severity");
        QTest::addColumn<QtMsgType>("type");

        QTest::newRow("debug") << Severity::Debug << QtDebugMsg;
        QTest::newRow("info") << Severity::Info << QtInfoMsg;
        QTest::newRow("warning") << Severity::Warning << QtWarningMsg;
        QTest::newRow("critical") << Severity::Critical << QtCriticalMsg;
    }

    void constructionLogsAtTheRightLevel() {
        QFETCH(const Severity, severity);
        QFETCH(const QtMsgType, type);

        const Notice notice("Test", "Hello %1", severity, Delivery::Inline, { "log" });

        QCOMPARE(logged.size(), qsizetype{ 1 });
        QCOMPARE(logged[0].type, type);
        QCOMPARE(logged[0].text, u"Hello log"_s);
    }

    void presentIgnoresInlineNotices() {
        const Notice notice("Test", "Not shown", Severity::Critical, Delivery::Inline);

        // A modal would block the test forever, so returning at all is the check
        present(nullptr, notice);
    }
};

QTEST_MAIN(TestNotice)
#include "tst_notice.moc"
