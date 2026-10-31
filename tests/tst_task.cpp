#include "Task.hpp"

#include <QtTest>

namespace {
// `Statistics` is the last kind: add the new ones there and in the tables below
constexpr u8 TASK_KIND_COUNT = static_cast<u8>(TaskKind::Statistics) + 1;

constexpr auto kindAt(const u8 idx) -> TaskKind {
    return static_cast<TaskKind>(idx);
}

// What the application relies on, spelled out: two tasks may run together only if both only read the project, and they
// are not the same kind of task.
constexpr auto conflictsAreSymmetric() -> bool {
    for (u8 lhs = 0; lhs < TASK_KIND_COUNT; lhs++) {
        for (u8 rhs = 0; rhs < TASK_KIND_COUNT; rhs++) {
            if (tasksConflict(kindAt(lhs), kindAt(rhs)) != tasksConflict(kindAt(rhs), kindAt(lhs))) {
                return false;
            }
        }
    }

    return true;
}

constexpr auto everyKindConflictsWithItself() -> bool {
    for (u8 idx = 0; idx < TASK_KIND_COUNT; idx++) {
        if (!tasksConflict(kindAt(idx), kindAt(idx))) {
            return false;
        }
    }

    return true;
}

constexpr auto rewritingKindsConflictWithEverything() -> bool {
    for (u8 lhs = 0; lhs < TASK_KIND_COUNT; lhs++) {
        if (taskAccess(kindAt(lhs)) != TaskAccess::Rewrites) {
            continue;
        }

        for (u8 rhs = 0; rhs < TASK_KIND_COUNT; rhs++) {
            if (!tasksConflict(kindAt(lhs), kindAt(rhs))) {
                return false;
            }
        }
    }

    return true;
}

constexpr auto differentReadingKindsCoexist() -> bool {
    for (u8 lhs = 0; lhs < TASK_KIND_COUNT; lhs++) {
        for (u8 rhs = 0; rhs < TASK_KIND_COUNT; rhs++) {
            const bool bothRead =
                taskAccess(kindAt(lhs)) == TaskAccess::Reads && taskAccess(kindAt(rhs)) == TaskAccess::Reads;

            if (bothRead && lhs != rhs && tasksConflict(kindAt(lhs), kindAt(rhs))) {
                return false;
            }
        }
    }

    return true;
}

static_assert(conflictsAreSymmetric());
static_assert(everyKindConflictsWithItself());
static_assert(rewritingKindsConflictWithEverything());
static_assert(differentReadingKindsCoexist());
}  // namespace

class TestTask final : public QObject {
    Q_OBJECT

   private slots:
    void access_data() {
        QTest::addColumn<TaskKind>("kind");
        QTest::addColumn<bool>("reads");

        QTest::newRow("Read") << TaskKind::Read << false;
        QTest::newRow("Write") << TaskKind::Write << true;
        QTest::newRow("Purge") << TaskKind::Purge << false;
        QTest::newRow("ExtractArchive") << TaskKind::ExtractArchive << false;
        QTest::newRow("Search") << TaskKind::Search << true;
        QTest::newRow("Replace") << TaskKind::Replace << false;
        QTest::newRow("ReplaceSingle") << TaskKind::ReplaceSingle << false;
        QTest::newRow("BatchScript") << TaskKind::BatchScript << false;
        QTest::newRow("GlobalLint") << TaskKind::GlobalLint << true;
        QTest::newRow("SerdeExport") << TaskKind::SerdeExport << true;
        QTest::newRow("SerdeImport") << TaskKind::SerdeImport << false;
        QTest::newRow("Statistics") << TaskKind::Statistics << true;
    }

    // A task that changes the translation files must never run beside another task: a kind listed here as reading
    // would let a search or a lint see half-written files.
    void access() {
        QFETCH(TaskKind, kind);
        QFETCH(bool, reads);

        QCOMPARE(taskAccess(kind), reads ? TaskAccess::Reads : TaskAccess::Rewrites);
    }

    void interruptible_data() {
        QTest::addColumn<TaskKind>("kind");
        QTest::addColumn<bool>("interruptible");

        QTest::newRow("Read") << TaskKind::Read << false;
        QTest::newRow("Write") << TaskKind::Write << false;
        QTest::newRow("Purge") << TaskKind::Purge << false;
        QTest::newRow("ExtractArchive") << TaskKind::ExtractArchive << false;
        QTest::newRow("Search") << TaskKind::Search << true;
        QTest::newRow("Replace") << TaskKind::Replace << true;
        QTest::newRow("ReplaceSingle") << TaskKind::ReplaceSingle << false;
        QTest::newRow("BatchScript") << TaskKind::BatchScript << true;
        QTest::newRow("GlobalLint") << TaskKind::GlobalLint << true;
        QTest::newRow("SerdeExport") << TaskKind::SerdeExport << true;
        QTest::newRow("SerdeImport") << TaskKind::SerdeImport << true;
        QTest::newRow("Statistics") << TaskKind::Statistics << true;
    }

    void interruptible() {
        QFETCH(TaskKind, kind);
        QFETCH(bool, interruptible);

        QCOMPARE(taskInterruptible(kind), interruptible);
    }

    void readersOfDifferentKindsCoexist() {
        QVERIFY(!tasksConflict(TaskKind::Search, TaskKind::GlobalLint));
        QVERIFY(!tasksConflict(TaskKind::Write, TaskKind::Statistics));
        // Two of one kind still conflict: a second search would race the first one's results
        QVERIFY(tasksConflict(TaskKind::Search, TaskKind::Search));
    }

    void rewritersBlockReaders() {
        QVERIFY(tasksConflict(TaskKind::Replace, TaskKind::Search));
        QVERIFY(tasksConflict(TaskKind::Search, TaskKind::Replace));
        QVERIFY(tasksConflict(TaskKind::Read, TaskKind::Write));
    }

    void taskKeepsItsIdentityAndInterruptFlag() {
        Task task(7, TaskKind::Search);

        QCOMPARE(task.id(), 7U);
        QCOMPARE(task.kind(), TaskKind::Search);
        QVERIFY(!task.interrupted());

        task.interrupt();

        QVERIFY(task.interrupted());
        // Interrupting twice is harmless
        task.interrupt();
        QVERIFY(task.interrupted());
        QCOMPARE(task.kind(), TaskKind::Search);
    }
};

QTEST_APPLESS_MAIN(TestTask)

#include "tst_task.moc"
