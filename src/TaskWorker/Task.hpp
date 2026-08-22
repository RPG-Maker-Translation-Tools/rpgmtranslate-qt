#pragma once

#include "Aliases.hpp"

#include <QMetaType>
#include <QString>

enum class TaskKind : u8 {
    Read,
    Write,
    Purge,
    ExtractArchive,
    Search,
    Replace,
    ReplaceSingle,
    BatchScript,
    BatchTranslate,
    GlobalLint,
    SerdeExport,
    SerdeImport,
};

enum class TaskAccess : u8 {
    Reads,
    Rewrites,
};

[[nodiscard]] auto taskName(TaskKind kind) -> QString;

[[nodiscard]] constexpr auto taskAccess(const TaskKind kind) -> TaskAccess {
    switch (kind) {
        case TaskKind::Write:
        case TaskKind::Search:
        case TaskKind::GlobalLint:
        case TaskKind::SerdeExport:
            return TaskAccess::Reads;
        default:
            return TaskAccess::Rewrites;
    }
}

[[nodiscard]] constexpr auto tasksConflict(const TaskKind lhs, const TaskKind rhs) -> bool {
    return lhs == rhs || taskAccess(lhs) == TaskAccess::Rewrites || taskAccess(rhs) == TaskAccess::Rewrites;
}

[[nodiscard]] constexpr auto taskInterruptible(const TaskKind kind) -> bool {
    switch (kind) {
        case TaskKind::Search:
        case TaskKind::Replace:
        case TaskKind::BatchScript:
        case TaskKind::BatchTranslate:
        case TaskKind::GlobalLint:
        case TaskKind::SerdeExport:
        case TaskKind::SerdeImport:
            return true;
        default:
            return false;
    }
}

class Task final {
   public:
    Task(const u32 id_, const TaskKind kind) : taskId(id_), taskKind(kind) {}

    Task(const Task&) = delete;
    auto operator=(const Task&) -> Task& = delete;

    [[nodiscard]] auto id() const -> u32 { return taskId; }

    [[nodiscard]] auto kind() const -> TaskKind { return taskKind; }

    [[nodiscard]] auto interrupted() const -> bool { return interruptFlag.load(std::memory_order_relaxed); }

    void interrupt() { interruptFlag.store(true, std::memory_order_relaxed); }

   private:
    const u32 taskId;
    const TaskKind taskKind;
    atomicBool interruptFlag = false;
};

using TaskToken = shared_ptr<Task>;

extern "C" auto taskInterrupted(void* context) -> bool;
