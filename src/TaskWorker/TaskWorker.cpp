#include "TaskWorker.hpp"

#include "Aliases.hpp"
#include "Enums.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QMessageBox>

enum class RustLogLevel : u8 {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Trace = 4,
};

struct ToProcess {
    QStringView text;
    i8 columnIndex;
};

auto fileLines(
    const QL1SV filename,
    const HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings
) -> result<FileLines, QString> {
    QString content;
    QSVList lines;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = filename.sliced(3).toUInt();
        lines = QStringView(mapSections[mapNumber])
                    .split(u'\n', Qt::SkipEmptyParts);
    } else {
        const QString path =
            projectSettings->translationPath() % u'/' % filename % u".txt";

        auto file = QFile(path);

        if (!file.open(QFile::ReadOnly)) {
            qWarning() << "Failed to open file %1: %2"_L1.arg(
                path,
                file.errorString()
            );
            return Err(path);
        }

        content = QString::fromUtf8(file.readAll());
        lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);
    }

    return FileLines{ .content = content, .lines = lines };
};

template <class F>
auto modifyFile(
    const QL1SV filename,
    HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings,
    F&& func
) -> bool {
    QString content;
    QSVList lines;

    QFile* file;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = filename.sliced(3).toUInt();
        lines = QStringView(mapSections[mapNumber])
                    .split(u'\n', Qt::SkipEmptyParts);
    } else {
        const QString path =
            projectSettings->translationPath() % u'/' % filename % u".txt";

        file = new QFile(path);

        if (!file->open(QFile::ReadWrite)) {
            qWarning() << "Failed to open file %1: %2"_L1.arg(
                path,
                file->errorString()
            );
            delete file;
            return false;
        }

        content = QString::fromUtf8(file->readAll());
        lines = QStringView(content).split(u'\n', Qt::SkipEmptyParts);
    }

    QString result = func(content, lines);

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = filename.sliced(3).toUInt();
        mapSections.find(mapNumber)->second = std::move(result);
    } else {
        const QByteArray utf8 = result.toUtf8();

        file->seek(0);
        file->resize(utf8.size());
        file->write(utf8);

        delete file;
    }

    return true;
}

[[nodiscard]] auto wrapText(const QStringView text, const u8 wrapLimit)
    -> QString {
    QStringList remainder;
    QStringList wrappedLines;

    for (const QStringView lineView : qTokenize(text, u'\n')) {
        QString currentLine;

        if (!remainder.isEmpty()) {
            currentLine = remainder.join(u' ') % u' ' % lineView;
            remainder.clear();
        } else {
            currentLine = lineView.toString();
        }

        if (currentLine.size() > wrapLimit) {
            QSVList words;
            u32 wordsLength = 0;

            const auto tokenized =
                qTokenize(currentLine, u' ', Qt::SkipEmptyParts);

            for (const auto wordView : tokenized) {
                words.emplace_back(wordView);
                wordsLength += wordView.size() + (words.size() > 1 ? 1 : 0);
            }

            while (wordsLength > wrapLimit && !words.isEmpty()) {
                const QStringView popped = words.takeLast();
                wordsLength -= popped.size() + (!words.isEmpty() ? 1 : 0);
                remainder.prepend(popped.toString());
            }

            wrappedLines.emplace_back(joinQSVList(words, u' '));
        } else {
            wrappedLines.emplace_back(std::move(currentLine));
        }
    }

    if (!remainder.isEmpty()) {
        wrappedLines.emplace_back(remainder.join(u' '));
    }

    return wrappedLines.join(u'\n');
}

TaskWorker::TaskWorker(QObject* const parent) : QObject{ parent } {
    moveToThread(&thread);
}

TaskWorker::~TaskWorker() {
    stop();
}

void TaskWorker::start() {
    thread.start();
}

void TaskWorker::stop() {
    thread.quit();
    thread.wait();
}

void TaskWorker::read(
    const QString& sourcePath,
    const QString& translationPath,
    const ReadMode readMode,
    const EngineType engineType,
    const DuplicateMode duplicateMode,
    const Selected selected,
    const BaseFlags flags,
    const bool mapEvents,
    const HashMap<FilenameArray, u64>& hashes,
    const QString& title
) {
    ByteBuffer outHashes;

    const QByteArray titleUtf8 = title.toUtf8();
    const QByteArray sourcePathUtf8 = sourcePath.toUtf8();
    const QByteArray translationPathUtf8 = translationPath.toUtf8();

    QByteArray hashesArray;
    const u32 hashesArraySize =
        (hashes.size() * sizeof(FilenameArray)) + (hashes.size() * sizeof(u64));
    hashesArray.reserve(hashesArraySize);

    for (const auto [filename, hash] : hashes) {
        const cstr hashBytes = ras<cstr>(&hash);

        hashesArray.append(filename);
        hashesArray.append(hashBytes, sizeof(u64));
    }

    const ByteBuffer hashesBuf =
        ByteBuffer{ .ptr = ras<const u8*>(hashesArray.data()),
                    .len = u32(hashesArray.size()),
                    .cap = u32(hashes.size()) };

    const bool success = rpgm_read(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        readMode,
        engineType,
        duplicateMode,
        selected,
        flags,
        mapEvents,
        hashesBuf,
        strtoffi(titleUtf8),
        &outHashes
    );

    success ? emit readFinished(outHashes)
            : emit readFinished(Err(rpgm_error()));
}

void TaskWorker::extractArchive(
    const QString& archivePath,
    const QString& folder
) {
    const QByteArray archivePathUtf8 = archivePath.toUtf8();
    const QByteArray folderUtf8 = folder.toUtf8();

    const bool success =
        rpgm_extract_archive(strtoffi(archivePathUtf8), strtoffi(folderUtf8));

    success ? emit extractFinished({})
            : emit extractFinished(Err(rpgm_error()));
}

void TaskWorker::write(const QString& gameTitle, const Selected selected) {
    f32 elapsed;

    const QByteArray sourcePathUtf8 =
        projectSettings->actualSourcePath().toUtf8();
    const QByteArray translationPathUtf8 =
        projectSettings->translationPath().toUtf8();
    const QByteArray outputPathUtf8 = projectSettings->outputPath().toUtf8();
    const QByteArray gameTitleUtf8 = gameTitle.toUtf8();

    const bool success = rpgm_write(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        strtoffi(outputPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        strtoffi(gameTitleUtf8),
        projectSettings->flags,
        selected,
        &elapsed
    );

    success ? emit writeFinished(elapsed)
            : emit writeFinished(Err(rpgm_error()));
}

void TaskWorker::search(
    const SearchMenu::Action action,
    const Selected selected,
    const QString& searchText,
    SearchLocation searchLocation,
    const i8 searchColumnIndex,
    const SearchFlags searchFlags,
    const u16 tabCount
) {
    if (action == SearchMenu::Action::Replace) {
        searchLocation &= ~SearchLocation::Source;
    }

    if (action == SearchMenu::Action::Put) {
        searchLocation &= ~SearchLocation::Translation;
    }

    QRegularExpression::PatternOptions options =
        QRegularExpression::UseUnicodePropertiesOption;

    if ((searchFlags & SearchFlags::CaseSensitive) == 0) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    QString pattern;

    if ((searchFlags & SearchFlags::RegExp) != 0) {
        pattern = searchText;
    } else {
        pattern = QRegularExpression::escape(searchText);
    }

    if ((searchFlags & SearchFlags::WholeWord) != 0) {
        pattern = u"\\b"_s % pattern % u"\\b";
    }

    const auto regexp = QRegularExpression(pattern, options);

    HashMap<FilenameArray, vector<CellMatch>> searchMatches;
    searchMatches.reserve(tabCount);

    const auto closure = [&regexp,
                          &searchMatches,
                          &searchText,
                          searchFlags,
                          searchColumnIndex,
                          action](
                             const FilenameArray filename,
                             const QStringView line,
                             const u32 rowIndex,
                             const u32 columnIndex
                         ) -> void {
        const auto matches = regexp.globalMatchView(line);

        if (!matches.hasNext()) {
            return;
        }

        u32 capturedCount = 0;
        vector<QRegularExpressionMatch> matchesVec;

        for (const auto& match : matches) {
            capturedCount += match.lastCapturedIndex() + 1;
            matchesVec.emplace_back(
                std::move(const_cast<QRegularExpressionMatch&>(match))
            );
        }

        const MatchIndex matchIndex(
            rowIndex,
            action == SearchMenu::Action::Put ? searchColumnIndex : columnIndex
        );

        auto cellMatches = CellMatch{ .matches = new TextMatch[capturedCount],
                                      .matchesCount = capturedCount,
                                      .matchIndex = matchIndex };
        u32 matchesPos = 0;

        for (const auto& match : matchesVec) {
            if ((searchFlags & SearchFlags::Put) != 0) {
                const u32 start = match.capturedStart();
                const u32 length = match.capturedLength();

                if (start == 0 && length == line.size()) {
                    cellMatches.matches[matchesPos++] = TextMatch(0, 0, false);
                } else {
                    break;
                }
            } else {
                for (const auto idx : range(0, match.lastCapturedIndex() + 1)) {
                    cellMatches.matches[matchesPos++] = TextMatch(
                        match.capturedStart(idx),
                        match.capturedLength(idx),
                        idx > 0
                    );
                }
            }
        }

        searchMatches.emplace(filename, vector<CellMatch>());
        searchMatches.find(filename)->second.push_back(cellMatches);
    };

    u32 count = 0;
    u32 progress = 0;

    auto filenames = selected.filenames(projectSettings->engineType);
    u8 skippedCount = 0;

    for (const auto [idx, filenameArray] : views::enumerate(filenames)) {
        const auto filename = QL1SV(filenameArray.data());
        const auto result = fileLines(filename, *mapSections, projectSettings);

        if (!result) {
            std::swap(filenames[idx], filenames[skippedCount++]);
            continue;
        }

        const auto lines = result.value().lines;
        count += lines.size();

        for (const auto [rowIndex, line] : views::enumerate(lines)) {
            progress++;
            emit progressChanged(Task::Search, progress, count);

            if (line.startsWith(COMMENT_PREFIX) &&
                !line.startsWith(MAP_DISPLAY_NAME_COMMENT_PREFIX) &&
                !line.startsWith(BOOKMARK_COMMENT)) {
                if ((searchFlags & SearchFlags::Comment) != 0) {
                    closure(filenameArray, line, rowIndex, 0);
                }

                continue;
            }

            const auto parts = lineParts(line, 0, filename);

            if (parts.isEmpty()) {
                continue;
            }

            const auto source = getSource(parts);

            vector<ToProcess> toProcess;

            if ((searchLocation & SearchLocation::Source) != 0) {
                toProcess.emplace_back(source, 0);
            }

            if ((searchLocation & SearchLocation::Translation) != 0) {
                if (searchColumnIndex == -1) {
                    for (const auto [idx, translation] :
                         views::enumerate(views::drop(parts, 1))) {
                        if (translation.empty()) {
                            continue;
                        }

                        toProcess.emplace_back(translation, idx + 1);
                    }
                } else if (searchColumnIndex == 0) {
                    const auto translation = getTranslation(parts);

                    toProcess.emplace_back(
                        translation.translation,
                        translation.index
                    );
                } else {
                    toProcess.emplace_back(
                        parts[searchColumnIndex],
                        searchColumnIndex
                    );
                }
            }

            for (const auto entryToProcess : toProcess) {
                const auto normalized =
                    qsvReplace(entryToProcess.text, NEW_LINE, LINE_FEED);

                closure(
                    filenameArray,
                    normalized,
                    rowIndex,
                    entryToProcess.columnIndex
                );
            }
        }
    }

    if (skippedCount > 0) {
        QMetaObject::invokeMethod(
            qApp,
            [filenames = std::move(filenames), skippedCount] -> void {
            QString skippedString;

            for (const auto filename : views::take(filenames, skippedCount)) {
                skippedString += QL1SV(filename.data());
                skippedString += u'\n';
            }

            QMessageBox::warning(
                nullptr,
                tr("Files were skipped"),
                tr("The program was unable to open the following files:\n %1")
                    .arg(skippedString)
            );
        },
            Qt::QueuedConnection
        );
    }

    emit searchFinished(std::move(searchMatches));
}

void TaskWorker::performBatchAction(
    const Selected selected,
    const BatchAction action,
    const u8 columnIndex,
    const std::variant<BatchMenu::TrimFlags, tuple<u8, QString>, u8>& variant,
    const Glossary& glossary
) {
    u32 count = 0;
    u32 done = 0;

    auto filenames = selected.filenames(projectSettings->engineType);

    if (action == BatchAction::Translate) {
        const auto [endpointIndex, context] = std::get<1>(variant);

        const auto glossaryJSON =
            QJsonDocument(glossary.toJSON()).toJson(QJsonDocument::Compact);

        const auto endpointSettingsJSON =
            QJsonDocument(
                settings->translation.endpoints[endpointIndex].toJSON()
            )
                .toJson(QJsonDocument::Compact);

        const QByteArray projectContext =
            projectSettings->projectContext.toUtf8();
        const QByteArray localContext =
            projectSettings->projectContext.toUtf8();
        const QByteArray translationPath =
            projectSettings->translationPath().toUtf8();

        ByteBuffer translatedFiles;
        ByteBuffer translatedFilesFFI;

        emit progressChanged(Task::BatchTranslate, 0, 0);

        const bool success = rpgm_translate(
            strtoffi(endpointSettingsJSON),
            strtoffi(projectContext),
            strtoffi(localContext),
            strtoffi(translationPath),
            projectSettings->sourceLang,
            projectSettings->translationLang,
            { .ptr = ras<const u8*>(filenames.data()),
              .len = u32(filenames.size()) },
            strtoffi(glossaryJSON),
            &translatedFiles,
            &translatedFilesFFI
        );

        if (!success) {
            emit translateFinished(Err(rpgm_error()));
        } else {
            emit translateFinished(tuple(translatedFiles, translatedFilesFFI));
        }
        return;
    }

    u16 skippedCount = 0;

    for (const auto [idx, filenameArray] : views::enumerate(filenames)) {
        const auto filename = QL1SV(filenameArray.data());
        emit lockFile(filename);

        if (action == BatchAction::Trim) {
            const BatchMenu::TrimFlags trimFlags = std::get<0>(variant);

            auto closure =
                [this, columnIndex, filename, &done, &count, trimFlags](
                    const QString& content,
                    const QSVList& lines
                ) -> QString {
                QString joined;
                joined.reserve(content.size() * 2);

                count += lines.size();

                for (const auto [idx, line] : views::enumerate(lines)) {
                    emit progressChanged(Task::BatchTrim, ++done, count);
                    QSVList parts = lineParts(line, idx, filename);

                    if (parts.size() < columnIndex) {
                        joined += joinQSVList(parts, SEPARATORL1);
                        joined += u'\n';
                        continue;
                    }

                    if ((trimFlags & BatchMenu::TrimFlags::Leading) != 0) {
                        for (const auto [idx, chr] :
                             views::enumerate(parts[columnIndex])) {
                            if (!chr.isSpace()) {
                                parts[columnIndex] =
                                    parts[columnIndex].sliced(idx);
                                break;
                            }
                        }
                    }

                    if ((trimFlags & BatchMenu::TrimFlags::Trailing) != 0) {
                        for (const auto [idx, chr] : views::enumerate(
                                 views::reverse(parts[columnIndex])
                             )) {
                            if (!chr.isSpace()) {
                                parts[columnIndex] = parts[columnIndex].sliced(
                                    0,
                                    parts[columnIndex].size() - idx
                                );
                                break;
                            }
                        }
                    }

                    joined += joinQSVList(parts, SEPARATORL1);
                    joined += u'\n';
                }

                joined.removeLast();
                return joined;
            };

            const bool success =
                modifyFile(filename, *mapSections, projectSettings, closure);

            if (!success) {
                std::swap(filenames[idx], filenames[skippedCount++]);
            }
        } else if (action == BatchAction::Wrap) {
            const u8 wrapLength = std::get<2>(variant);

            auto closure = [this, columnIndex, wrapLength, filename](
                               const QString& content,
                               const QSVList& lines
                           ) -> QString {
                QString joined;
                joined.reserve(content.size() * 2);

                for (const auto [idx, line] : views::enumerate(lines)) {
                    const QSVList parts = lineParts(line, idx, filename);

                    if (parts.size() < columnIndex) {
                        joined += joinQSVList(parts, SEPARATORL1);
                        joined += u'\n';
                        continue;
                    }

                    const QString wrapped =
                        wrapText(parts[columnIndex], wrapLength);
                    joined += wrapped;
                    joined += u'\n';
                }

                joined.removeLast();
                return joined;
            };

            const bool success =
                modifyFile(filename, *mapSections, projectSettings, closure);

            if (!success) {
                std::swap(filenames[idx], filenames[skippedCount++]);
            }
        }
    }

    emit lockFile(QString());
};

void TaskWorker::replace(
    const HashMap<FilenameArray, vector<CellMatch>>& searchMatches,
    const Selected selected,
    const SearchMenu::Action action,
    const QString& searchText,
    const QString& replaceText,
    const SearchLocation searchLocation,
    const i8 columnIndex,
    const SearchFlags searchFlags
) {
    u32 done = 0;
    u32 total = 0;

    for (const auto& [filename, matches] : searchMatches) {
        emit lockFile(QL1SV(filename.data()));
        total += matches.size();

        auto closure =
            [this, action, &matches, &replaceText, filename, &done, &total](
                const QString& content,
                const QSVList& lines
            ) -> QString {
            QStringList ownedLines;
            ownedLines.reserve(isize(matches.size()));

            QSVList newLines;
            newLines.reserve(lines.size());

            u32 rowStart = 0;

            for (const auto& cellMatch : matches) {
                emit progressChanged(
                    action == SearchMenu::Action::Replace ? Task::Replace
                                                          : Task::Put,
                    ++done,
                    total
                );

                for (const auto idx : range(rowStart, cellMatch.rowIndex())) {
                    newLines.push_back(lines[idx]);
                }

                const QStringView line = lines[cellMatch.rowIndex()];
                QSVList parts =
                    lineParts(line, cellMatch.rowIndex() + 1, filename);
                const QStringView part = parts[cellMatch.colIndex()];

                QString replaced;

                if (action == SearchMenu::Action::Replace) {
                    u32 start = 0;

                    const auto matchSpan =
                        span(cellMatch.matches, cellMatch.matchesCount);

                    const auto applySubstitutions =
                        [&](const TextMatch& fullMatch,
                            const u32 capturedBegin,
                            const u32 capturedEnd) -> QString {
                        const QStringView beforeFull =
                            part.sliced(0, fullMatch.start());
                        const QStringView afterFull =
                            part.sliced(fullMatch.start() + fullMatch.len());

                        QStringView lastCapture;
                        for (u32 k = capturedEnd; k > capturedBegin;) {
                            --k;
                            if (matchSpan[k].capture()) {
                                lastCapture = part.sliced(
                                    matchSpan[k].start(),
                                    matchSpan[k].len()
                                );
                                break;
                            }
                        }

                        QString result;
                        result.reserve(replaceText.size());

                        for (auto cidx : range(0, replaceText.size())) {
                            if (replaceText[cidx] != u'\\' ||
                                cidx + 1 >= replaceText.size()) {
                                result.append(replaceText[cidx]);
                                continue;
                            }

                            const QChar next = replaceText[cidx + 1];

                            if (next == u'`') {
                                result.append(beforeFull);
                                ++cidx;
                            } else if (next == u'\'') {
                                result.append(afterFull);
                                ++cidx;
                            } else if (next == u'+') {
                                result.append(lastCapture);
                                ++cidx;
                            } else if (next == u'\\') {
                                result.append(u'\\');
                                ++cidx;
                            } else if (next.isDigit() && next != u'0') {
                                bool handled = false;

                                if (cidx + 2 < replaceText.size() &&
                                    replaceText[cidx + 2].isDigit()) {
                                    const i32 twoDigit =
                                        (next.digitValue() * 10) +
                                        replaceText[cidx + 2].digitValue();
                                    const u32 idx =
                                        capturedBegin + u32(twoDigit - 1);
                                    if (idx < capturedEnd &&
                                        matchSpan[idx].capture()) {
                                        result.append(part.sliced(
                                            matchSpan[idx].start(),
                                            matchSpan[idx].len()
                                        ));
                                        cidx += 2;
                                        handled = true;
                                    }
                                }

                                if (!handled) {
                                    const i32 oneDigit = next.digitValue();
                                    const u32 idx =
                                        capturedBegin + u32(oneDigit - 1);
                                    if (idx < capturedEnd &&
                                        matchSpan[idx].capture()) {
                                        result.append(part.sliced(
                                            matchSpan[idx].start(),
                                            matchSpan[idx].len()
                                        ));
                                        cidx += 1;
                                        handled = true;
                                    }
                                }

                                if (!handled) {
                                    result.append(u'\\');
                                }
                            } else {
                                result.append(u'\\');
                            }
                        }

                        return result;
                    };

                    for (u32 i = 0; i < u32(matchSpan.size());) {
                        const auto& match = matchSpan[i];

                        if (match.capture()) {
                            ++i;
                            continue;
                        }

                        u32 j = i + 1;
                        while (j < u32(matchSpan.size()) &&
                               matchSpan[j].capture()) {
                            ++j;
                        }

                        replaced.append(
                            part.sliced(start, match.start() - start)
                        );
                        replaced.append(applySubstitutions(match, i + 1, j));
                        start = match.start() + match.len();
                        i = j;
                    }

                    replaced.append(part.sliced(start, part.size() - start));
                    parts[cellMatch.colIndex()] = replaced;
                } else {
                    parts[cellMatch.colIndex()] = replaceText;
                }

                ownedLines.append(joinQSVList(parts, SEPARATORL1));
                newLines.push_back(ownedLines.last());
                rowStart = cellMatch.rowIndex() + 1;
            }

            for (const auto idx : range(rowStart, lines.size())) {
                newLines.push_back(lines[idx]);
            }

            return joinQSVList(newLines, u'\n');
        };

        modifyFile(
            QL1SV(filename.data()),
            *mapSections,
            projectSettings,
            closure
        );
    }

    for (const auto& [key, matches] : searchMatches) {
        for (const auto match : matches) {
            delete[] match.matches;
        }
    }

    emit lockFile(QString());
}

void TaskWorker::translateSingle(
    const QString& filename,
    const QString& text,
    const Glossary& glossary
) {
    vector<QString> translations;
    translations.reserve(settings->translation.endpoints.size());

    TranslationEndpoint endpoint;
    QString localContext;

    if (projectSettings->fileContexts.contains(filename)) {
        const QString localContext = projectSettings->fileContexts[filename];
    }

    const auto glossaryJSON =
        QJsonDocument(glossary.toJSON()).toJson(QJsonDocument::Compact);

    const auto translateClosure =
        [this, &translations, &glossaryJSON, &localContext, &text](
            const EndpointSettings& endpointSettings
        ) -> void {
        FFIString outString;

        const QByteArray projectContext =
            projectSettings->projectContext.toUtf8();
        const QByteArray localContextUtf8 = localContext.toUtf8();
        const QByteArray textUtf8 = text.toUtf8();

        const QByteArray endpointUtf8 = QJsonDocument(endpointSettings.toJSON())
                                            .toJson(QJsonDocument::Compact);

        const bool success = rpgm_translate_single(
            strtoffi(endpointUtf8),
            strtoffi(projectContext),
            strtoffi(localContextUtf8),
            projectSettings->sourceLang,
            projectSettings->translationLang,
            strtoffi(textUtf8),
            strtoffi(glossaryJSON),
            &outString
        );

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            translations.push_back(error.toString());
            return;
        }

        translations.push_back(
            QString::fromUtf8(outString.ptr, isize(outString.len))
        );
    };

    for (const auto& endpoint : settings->translation.endpoints) {
        if (!endpoint.singleTranslation) {
            continue;
        }

        translateClosure(endpoint);
    }

    emit singleTranslateFinished(translations);
}

void TaskWorker::replaceSingle(
    const QString& replaceText,
    const SearchPanelDock::Action action,
    const QString& filename,
    const i32 rowIndex,
    const u8 columnIndex,
    const span<const TextMatch> matches
) {
    tuple<QString, TextMatch*> replacedData;

    auto closure =
        [&replaceText,
         &filename,
         action,
         rowIndex,
         columnIndex,
         matches,
         &replacedData](const QString& content, QSVList& lines) -> QString {
        const QStringView line = lines[rowIndex];
        QSVList parts = lineParts(line, rowIndex + 1, filename);

        QString replaced;

        if (action == SearchPanelDock::Action::Put) {
            auto* const newMatches = new TextMatch[1];
            newMatches[0] = TextMatch(0, u32(replaceText.size()), false);
            parts[columnIndex] = QStringView(replaceText);
            replacedData = { replaceText, newMatches };
        } else {
            const QStringView text = parts[columnIndex];
            replaced.reserve(
                isize(text.size() + (replaceText.size() * matches.size()))
            );
            u32 pos = 0;

            auto* const newMatches = new TextMatch[matches.size()];
            u32 matchesIndex = 0;
            u32 delta = 0;

            for (const auto match : matches) {
                replaced.append(text.sliced(pos, match.start() - pos));

                const u32 newStart = match.start() + delta;
                const u32 newLen = u32(replaceText.size());

                newMatches[matchesIndex++] = TextMatch(newStart, newLen, false);
                replaced.append(replaceText);

                pos = match.start() + match.len();
                delta += newLen - match.len();
            }

            replaced.append(text.sliced(pos));
            parts[columnIndex] = QStringView(replaced);
            replacedData = { std::move(replaced), newMatches };
        }

        const QString merged = joinQSVList(parts, SEPARATORL1);
        lines[rowIndex] = QStringView(merged);

        return joinQSVList(lines, u'\n');
    };

    modifyFile(
        QL1SV(filename.toLatin1()),
        *mapSections,
        projectSettings,
        closure
    );

    emit singleReplaceFinished(replacedData);
}

void TaskWorker::purge(const QString& gameTitle, const Selected selected) {
    const QByteArray sourcePathUtf8 =
        projectSettings->actualSourcePath().toUtf8();
    const QByteArray translationPathUtf8 =
        projectSettings->translationPath().toUtf8();
    const QByteArray gameTitleUtf8 = gameTitle.toUtf8();

    const bool success = rpgm_purge(
        strtoffi(sourcePathUtf8),
        strtoffi(translationPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        strtoffi(gameTitleUtf8),
        projectSettings->flags,
        selected
    );

    success ? emit purgeFinished({}) : emit purgeFinished(Err(rpgm_error()));
}