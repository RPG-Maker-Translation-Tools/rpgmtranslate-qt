---
title: "Search Features"
---

Open the search menu with the ![](./assets/search.svg) button (`Ctrl+F`). Type into the search input and click the search button to run it. The inputs are multi-line, so Enter inserts a new line instead of running the search.

One input and set of options drives three actions - **Search**, **Replace**, and **Put**, covered below in order of danger. Search only looks; Replace and Put write to your translation, so read their sections before using them on anything you'd mind losing.

Pick files in the file select menu first (none are selected by default). It has "Select All" and "Deselect All" buttons; dragging across the checkboxes toggles every one you pass.

## Matching options

These apply to all three actions:

| Option | Effect |
| ---------------------------- | ------------------------------------------------------------------------------------ |
| Match Case | Case-sensitive matching. |
| Search Whole | Only matches whole words. |
| Search By Regular Expression | Treats the input as a regex - see [Regular expressions](#regular-expressions) below. |
| Search In Comments | Also searches comment lines, which are skipped by default. |
| File select | The files to process. |
| Location | Search Everywhere, Only Source, or Only Translation. |
| Column | All Columns, Rightmost Column, or any other translation column. |

## Search

Search only finds and lists matches; it never changes anything. Results appear in the ![](./assets/dock.svg) results panel (`Ctrl+R`), docked on the right by default (drag it out to float it). The panel filters results by file; its clear button empties it.

Matches are stored compactly, so even millions of matches stay responsive.

From the results panel, per-match:

| Click | Action |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Left | Jump to that match in the translation table. |
| Right | Replace just that match with the replace input's text. Matches in the source column can't be replaced. |
| Middle | Put the replace input's text in place of the cell (see [Put](#put)). For a match in a translation column you're asked to confirm first; for a match in the source column you pick the translation column to put it into. |

## Replace

The Replace button asks for confirmation, saying how many cells and files it covers, then replaces every match with the replace input's text across all files the search covers, not just the visible results. Replace only changes translation text, never the source, and it needs a specific translation column selected in **Column** - "All Columns" and "Rightmost Column" are refused. If the replace input is empty, you're asked whether you really want to replace the matches with nothing.

**Before a global replace**, run the search, skim the results panel, and confirm they're what you want. A wrong match slipping through a whole-project replace is hard to notice later. To do it one at a time, right-click individual results instead.

## Put

Put is the one to be careful with: it **overwrites the translation cell** for every row whose source text matches the search input, replacing it with the replace input's text - even if that cell already had a translation.

Put is a toggle next to the Replace button: while it is on, the Replace button performs Put instead of Replace. Like Replace, it needs a specific translation column selected in **Column**, and that column is the one it writes to.

Put only ever matches the _source_ text, so leave Location on "Search Everywhere" or "Only Source" - with "Only Translation" it finds nothing. It also matches the **entire** source string, start to end, not just a substring - so searching for `Hello` only hits a source string that's exactly `Hello`, not `Hello, world!`. To match a variable ending, use a regex with a greedy quantifier, e.g. `Hello .+` matches `Hello World`, `Hello Rust`, `Hello Whatever`, and so on.

As with Replace: search first, check the results panel, and only then run Put for everything. Middle-click a single result to put just that one row instead.

## Regular expressions

The application uses [Qt's regular expression implementation](https://doc.qt.io/qt-6/qregularexpression.html), which is essentially [PCRE2](https://www.pcre.org/current/doc/html/pcre2syntax.html) - Unicode is fully supported.

In the replace/put text, these substitutions are available:

| Sequence | Inserts |
| ---------------- | ------------------------------------------- |
| `` \` `` | The text before the full match. |
| `\'` | The text after the full match. |
| `\+` | The last captured group. |
| `\{n}` / `\{nn}` | Captured group `n`. `\0` is the full match. |
| `\\` | A literal `\`. |

To keep an easy way to fully revert a regex replace, try it first in an external tool (a text editor, a script). The application keeps no replace/put history.
