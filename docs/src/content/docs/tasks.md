---
title: "Tasks"
---

Long-running work (reading a project, writing it back, search, replace, batch script, a global lint check, project statistics, purging, extracting an archive, exporting/importing) runs as a **task**: off the UI thread, tracked centrally, and shown in the task list.

## Where tasks show up

The button in the top bar (reads "No tasks" when idle) opens the task list. Each running task shows its name and a progress bar (`done/total` items). Tasks that support it also get an abort button next to their progress bar.

## Multiple tasks at once

Several tasks can run in parallel, each on its own background thread. What decides whether two may run together is what they touch:

- **Write, Search, Export, Global Check, and Statistics** only read project state, so different ones of these can run simultaneously - search while writing, global-check while writing, etc. Two tasks of the same kind (two searches, for instance) never run at once.
- Every other task (**Read, Purge, Extract archive, Replace, Batch Script, Import**, and replacing or putting a single search result) mutates project state, so it blocks _everything_ else, including another instance of itself, until it finishes.

What a running task would refuse to start is greyed out (Write, Search, Batch, Lint, Statistics, Save, Load Backup, the Read/Purge/Export menu entries and so on) instead of failing with a message. Settings can't be opened during a task, and tabs can't be opened during Read or Purge or while a script, replace or import is rewriting the files.

## Aborting

Search, Replace, Batch Script, Global Check, Statistics, Export, and Import can be aborted mid-run from their abort button in the task list. Read, Write, Purge, and Extract archive can't be interrupted once started - let them finish.

Closing a project or opening another one aborts the abortable tasks and waits for them. If a task that can't be aborted is running, closing or opening is refused with a message, and quitting asks you first.
