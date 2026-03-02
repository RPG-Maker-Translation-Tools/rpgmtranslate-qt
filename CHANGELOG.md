# Changelog

## v1.0.0-rc.1

The first release candidate of the rewrite of the original project in C++.

### Changes

- Overall, improved user experience with the application.
- Added tracking of the currently executing tasks, such as batch actions, search and replace.
- No more temporary files. `maps.txt` is parsed to sections and stored in-memory for the duration of the project, while matches are tightly packed into a memory-efficient way, and stored in-memory as long as they're displayed in the search panel.
- Added stubs for LanguageTool and spell check, but those aren't currently implemented.
