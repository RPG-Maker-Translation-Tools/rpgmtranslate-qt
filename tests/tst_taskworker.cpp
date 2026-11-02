#include "TaskWorker.hpp"

#include <QSignalSpy>
#include <QtTest>

class TestTaskWorker final : public QObject {
    Q_OBJECT

   private slots:
    void startsIdle() {
        TaskWorker worker;

        QVERIFY(worker.idle());
        QVERIFY(worker.runningTask(TaskKind::Search) == nullptr);
        QVERIFY(worker.uninterruptibleTask() == nullptr);
        QVERIFY(worker.blockingTask(TaskAccess::Rewrites) == nullptr);
        QVERIFY(!worker.blocks(TaskKind::Read));
    }

    void readersRunTogetherButNotTwiceOfAKind() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        const auto lint = worker.startTask(TaskKind::GlobalLint);

        QVERIFY(search.has_value());
        QVERIFY(lint.has_value());
        QVERIFY(!worker.idle());
        // Tasks are numbered in the order they start
        QCOMPARE((*lint)->id(), (*search)->id() + 1);

        const auto again = worker.startTask(TaskKind::Search);

        QVERIFY(!again.has_value());
        QCOMPARE(again.error(), TaskKind::Search);
    }

    // A refusal names the running task in the way, which the window shows to the user
    void rewriterIsRefusedWhileAnythingRuns() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        QVERIFY(search.has_value());

        const auto replace = worker.startTask(TaskKind::Replace);

        QVERIFY(!replace.has_value());
        QCOMPARE(replace.error(), TaskKind::Search);
        // A refused task is not left running
        QVERIFY(worker.runningTask(TaskKind::Replace) == nullptr);
    }

    void readerIsRefusedWhileARewriterRuns() {
        TaskWorker worker;

        const auto replace = worker.startTask(TaskKind::Replace);
        QVERIFY(replace.has_value());

        const auto search = worker.startTask(TaskKind::Search);

        QVERIFY(!search.has_value());
        QCOMPARE(search.error(), TaskKind::Replace);
    }

    void blocksMatchesWhatStartWouldRefuse() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        QVERIFY(search.has_value());

        QVERIFY(worker.blocks(TaskKind::Search));
        QVERIFY(worker.blocks(TaskKind::Replace));
        QVERIFY(!worker.blocks(TaskKind::Write));
        QVERIFY(!worker.blocks(TaskKind::Statistics));
    }

    void blockingTaskDependsOnTheAccessAsked() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        QVERIFY(search.has_value());

        // Something that only reads waits for no reader, but whatever rewrites waits for every task
        QVERIFY(worker.blockingTask(TaskAccess::Reads) == nullptr);
        QVERIFY(worker.blockingTask(TaskAccess::Rewrites) == *search);

        worker.finishTask(*search);

        const auto replace = worker.startTask(TaskKind::Replace);
        QVERIFY(replace.has_value());

        QVERIFY(worker.blockingTask(TaskAccess::Reads) == *replace);
        QVERIFY(worker.blockingTask(TaskAccess::Rewrites) == *replace);
    }

    void runningTaskFindsByKind() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        const auto lint = worker.startTask(TaskKind::GlobalLint);
        QVERIFY(search.has_value() && lint.has_value());

        QVERIFY(worker.runningTask(TaskKind::Search) == *search);
        QVERIFY(worker.runningTask(TaskKind::GlobalLint) == *lint);
        QVERIFY(worker.runningTask(TaskKind::Write) == nullptr);
    }

    void finishingFreesTheWayAndSaysSo() {
        TaskWorker worker;
        const QSignalSpy finished(&worker, &TaskWorker::taskFinished);

        const auto search = worker.startTask(TaskKind::Search);
        const auto lint = worker.startTask(TaskKind::GlobalLint);
        QVERIFY(search.has_value() && lint.has_value());

        const u32 searchId = (*search)->id();
        worker.finishTask(*search);

        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished.at(0).at(0).toUInt(), searchId);
        QVERIFY(worker.runningTask(TaskKind::Search) == nullptr);
        QVERIFY(!worker.idle());

        // The lint still runs, so a rewriter is still refused, and names it
        const auto replace = worker.startTask(TaskKind::Replace);
        QVERIFY(!replace.has_value());
        QCOMPARE(replace.error(), TaskKind::GlobalLint);

        worker.finishTask(*lint);

        QVERIFY(worker.idle());
        QVERIFY(worker.startTask(TaskKind::Replace).has_value());
    }

    void startingAnnouncesTheTask() {
        TaskWorker worker;
        const QSignalSpy started(&worker, &TaskWorker::taskStarted);

        const auto search = worker.startTask(TaskKind::Search);
        QVERIFY(search.has_value());
        QCOMPARE(started.count(), 1);

        // A refused task is not announced
        QVERIFY(!worker.startTask(TaskKind::Search).has_value());
        QCOMPARE(started.count(), 1);
    }

    void uninterruptibleTaskIsFoundWhileItRuns() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        QVERIFY(search.has_value());
        QVERIFY(worker.uninterruptibleTask() == nullptr);

        worker.finishTask(*search);

        const auto read = worker.startTask(TaskKind::Read);
        QVERIFY(read.has_value());
        QVERIFY(worker.uninterruptibleTask() == *read);
    }

    void interruptAllSignalsEveryRunningTask() {
        TaskWorker worker;

        const auto search = worker.startTask(TaskKind::Search);
        const auto lint = worker.startTask(TaskKind::GlobalLint);
        QVERIFY(search.has_value() && lint.has_value());
        QVERIFY(!(*search)->interrupted());
        QVERIFY(!(*lint)->interrupted());

        worker.interruptAll();

        QVERIFY((*search)->interrupted());
        QVERIFY((*lint)->interrupted());
    }

    void glossaryKeepsOnlyUsableTerms() {
        Term complete;
        complete.term = u"HP"_s;
        complete.translation = u"Health"_s;

        Term disabled = complete;
        disabled.enabled = false;

        Term noTerm = complete;
        noTerm.term.clear();

        // A row that was added and never filled in
        Term noTranslation = complete;
        noTranslation.translation.clear();

        Term second = complete;
        second.term = u"\u9B54\u529B"_s;
        second.translation = u"MP"_s;

        const Glossary glossary{ complete, disabled, noTerm, noTranslation, second };
        const PreparedGlossary prepared = prepareGlossary(glossary);

        QCOMPARE(prepared.size(), usize{ 2 });
        QVERIFY(prepared[0].term == &glossary[0]);
        QCOMPARE(prepared[0].termUtf8, QByteArray("HP"));
        QCOMPARE(prepared[0].translationUtf8, QByteArray("Health"));
        QVERIFY(prepared[1].term == &glossary[4]);
        QCOMPARE(prepared[1].termUtf8, u"\u9B54\u529B"_s.toUtf8());

        QVERIFY(prepareGlossary(Glossary{}).empty());
    }

    void noCustomLintsGiveAnEmptyMatcher() {
        TaskWorker worker;
        Settings settings;
        worker.init(&settings, nullptr, nullptr);

        const CustomLintMatcher matcher = worker.customLintMatcher();

        QVERIFY(matcher.indices.empty());
        QVERIFY(matcher.regexp.pattern().isEmpty());
    }

    void customLintMatcherSkipsDisabledLints() {
        TaskWorker worker;
        Settings settings;
        settings.translation.customLints = { CustomLint{ .sequence = u"foo"_s },
                                             CustomLint{ .sequence = u"bar"_s, .disabled = true },
                                             CustomLint{ .sequence = u"baz"_s } };
        worker.init(&settings, nullptr, nullptr);

        const CustomLintMatcher matcher = worker.customLintMatcher();

        QCOMPARE(matcher.indices, (vector<i32>{ 0, 2 }));
        QVERIFY2(matcher.regexp.isValid(), qPrintable(matcher.regexp.errorString()));

        // Capture group N + 1 belongs to the lint at `indices[N]`
        const QRegularExpressionMatch foo = matcher.regexp.match(u"a foo"_s);
        QVERIFY(foo.hasMatch());
        QVERIFY(foo.hasCaptured(1));
        QVERIFY(!foo.hasCaptured(2));

        const QRegularExpressionMatch baz = matcher.regexp.match(u"a baz"_s);
        QVERIFY(baz.hasCaptured(2));

        QVERIFY(!matcher.regexp.match(u"a bar"_s).hasMatch());
    }

    // The groups a user writes in a sequence must not shift the numbering of the lints
    void customLintMatcherIgnoresTheUsersOwnGroups() {
        TaskWorker worker;
        Settings settings;
        settings.translation.customLints = { CustomLint{ .sequence = u"(a|b)c"_s }, CustomLint{ .sequence = u"(x)(y)z"_s } };
        worker.init(&settings, nullptr, nullptr);

        const CustomLintMatcher matcher = worker.customLintMatcher();
        QVERIFY2(matcher.regexp.isValid(), qPrintable(matcher.regexp.errorString()));

        const QRegularExpressionMatch second = matcher.regexp.match(u"xyz"_s);
        QVERIFY(second.hasCaptured(2));
        QVERIFY(!second.hasCaptured(1));
        QVERIFY(!second.hasCaptured(3));
    }

    void customLintMatcherHonoursCaseInsensitivity() {
        TaskWorker worker;
        Settings settings;
        settings.translation.customLints = { CustomLint{ .sequence = u"exact"_s },
                                             CustomLint{ .sequence = u"loose"_s, .caseInsensitive = true } };
        worker.init(&settings, nullptr, nullptr);

        const CustomLintMatcher matcher = worker.customLintMatcher();
        QVERIFY2(matcher.regexp.isValid(), qPrintable(matcher.regexp.errorString()));

        QVERIFY(matcher.regexp.match(u"LOOSE"_s).hasCaptured(2));
        QVERIFY(matcher.regexp.match(u"Loose"_s).hasCaptured(2));
        // The other lint is still case sensitive: the flag does not leak into it
        QVERIFY(matcher.regexp.match(u"exact"_s).hasCaptured(1));
        QVERIFY(!matcher.regexp.match(u"EXACT"_s).hasMatch());
    }
};

QTEST_MAIN(TestTaskWorker)

#include "tst_taskworker.moc"
