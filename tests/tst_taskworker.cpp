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
};

QTEST_MAIN(TestTaskWorker)

#include "tst_taskworker.moc"
