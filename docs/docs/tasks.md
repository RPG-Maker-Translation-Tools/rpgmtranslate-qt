# Tasks

Long-running work (reading a project, writing it back, search, replace, batch translate/script, a global lint check, purging, extracting an archive, exporting/importing) runs as a **task**: off the UI thread, tracked centrally, and shown in the task list.

## Where tasks show up

The button in the top bar (reads "No tasks" when idle) opens the task list. Each running task shows its name and a progress bar (`done/total` items). Tasks that support it also get an abort button next to their progress bar.

## Multiple tasks at once

Tasks aren't limited to one at a time - several can genuinely run in parallel, each on its own background thread. Whether two tasks are allowed to run together depends on what they touch, not just whether the app is "busy":

- **Write, Search, Export, and Global Check** only read project state, so any combination of these can run simultaneously - search while writing, global-check while writing, etc.
- Every other task (**Read, Purge, Extract archive, Replace, Batch Script, Batch Translate, Import**) mutates project state, so it blocks _everything_ else, including another instance of itself, until it finishes.

If you try to start a task that isn't allowed to run alongside what's currently running, you'll get a notice naming the task that's in the way instead of the new one silently queuing or failing.

## Aborting

Search, Replace, Batch Script, Batch Translate, Global Check, Export, and Import can be aborted mid-run from their abort button in the task list. Read, Write, Purge, and Extract archive can't be interrupted once started - let them finish.
