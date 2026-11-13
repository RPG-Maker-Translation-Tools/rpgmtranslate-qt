#include "ModalCatcher.hpp"
#include "Notice.hpp"

#include <QMainWindow>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
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

struct ShownBox {
    QString title;
    QString text;
    QMessageBox::Icon icon = QMessageBox::NoIcon;
    bool found = false;
};

// `present` blocks in `exec()` for a modal, so the box is inspected and closed from the event loop it runs. The second
// timer closes everything if the box never showed up, rather than hanging the test.
void captureModal(ShownBox& shown) {
    QTimer::singleShot(0, [&shown]() -> void {
        auto* const box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());

        if (box != nullptr) {
            shown = { .title = box->windowTitle(), .text = box->text(), .icon = box->icon(), .found = true };
            box->accept();
        }
    });
    QTimer::singleShot(3000, []() -> void { QApplication::closeAllWindows(); });
}
}  // namespace

using MakeNotice = Notice (*)();
Q_DECLARE_METATYPE(MakeNotice)

class TestNotice final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() {
        qRegisterMetaType<MakeNotice>();
        previousHandler = qInstallMessageHandler(captureHandler);
    }

    void cleanupTestCase() { qInstallMessageHandler(previousHandler); }

    void init() { logged.clear(); }

    // Each row builds a notice and names the English text it must fill in
    void english_data() {
        QTest::addColumn<MakeNotice>("make");
        QTest::addColumn<QString>("expected");

        QTest::newRow("noPlaceholdersIsReturnedAsIs")
            << MakeNotice([]() -> Notice { return { "Test", "Nothing to fill, 100% plain", Severity::Info, Delivery::Inline }; })
            << u"Nothing to fill, 100% plain"_s;
        QTest::newRow("argumentsFillTheirPlaceholders")
            << MakeNotice([]() -> Notice {
                   return { "Test", "Failed to open %1: %2", Severity::Info, Delivery::Inline, { "a.txt", "denied" } };
               })
            << u"Failed to open a.txt: denied"_s;
        QTest::newRow("placeholdersMayRepeatAndReorder")
            << MakeNotice([]() -> Notice { return { "Test", "%2 %1 %2", Severity::Info, Delivery::Inline, { "one", "two" } }; })
            << u"two one two"_s;
        // Substituted text is never scanned again, so a path holding `%1` stays as it is
        QTest::newRow("substitutedTextIsNotRescanned")
            << MakeNotice([]() -> Notice { return { "Test", "%1|%2", Severity::Info, Delivery::Inline, { "%2", "x" } }; })
            << u"%2|x"_s;
        QTest::newRow("unknownPlaceholdersAreKept")
            << MakeNotice([]() -> Notice {
                   return { "Test", "%1 %2 %0 %x 50%", Severity::Info, Delivery::Inline, { "a" } };
               })
            << u"a %2 %0 %x 50%"_s;
        QTest::newRow("twoDigitPlaceholders")
            << MakeNotice([]() -> Notice {
                   return { "Test",
                            "%12|%1",
                            Severity::Info,
                            Delivery::Inline,
                            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "twelve" } };
               })
            << u"twelve|1"_s;
        QTest::newRow("countReplacesPercentN")
            << MakeNotice([]() -> Notice {
                   return { "Test", "%n file(s) in %1", Severity::Info, Delivery::Inline, { "dir" }, 3 };
               })
            << u"3 file(s) in dir"_s;
        QTest::newRow("zeroIsACount")
            << MakeNotice([]() -> Notice { return { "Test", "%n left", Severity::Info, Delivery::Inline, {}, 0 }; })
            << u"0 left"_s;
        QTest::newRow("negativeCountLeavesPercentN")
            << MakeNotice([]() -> Notice { return { "Test", "%n left, %1", Severity::Info, Delivery::Inline, { "a" } }; })
            << u"%n left, a"_s;
    }

    void english() {
        QFETCH(const MakeNotice, make);
        QFETCH(const QString, expected);

        QCOMPARE(make().english(), expected);
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

        ModalCatcher box;
        present(nullptr, notice);

        QVERIFY(box.seen.isEmpty());
        QVERIFY(box.text.isEmpty());
    }

    void aStatusNoticeGoesToTheStatusBarOfTheWindow() {
        QMainWindow window;
        auto* const child = new QWidget(&window);
        const Notice notice("Test", "Saved %1 files", Severity::Info, Delivery::Status, { "3" });

        // Any widget of the window finds its status bar
        present(child, notice);

        QCOMPARE(window.statusBar()->currentMessage(), u"Saved 3 files"_s);
    }

    void aStatusNoticeWithoutAStatusBarIsModal() {
        ShownBox shown;
        captureModal(shown);

        present(nullptr, Notice("Test", "No bar", Severity::Info, Delivery::Status));

        QVERIFY(shown.found);
        QCOMPARE(shown.text, u"No bar"_s);
    }

    void aStatusNoticeOnAWidgetOutsideAMainWindowIsModal() {
        QWidget plain;
        ShownBox shown;
        captureModal(shown);

        present(&plain, Notice("Test", "Plain window", Severity::Info, Delivery::Status));

        QVERIFY(shown.found);
    }

    void modalNoticesShowTheirSeverity_data() {
        QTest::addColumn<Severity>("severity");
        QTest::addColumn<QMessageBox::Icon>("icon");
        QTest::addColumn<QString>("title");

        QTest::newRow("debug") << Severity::Debug << QMessageBox::Information << u"Information"_s;
        QTest::newRow("info") << Severity::Info << QMessageBox::Information << u"Information"_s;
        QTest::newRow("warning") << Severity::Warning << QMessageBox::Warning << u"Warning"_s;
        QTest::newRow("critical") << Severity::Critical << QMessageBox::Critical << u"Error"_s;
    }

    void modalNoticesShowTheirSeverity() {
        QFETCH(const Severity, severity);
        QFETCH(const QMessageBox::Icon, icon);
        QFETCH(const QString, title);

        ShownBox shown;
        captureModal(shown);

        present(nullptr, Notice("Test", "Broke %1", severity, Delivery::Modal, { "it" }));

        QVERIFY(shown.found);
        QCOMPARE(shown.icon, icon);
        QCOMPARE(shown.title, title);
        QCOMPARE(shown.text, u"Broke it"_s);
    }
};

QTEST_MAIN(TestNotice)
#include "tst_notice.moc"
