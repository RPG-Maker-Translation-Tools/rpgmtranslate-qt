---
title: "Git"
---

A built-in git client tracks your translation as it progresses. It is aimed at local version control: remote support is limited to pushing, and some controls are still being wired up.

Open it with the ![](./assets/commit.svg) button.

## Setup

The repository lives in the project's `.rpgmtranslate` directory, not in the game directory itself. If it isn't a git repository yet, you'll see a "Create Repository" button that initializes one, and writes a `.gitignore` that leaves out backups and the baseline copy of the game data (an existing `.gitignore` is kept).

Commit author identity is configured in [Settings > Core > Git](./settings.md#git): either your own name/email, or "prefer the repository's git config" to defer to a `git config` identity already set for the repository.

## Working with changes

- The changes list shows every modified/added/untracked/deleted/renamed file, with a status badge per file.
- Click a file's checkbox to stage or unstage it.
- The refresh button re-scans the working directory for changes.
- "Copy translation to root" copies `.rpgmtranslate/translation` into a `translation` directory in the game directory, overwriting files there - handy if you keep the translation files in a repository of the game itself.

## Committing

Type a commit message and commit normally, or use the menu next to the commit button for:

- **Commit (Amend)** - amend the previous commit instead of creating a new one.
- **Commit and push** - commit and immediately push to `origin`.

## Branches

The branch selector lists local and remote branches (marked separately) and lets you switch between them. Creating/deleting branches isn't available from the UI yet.

## History

The commit history lists your local commits. It is display-only: clicking a commit shows no diff yet.

## Known limitations

This is still a work in progress:

- No diff view for a file's changes yet.
- No discard/revert-file action yet. The "Stage Selected" and "Revert Selected" items of the changes list's context menu don't do anything yet.
- No pull/fetch, stash, tag, or remote-management UI yet - `git pull`/fetch and remote branches still need the regular command line or another git client.
