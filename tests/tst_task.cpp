#include "Task.hpp"

#include <QtTest>

namespace {
constexpr u8 TASK_KIND_COUNT = enchantum::count<TaskKind>;

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

        // The table lists every kind: a new one that is missing here is a failure, not a silent pass
        static constexpr pair<TaskKind, bool> TABLE[] = {
            { TaskKind::Read, false },        { TaskKind::Write, true },        { TaskKind::Purge, false },
            { TaskKind::ExtractArchive, false }, { TaskKind::Search, true },    { TaskKind::Replace, false },
            { TaskKind::ReplaceSingle, false }, { TaskKind::BatchScript, false }, { TaskKind::GlobalLint, true },
            { TaskKind::SerdeExport, true },  { TaskKind::SerdeImport, false }, { TaskKind::Statistics, true },
        };

        for (const auto& [kind, reads] : TABLE) {
            QTest::newRow(enchantum::to_string(kind).data()) << kind << reads;
        }

        for (const TaskKind kind : enchantum::values<TaskKind>) {
            QVERIFY2(ranges::any_of(TABLE, [kind](const auto& row) -> bool { return row.first == kind; }), enchantum::to_string(kind).data());
        }
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

        static constexpr pair<TaskKind, bool> TABLE[] = {
            { TaskKind::Read, false },        { TaskKind::Write, false },       { TaskKind::Purge, false },
            { TaskKind::ExtractArchive, false }, { TaskKind::Search, true },    { TaskKind::Replace, true },
            { TaskKind::ReplaceSingle, false }, { TaskKind::BatchScript, true }, { TaskKind::GlobalLint, true },
            { TaskKind::SerdeExport, true },  { TaskKind::SerdeImport, true },  { TaskKind::Statistics, true },
        };

        for (const auto& [kind, interruptible] : TABLE) {
            QTest::newRow(enchantum::to_string(kind).data()) << kind << interruptible;
        }

        for (const TaskKind kind : enchantum::values<TaskKind>) {
            QVERIFY2(ranges::any_of(TABLE, [kind](const auto& row) -> bool { return row.first == kind; }), enchantum::to_string(kind).data());
        }
    }

    void interruptible() {
        QFETCH(TaskKind, kind);
        QFETCH(bool, interruptible);

        QCOMPARE(taskInterruptible(kind), interruptible);
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
