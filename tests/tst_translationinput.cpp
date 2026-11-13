#include "TranslationInput.hpp"

#include "ProjectSettings.hpp"
#include "Settings.hpp"

#include <QApplication>
#include <QSignalSpy>
#include <QtTest>

Q_DECLARE_METATYPE(LintType)

namespace {
struct Fixture {
    Fixture() : input(&projectSettings, &settings) {
        // The replacements of a new installation are all off
        for (auto& replacement : settings.translation.replacements) {
            replacement.enabled = true;
        }

        input.resize(400, 120);
        input.show();
    }

    [[nodiscard]] auto text() const -> QString { return input.toPlainText(); }

    void type(const QString& keys) { QTest::keyClicks(&input, keys); }

    Settings settings;
    ProjectSettings projectSettings;
    TranslationInput input;
};

// How red the most red pixel of the first line is: the line-length hint is drawn in red, translucent, over the text
auto reddest(const QImage& image, const i32 firstLines) -> i32 {
    i32 best = 0;

    for (i32 yPos = 0; yPos < firstLines && yPos < image.height(); yPos++) {
        for (i32 xPos = 0; xPos < image.width(); xPos++) {
            const QColor pixel = image.pixelColor(xPos, yPos);
            best = max(best, pixel.red() - ((pixel.green() + pixel.blue()) / 2));
        }
    }

    return best;
}

// Where the hint line is drawn: the column of the reddest pixel of the first lines
auto hintColumn(const QImage& image) -> i32 {
    i32 best = 0;
    i32 column = -1;

    for (i32 yPos = 0; yPos < 20 && yPos < image.height(); yPos++) {
        for (i32 xPos = 0; xPos < image.width(); xPos++) {
            const QColor pixel = image.pixelColor(xPos, yPos);
            const i32 red = pixel.red() - ((pixel.green() + pixel.blue()) / 2);

            if (red > best) {
                best = red;
                column = xPos;
            }
        }
    }

    return column;
}
}  // namespace

class TestTranslationInput final : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() { qRegisterMetaType<LintType>(); }

    // Auto-replacements
    void typingASequenceReplacesIt() {
        Fixture fixture;

        fixture.type(u"a<<b>>c--d"_s);

        QCOMPARE(fixture.text(), u"a«" "b»" "c—" "d"_s);
    }

    void backspaceBringsTheReplacedSequenceBack() {
        Fixture fixture;
        fixture.type(u"<<"_s);
        QCOMPARE(fixture.text(), u"«"_s);

        QTest::keyClick(&fixture.input, Qt::Key_Backspace);

        QCOMPARE(fixture.text(), u"<<"_s);

        // Only the first backspace brings it back; the next one deletes as usual
        QTest::keyClick(&fixture.input, Qt::Key_Backspace);
        QCOMPARE(fixture.text(), u"<"_s);
    }

    void backspaceDeletesNormallyAwayFromAReplacement() {
        Fixture fixture;
        fixture.type(u"<<x"_s);

        QTest::keyClick(&fixture.input, Qt::Key_Backspace);

        QCOMPARE(fixture.text(), u"«"_s);
    }

    void disabledReplacementsAreLeftAlone() {
        Fixture fixture;
        for (auto& replacement : fixture.settings.translation.replacements) {
            replacement.enabled = false;
        }

        fixture.type(u"<<--"_s);

        QCOMPARE(fixture.text(), u"<<--"_s);
    }

    void deletingTextDoesNotBringAReplacementBack() {
        Fixture fixture;
        fixture.input.setPlainText(u"<<<"_s);
        QTest::keyClick(&fixture.input, Qt::Key_End, Qt::ControlModifier);

        // The deletion leaves `<<` before the cursor, which typing would have replaced
        QTest::keyClick(&fixture.input, Qt::Key_Backspace);

        QCOMPARE(fixture.text(), u"<<"_s);
    }

    void replacingTheTextDoesNotReplaceSequencesInIt() {
        Fixture fixture;

        fixture.input.replaceText(u"<< -- >>"_s);

        QCOMPARE(fixture.text(), u"<< -- >>"_s);
    }

    void replacingTheTextIsOneUndoableEdit() {
        Fixture fixture;
        fixture.input.setPlainText(u"first"_s);
        fixture.input.document()->clearUndoRedoStacks();

        fixture.input.replaceText(u"second"_s);
        QCOMPARE(fixture.text(), u"second"_s);

        fixture.input.undo();

        QCOMPARE(fixture.text(), u"first"_s);
    }

    // Keys
    void returnAndEscapeLeaveTheInputWithoutTypingAnything() {
        Fixture fixture;
        fixture.type(u"text"_s);

        for (const Qt::Key key : { Qt::Key_Return, Qt::Key_Enter, Qt::Key_Escape }) {
            fixture.input.activateWindow();
            fixture.input.setFocus();
            QTRY_VERIFY(fixture.input.hasFocus());
            QTest::keyClick(&fixture.input, key);
            QCOMPARE(fixture.text(), u"text"_s);
            QVERIFY(!fixture.input.hasFocus());
        }
    }

    void controlReturnAsksForTheNextInput() {
        Fixture fixture;
        QSignalSpy next(&fixture.input, &TranslationInput::focusNextInput);

        QTest::keyClick(&fixture.input, Qt::Key_Return);
        QCOMPARE(next.size(), qsizetype{ 0 });

        QTest::keyClick(&fixture.input, Qt::Key_Return, Qt::ControlModifier);
        QCOMPARE(next.size(), qsizetype{ 1 });
    }

    void shiftReturnStartsANewLine() {
        Fixture fixture;
        fixture.type(u"a"_s);

        QTest::keyClick(&fixture.input, Qt::Key_Return, Qt::ShiftModifier);
        fixture.type(u"b"_s);

        QCOMPARE(fixture.text(), u"a\nb"_s);
    }

    // The height
    void theContentHeightFollowsTheLines() {
        Fixture fixture;
        QSignalSpy heights(&fixture.input, &TranslationInput::contentHeightChanged);
        const u32 lineHeight = QFontMetrics(fixture.input.font()).lineSpacing();

        fixture.input.setPlainText(u"one\ntwo\nthree"_s);

        QVERIFY(!heights.isEmpty());
        QCOMPARE(heights.last().at(0).toUInt(), (3 * lineHeight) + 10);

        // The same height is not reported again
        const qsizetype reported = heights.size();
        fixture.input.setPlainText(u"1\n2\n3"_s);
        QCOMPARE(heights.size(), reported);
    }

    void textChangedIsReportedOncePerEdit() {
        Fixture fixture;
        QSignalSpy changed(&fixture.input, &QPlainTextEdit::textChanged);

        fixture.type(u"ab"_s);

        QCOMPARE(changed.size(), qsizetype{ 2 });
    }

    // Lints
    void lintFormatsReachTheTextAfterTheNextEventLoop() {
        Fixture fixture;
        fixture.input.setPlainText(u"abc def"_s);

        vector<LintCharState> states(7);
        for (i32 idx = 0; idx < 3; idx++) {
            states[idx].background = QColor(Qt::yellow);
            states[idx].entries.append(LintEntry{ .captured = u"abc"_s, .index = 0, .type = LintType::Misc });
        }

        fixture.input.applyLintFormats(std::move(states), 7);
        QTest::qWait(20);

        const QList<QTextLayout::FormatRange> formats = fixture.input.document()->firstBlock().layout()->formats();
        QVERIFY(!formats.isEmpty());
        QCOMPARE(formats.first().start, 0);
        QCOMPARE(formats.first().length, 3);
        QCOMPARE(formats.first().format.background().color(), QColor(Qt::yellow));
    }

    void applyingLintFormatsDoesNotReportATextChange() {
        Fixture fixture;
        fixture.input.setPlainText(u"abc"_s);
        QSignalSpy changed(&fixture.input, &QPlainTextEdit::textChanged);

        vector<LintCharState> states(3);
        states[0].background = QColor(Qt::red);
        fixture.input.applyLintFormats(std::move(states), 3);
        QTest::qWait(20);

        QCOMPARE(changed.size(), qsizetype{ 0 });
    }

    // The tooltip of a lint
    void hoveringALintShowsItsTooltipAndASuggestionReplacesTheText() {
        Fixture fixture;
        fixture.input.setPlainText(u"a wrng word"_s);

        vector<LintCharState> states(11);
        for (i32 idx = 2; idx < 6; idx++) {
            states[idx].underlineStyle = QTextCharFormat::SpellCheckUnderline;
            states[idx].underlineColor = Qt::red;
            states[idx].entries.append(
                LintEntry{ .extra = u"Possible typo"_s,
                           .suggestions = QStringList{ u"wrong"_s, u"wring"_s },
                           .captured = u"wrng"_s,
                           .translationId = nullptr,
                           .index = 0,
                           .type = LintType::LanguageToolCheck }
            );
        }

        fixture.input.applyLintFormats(std::move(states), 11);
        QTest::qWait(20);

        // The pointer moves onto the lint
        QTextCursor onLint = fixture.input.textCursor();
        onLint.setPosition(3);
        const QPoint where = fixture.input.cursorRect(onLint).center();
        QMouseEvent move(QEvent::MouseMove, where, fixture.input.viewport()->mapToGlobal(where), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(fixture.input.viewport(), &move);

        // Not at once: a mouse crossing the text would flash a tooltip for each lint
        LintTooltip* tooltip = nullptr;
        const auto findTooltip = [&tooltip]() -> bool {
            for (QWidget* const widget : QApplication::topLevelWidgets()) {
                if (auto* const found = qobject_cast<LintTooltip*>(widget); found != nullptr && found->isVisible()) {
                    tooltip = found;
                    return true;
                }
            }

            return false;
        };
        QVERIFY(!findTooltip());
        QTRY_VERIFY_WITH_TIMEOUT(findTooltip(), 4000);

        emit tooltip->suggestionActivated(LintType::LanguageToolCheck, 0, 1);

        QCOMPARE(fixture.text(), u"a wring word"_s);
        QVERIFY(!tooltip->isVisible());
    }

    void suggestionsOutsideTheOfferedOnesChangeNothing() {
        Fixture fixture;
        fixture.input.setPlainText(u"a wrng word"_s);

        vector<LintCharState> states(11);
        for (i32 idx = 2; idx < 6; idx++) {
            states[idx].entries.append(
                LintEntry{ .suggestions = QStringList{ u"wrong"_s },
                           .captured = u"wrng"_s,
                           .translationId = nullptr,
                           .index = 0,
                           .type = LintType::LanguageToolCheck }
            );
            states[idx].underlineStyle = QTextCharFormat::SpellCheckUnderline;
            states[idx].underlineColor = Qt::red;
        }

        fixture.input.applyLintFormats(std::move(states), 11);
        QTest::qWait(20);

        QTextCursor onLint = fixture.input.textCursor();
        onLint.setPosition(3);
        const QPoint where = fixture.input.cursorRect(onLint).center();
        QMouseEvent move(QEvent::MouseMove, where, fixture.input.viewport()->mapToGlobal(where), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(fixture.input.viewport(), &move);

        LintTooltip* tooltip = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ([&tooltip]() -> bool {
                for (QWidget* const widget : QApplication::topLevelWidgets()) {
                    if (auto* const found = qobject_cast<LintTooltip*>(widget); found != nullptr && found->isVisible()) {
                        tooltip = found;
                        return true;
                    }
                }

                return false;
            })(),
            4000
        );

        emit tooltip->suggestionActivated(LintType::LanguageToolCheck, 0, 5);
        emit tooltip->suggestionActivated(LintType::LanguageToolCheck, 0, -1);
        emit tooltip->suggestionActivated(LintType::Misc, 3, 0);

        QCOMPARE(fixture.text(), u"a wrng word"_s);
    }

    void leavingTheInputBeforeTheDelayShowsNoTooltip() {
        Fixture fixture;
        fixture.input.setPlainText(u"a wrng word"_s);

        vector<LintCharState> states(11);
        states[3].entries.append(LintEntry{ .translationId = nullptr, .index = 0, .type = LintType::LanguageToolCheck });
        states[3].underlineStyle = QTextCharFormat::SpellCheckUnderline;
        states[3].underlineColor = Qt::red;
        fixture.input.applyLintFormats(std::move(states), 11);
        QTest::qWait(20);

        QTextCursor onLint = fixture.input.textCursor();
        onLint.setPosition(3);
        const QPoint where = fixture.input.cursorRect(onLint).center();
        QMouseEvent move(QEvent::MouseMove, where, fixture.input.viewport()->mapToGlobal(where), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(fixture.input.viewport(), &move);

        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&fixture.input, &leave);
        QTest::qWait(1500);

        for (QWidget* const widget : QApplication::topLevelWidgets()) {
            QVERIFY(qobject_cast<LintTooltip*>(widget) == nullptr || !widget->isVisible());
        }
    }

    void aTooltipOutlivesTheMouseLeavingAndAnAppliedSuggestionIsNotAppliedAgain() {
        Fixture fixture;
        fixture.input.setPlainText(u"a wrng word"_s);

        vector<LintCharState> states(11);
        for (i32 idx = 2; idx < 6; idx++) {
            states[idx].underlineStyle = QTextCharFormat::SpellCheckUnderline;
            states[idx].underlineColor = Qt::red;
            states[idx].entries.append(
                LintEntry{ .suggestions = QStringList{ u"wrong"_s, u"wring"_s },
                           .captured = u"wrng"_s,
                           .translationId = nullptr,
                           .index = 0,
                           .type = LintType::LanguageToolCheck }
            );
        }

        fixture.input.applyLintFormats(std::move(states), 11);
        QTest::qWait(20);

        const auto moveTo = [&fixture](const i32 position) -> void {
            QTextCursor cursor = fixture.input.textCursor();
            cursor.setPosition(position);
            const QPoint where = fixture.input.cursorRect(cursor).center();
            QMouseEvent move(QEvent::MouseMove, where, fixture.input.viewport()->mapToGlobal(where), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(fixture.input.viewport(), &move);
        };

        // Two moves inside one lint are one hover
        moveTo(3);
        moveTo(4);

        LintTooltip* tooltip = nullptr;
        const auto findTooltip = [&tooltip]() -> bool {
            for (QWidget* const widget : QApplication::topLevelWidgets()) {
                if (auto* const found = qobject_cast<LintTooltip*>(widget); found != nullptr && found->isVisible()) {
                    tooltip = found;
                    return true;
                }
            }

            return false;
        };
        QTRY_VERIFY_WITH_TIMEOUT(findTooltip(), 4000);

        // The mouse goes to the tooltip: it waits for the mouse instead of going away at once
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&fixture.input, &leave);
        QVERIFY(tooltip->isVisible());

        emit tooltip->suggestionActivated(LintType::LanguageToolCheck, 0, 1);
        QCOMPARE(fixture.text(), u"a wring word"_s);

        // The range it replaced is gone, so this one has nowhere to go
        emit tooltip->suggestionActivated(LintType::LanguageToolCheck, 0, 0);
        QCOMPARE(fixture.text(), u"a wring word"_s);
    }

    // Lint formats
    void charactersWithDifferentLintsAreFormattedApart() {
        Fixture fixture;
        fixture.input.setPlainText(u"abcdef"_s);

        const auto entry = [](const QString& captured) -> LintEntry {
            return LintEntry{ .captured = captured, .translationId = nullptr, .index = 0, .type = LintType::Misc };
        };

        vector<LintCharState> states(6);
        states[0].underlineStyle = QTextCharFormat::SpellCheckUnderline;
        states[0].underlineColor = Qt::red;
        states[1].underlineStyle = QTextCharFormat::SpellCheckUnderline;
        states[1].underlineColor = Qt::red;
        states[0].entries.append(entry(u"x"_s));
        states[1].entries.append(entry(u"x"_s));
        // The same look as the characters before it, a different lint
        states[2].underlineStyle = QTextCharFormat::SpellCheckUnderline;
        states[2].underlineColor = Qt::red;
        states[3].underlineStyle = QTextCharFormat::SpellCheckUnderline;
        states[3].underlineColor = Qt::red;
        states[2].entries.append(entry(u"y"_s));
        states[3].entries.append(entry(u"y"_s));
        // One lint more than the one before
        states[4].underlineStyle = QTextCharFormat::SpellCheckUnderline;
        states[4].underlineColor = Qt::red;
        states[4].entries.append(entry(u"y"_s));
        states[4].entries.append(entry(u"z"_s));
        states[5].foreground = QColor(Qt::red);

        fixture.input.applyLintFormats(std::move(states), 6);
        QTest::qWait(20);

        QList<pair<i32, i32>> runs;

        for (const QTextLayout::FormatRange& range : fixture.input.document()->firstBlock().layout()->formats()) {
            runs.append({ range.start, range.length });
        }

        ranges::sort(runs);
        QCOMPARE(runs, (QList<pair<i32, i32>>{ { 0, 2 }, { 2, 2 }, { 4, 1 }, { 5, 1 } }));
    }

    void lintFormatsOfAShorterTextAreNotAppliedToALongerOne() {
        Fixture fixture;
        fixture.input.setPlainText(u"abcdef"_s);

        vector<LintCharState> states(3);
        states[1].foreground = QColor(Qt::red);
        fixture.input.applyLintFormats(std::move(states), 3);
        QTest::qWait(20);

        QVERIFY(fixture.input.document()->firstBlock().layout()->formats().isEmpty());
    }

    // The hint
    void aLineLongerThanTheHintGetsAStrongerLine() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 10;

        fixture.input.setPlainText(u"short"_s);
        const i32 within = reddest(fixture.input.grab().toImage(), 20);

        fixture.input.setPlainText(u"a line that is far longer than the ten characters of the hint"_s);
        const i32 beyond = reddest(fixture.input.grab().toImage(), 20);

        QVERIFY(within > 0);
        QVERIFY(beyond > within + 50);
    }

    // The widths of the fonts are remembered, a handful of them: a font that comes back after more than that were
    // measured gets the width it had
    void theHintLineStaysWhereItWasAfterMoreFontsThanTheCacheHoldsWereMeasured() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 15;

        const auto columnsAtSizes = [&fixture] -> QList<i32> {
            QList<i32> columns;

            for (i32 size = 8; size < 20; size++) {
                QFont font = fixture.input.font();
                font.setPointSize(size);
                fixture.input.setFont(font);
                columns.append(hintColumn(fixture.input.grab().toImage()));
            }

            return columns;
        };

        const QList<i32> first = columnsAtSizes();
        const QList<i32> second = columnsAtSizes();

        QCOMPARE(second, first);
        QString shown;

        for (const i32 column : first) {
            shown += QString::number(column) + u' ';
        }

        QVERIFY2(first.last() > first.first(), qPrintable(shown));
    }

    void noHintDrawsNoLine() {
        Fixture fixture;
        const QString text = u"a line that is far longer than the hint would have been"_s;
        fixture.input.setPlainText(text);

        // The antialiasing of the text has coloured fringes of its own, so what is compared is the line that is drawn
        fixture.projectSettings.lineLengthHint = 10;
        const i32 withLine = reddest(fixture.input.grab().toImage(), 20);
        fixture.projectSettings.lineLengthHint = 0;
        const i32 withoutLine = reddest(fixture.input.grab().toImage(), 20);

        QVERIFY2(withLine > 100, qPrintable(QString::number(withLine)));
        QVERIFY2(withoutLine * 4 < withLine, qPrintable(QString::number(withoutLine)));
    }

    // Font size codes change the width of what follows them
    void gameFontSizeCodesCountTowardsTheOverflow() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 20;
        fixture.projectSettings.hintFontSize = 28;

        fixture.input.setPlainText(u"0123456789"_s);
        const i32 plain = reddest(fixture.input.grab().toImage(), 20);

        // Larger glyphs: the same ten characters are wider than twenty small ones
        fixture.input.setPlainText(u"\\fs[96]0123456789"_s);
        const i32 enlarged = reddest(fixture.input.grab().toImage(), 20);

        fixture.input.setPlainText(u"\\{\\{\\{\\{\\{\\{0123456789"_s);
        const i32 grown = reddest(fixture.input.grab().toImage(), 20);

        QVERIFY(enlarged > plain + 50);
        QVERIFY(grown > plain + 50);
    }

    void aHintFontOfTheGameIsUsed() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 10;

        // The platform does not always say that its fixed font is one, so the widths of its letters are compared
        const QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        const QFontMetricsF fixedMetrics(fixed);
        const QFontMetricsF editorMetrics(fixture.input.font());
        const bool narrowIsWide = fixedMetrics.horizontalAdvance(u'i') == fixedMetrics.horizontalAdvance(u'W');
        const bool editorIsNot = editorMetrics.horizontalAdvance(u'i') < editorMetrics.horizontalAdvance(u'W');

        if (!narrowIsWide || !editorIsNot) {
            QSKIP("there is no fixed-width font that differs from the editor's");
        }

        // A run of narrow letters goes over the hint sooner in a font where every letter is as wide as the others than in
        // the editor's, so some length gets the overflow line drawn over the line of the hint with the game's font only
        bool differs = false;

        for (i32 count = 4; count <= 60 && !differs; count++) {
            fixture.input.setPlainText(QString(count, u'i'));

            fixture.projectSettings.hintFont.clear();
            const i32 inTheEditorFont = reddest(fixture.input.grab().toImage(), 20);

            fixture.projectSettings.hintFont = fixed.family();
            differs = reddest(fixture.input.grab().toImage(), 20) > inTheEditorFont;
        }

        QVERIFY2(differs, qPrintable(fixed.family()));
    }

    void aShrinkingCodeNarrowsTheTextAndOnlyTheLinesInViewAreDrawn() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 20;
        fixture.projectSettings.hintFontSize = 28;

        fixture.input.setPlainText(QString(30, u'0'));
        const i32 plain = reddest(fixture.input.grab().toImage(), 20);

        fixture.input.setPlainText(u"\\}"_s + QString(30, u'0'));
        const i32 shrunk = reddest(fixture.input.grab().toImage(), 20);

        QVERIFY(plain > shrunk + 50);

        fixture.input.setPlainText((QString(30, u'0') + u'\n').repeated(300));
        QVERIFY(reddest(fixture.input.grab().toImage(), 20) > 0);
    }

    void moreFontsThanTheWidthCacheHoldsStillDrawTheHint() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 10;

        for (u8 size = 10; size < 24; size++) {
            fixture.projectSettings.hintFontSize = size;
            fixture.input.setPlainText(u"a line that is far longer than the ten characters of the hint"_s);

            QVERIFY(reddest(fixture.input.grab().toImage(), 20) > 100);
        }
    }

    void codesThatDrawNothingAreNotCounted() {
        Fixture fixture;
        fixture.projectSettings.lineLengthHint = 10;

        fixture.input.setPlainText(u"\\C[1]\\V[22]\\N[3]\\_\\\\ab"_s);
        const i32 codes = reddest(fixture.input.grab().toImage(), 20);

        fixture.input.setPlainText(u"ab"_s);
        const i32 plain = reddest(fixture.input.grab().toImage(), 20);

        QVERIFY(codes < plain + 50);
    }
    void onlyWebsiteLinksAreOpened_data() {
        QTest::addColumn<QString>("link");
        QTest::addColumn<bool>("web");

        QTest::newRow("http") << u"http://example.com"_s << true;
        QTest::newRow("https") << u"https://example.com/a?b=c"_s << true;
        QTest::newRow("uppercase scheme") << u"HTTP://example.com"_s << true;
        QTest::newRow("javascript") << u"javascript:alert(1)"_s << false;
        QTest::newRow("file") << u"file:///etc/passwd"_s << false;
        QTest::newRow("scheme as a prefix") << u"httpx://example.com"_s << false;
        QTest::newRow("no scheme") << u"example.com"_s << false;
    }

    void onlyWebsiteLinksAreOpened() {
        QFETCH(QString, link);
        QFETCH(bool, web);

        QCOMPARE(isWebLink(QUrl(link)), web);
    }
};

QTEST_MAIN(TestTranslationInput)
#include "tst_translationinput.moc"
