#include "TaskWorker.hpp"

#include "Enums.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QApplication>
#include <QFile>
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

auto toFFIString(const QByteArray& utf8) -> FFIString {
    return { .ptr = utf8.data(), .len = usize(utf8.size()) };
}

auto fileLines(
    const QLatin1StringView filename,
    const HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings
) -> result<FileLines, QString> {
    QString content;
    QSVList lines;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = filename.sliced(3).toUInt();
        lines =
            QStringView(mapSections[mapNumber]).split('\n', Qt::SkipEmptyParts);
    } else {
        const QString path =
            projectSettings->translationPath() + '/' + filename + u".txt";

        auto file = QFile(path);

        if (!file.open(QFile::ReadOnly)) {
            qWarning() << u"Failed to open file %1: %2"_qssv.arg(path).arg(
                file.errorString()
            );
            return Err(path);
        }

        content = file.readAll();
        lines = QStringView(content).split('\n', Qt::SkipEmptyParts);
    }

    return FileLines{ .content = content, .lines = lines };
};

template <class F>
auto modifyFile(
    const QLatin1StringView filename,
    HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings,
    F&& func
) -> bool {
    QString content;
    QSVList lines;

    QFile* file;

    if (filename.startsWith("map"_L1)) {
        const u16 mapNumber = filename.sliced(3).toUInt();
        lines =
            QStringView(mapSections[mapNumber]).split('\n', Qt::SkipEmptyParts);
    } else {
        const QString path =
            projectSettings->translationPath() + '/' + filename + u".txt";

        file = new QFile(path);

        if (!file->open(QFile::ReadWrite)) {
            qWarning() << u"Failed to open file %1: %2"_qssv.arg(path).arg(
                file->errorString()
            );
            delete file;
            return false;
        }

        content = file->readAll();
        lines = QStringView(content).split('\n', Qt::SkipEmptyParts);
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
            const u32 totalSize =
                remainder.join(' ').size() + 1 + lineView.size();
            currentLine.reserve(totalSize);
            currentLine = remainder.join(' ');
            currentLine += ' ';
            currentLine += lineView;
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

            wrappedLines.emplace_back(joinQSVList(words, ' '));
        } else {
            wrappedLines.emplace_back(std::move(currentLine));
        }
    }

    if (!remainder.isEmpty()) {
        wrappedLines.emplace_back(remainder.join(' '));
    }

    return wrappedLines.join('\n');
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
    const QString& projectPath,
    const QString& sourcePath,
    const QString& translationPath,
    const ReadMode readMode,
    const EngineType engineType,
    const DuplicateMode duplicateMode,
    const Selected selected,
    const BaseFlags flags,
    const bool mapEvents,
    const ByteBuffer hashes
) {
    ByteBuffer outHashes;

    const QByteArray projectPathUtf8 = projectPath.toUtf8();
    const QByteArray sourcePathUtf8 = sourcePath.toUtf8();
    const QByteArray translationPathUtf8 = translationPath.toUtf8();

    const FFIString error = rpgm_read(
        toFFIString(projectPathUtf8),
        toFFIString(sourcePathUtf8),
        toFFIString(translationPathUtf8),
        readMode,
        engineType,
        duplicateMode,
        selected,
        flags,
        mapEvents,
        hashes,
        &outHashes
    );

    emit readFinished({ error, outHashes });
}

void TaskWorker::extractArchive(
    const QString& archivePath,
    const QString& folder
) {
    const QByteArray archivePathUtf8 = archivePath.toUtf8();
    const QByteArray folderUtf8 = folder.toUtf8();

    const FFIString error = rpgm_extract_archive(
        toFFIString(archivePathUtf8),
        toFFIString(folderUtf8)
    );

    emit extractFinished(error);
}

void TaskWorker::write(const QString& gameTitle, const Selected selected) {
    f32 elapsed;

    const QByteArray sourcePathUtf8 = projectSettings->sourcePath().toUtf8();
    const QByteArray translationPathUtf8 =
        projectSettings->translationPath().toUtf8();
    const QByteArray outputPathUtf8 = projectSettings->outputPath().toUtf8();
    const QByteArray gameTitleUtf8 = gameTitle.toUtf8();

    const FFIString error = rpgm_write(
        toFFIString(sourcePathUtf8),
        toFFIString(translationPathUtf8),
        toFFIString(outputPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        toFFIString(gameTitleUtf8),
        projectSettings->flags,
        selected,
        &elapsed
    );

    emit writeFinished({ error, elapsed });
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
        pattern = u"\\b"_s + pattern + u"\\b";
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
                for (const i32 idx : range(0, match.lastCapturedIndex() + 1)) {
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
        const auto filename = QLatin1StringView(filenameArray.data());
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
                skippedString += QLatin1StringView(filename.data());
                skippedString += '\n';
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
    const std::variant<
        BatchMenu::TrimFlags,
        std::tuple<TranslationEndpoint, QString>,
        u8>& variant,
    const Glossary& glossary
) {
    u32 count = 0;
    u32 done = 0;

    auto filenames = selected.filenames(projectSettings->engineType);

    if (action == BatchAction::Translate) {
        const auto [endpoint, context] = std::get<1>(variant);

        const auto glossaryJSON =
            QJsonDocument(glossary.toJSON()).toJson(QJsonDocument::Compact);

        const auto endpointSettingsJSON =
            QJsonDocument(settings->translation.openaiCompatible.toJSON())
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

        const FFIString error = rpgm_translate(
            endpoint,
            toFFIString(endpointSettingsJSON),
            toFFIString(projectContext),
            toFFIString(localContext),
            toFFIString(translationPath),
            projectSettings->sourceLang,
            projectSettings->translationLang,
            { .ptr = ras<const u8*>(filenames.data()),
              .len = usize(filenames.size()) },
            toFFIString(glossaryJSON),
            &translatedFiles,
            &translatedFilesFFI
        );

        if (error.ptr != nullptr) {
            emit translateFinished(Err(error));
        } else {
            emit translateFinished(
                std::tuple(translatedFiles, translatedFilesFFI)
            );
        }
        return;
    }

    u16 skippedCount = 0;

    for (const auto [idx, filenameArray] : views::enumerate(filenames)) {
        const auto filename = QLatin1StringView(filenameArray.data());
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
                        joined += '\n';
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
                    joined += '\n';
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
                        joined += '\n';
                        continue;
                    }

                    const QString wrapped =
                        wrapText(parts[columnIndex], wrapLength);
                    joined += wrapped;
                    joined += '\n';
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
        emit lockFile(QLatin1StringView(filename.data()));
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

                for (const u32 idx :
                     range<u32>(rowStart, cellMatch.rowIndex())) {
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

                        for (int ci = 0; ci < replaceText.size(); ++ci) {
                            if (replaceText[ci] != u'\\' ||
                                ci + 1 >= replaceText.size()) {
                                result.append(replaceText[ci]);
                                continue;
                            }

                            const QChar next = replaceText[ci + 1];

                            if (next == '`') {
                                result.append(beforeFull);
                                ++ci;
                            } else if (next == '\'') {
                                result.append(afterFull);
                                ++ci;
                            } else if (next == '+') {
                                result.append(lastCapture);
                                ++ci;
                            } else if (next == '\\') {
                                result.append('\\');
                                ++ci;
                            } else if (next.isDigit() && next != '0') {
                                bool handled = false;

                                if (ci + 2 < replaceText.size() &&
                                    replaceText[ci + 2].isDigit()) {
                                    const int twoDigit =
                                        (next.digitValue() * 10) +
                                        replaceText[ci + 2].digitValue();
                                    const u32 idx =
                                        capturedBegin + u32(twoDigit - 1);
                                    if (idx < capturedEnd &&
                                        matchSpan[idx].capture()) {
                                        result.append(part.sliced(
                                            matchSpan[idx].start(),
                                            matchSpan[idx].len()
                                        ));
                                        ci += 2;
                                        handled = true;
                                    }
                                }

                                if (!handled) {
                                    const int oneDigit = next.digitValue();
                                    const u32 idx =
                                        capturedBegin + u32(oneDigit - 1);
                                    if (idx < capturedEnd &&
                                        matchSpan[idx].capture()) {
                                        result.append(part.sliced(
                                            matchSpan[idx].start(),
                                            matchSpan[idx].len()
                                        ));
                                        ci += 1;
                                        handled = true;
                                    }
                                }

                                if (!handled) {
                                    result.append('\\');
                                }
                            } else {
                                result.append('\\');
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

            for (const u32 idx : range<u32>(rowStart, lines.size())) {
                newLines.push_back(lines[idx]);
            }

            return joinQSVList(newLines, '\n');
        };

        modifyFile(
            QLatin1StringView(filename.data()),
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
    array<QString, TRANSLATION_ENDPOINT_COUNT> translations;

    TranslationEndpoint endpoint;
    QString localContext;

    if (projectSettings->fileContexts.contains(filename)) {
        const QString localContext = projectSettings->fileContexts[filename];
    }

    const auto glossaryJSON =
        QJsonDocument(glossary.toJSON()).toJson(QJsonDocument::Compact);

    const auto translateClosure =
        [this, &translations, &glossaryJSON, &localContext, &text](
            TranslationEndpoint endpoint,
            const QByteArray& endpointSettings
        ) -> void {
        FFIString outString;

        const QByteArray projectContext =
            projectSettings->projectContext.toUtf8();
        const QByteArray localContextUtf8 = localContext.toUtf8();
        const QByteArray textUtf8 = text.toUtf8();

        const FFIString error = rpgm_translate_single(
            endpoint,
            toFFIString(endpointSettings),
            toFFIString(projectContext),
            toFFIString(localContextUtf8),
            projectSettings->sourceLang,
            projectSettings->translationLang,
            toFFIString(textUtf8),
            { .ptr = ras<const u8*>(glossaryJSON.data()),
              .len = usize(glossaryJSON.size()) },
            &outString
        );

        if (error.ptr != nullptr) {
            translations[u8(endpoint)] =
                QString::fromUtf8(error.ptr, isize(error.len));
            rpgm_string_free(error);
            return;
        }

        translations[u8(endpoint)] =
            QString::fromUtf8(outString.ptr, isize(outString.len));
    };

    if (settings->translation.google.singleTranslation) {
        translateClosure(TranslationEndpoint::Google, QByteArray());
    }

    if (settings->translation.yandex.singleTranslation) {
        translateClosure(
            TranslationEndpoint::Yandex,
            QJsonDocument(settings->translation.yandex.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.deepl.singleTranslation) {
        translateClosure(
            TranslationEndpoint::DeepL,
            QJsonDocument(settings->translation.deepl.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.chatgpt.singleTranslation) {
        translateClosure(
            TranslationEndpoint::OpenAI,
            QJsonDocument(settings->translation.chatgpt.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.claude.singleTranslation) {
        translateClosure(
            TranslationEndpoint::Anthropic,
            QJsonDocument(settings->translation.claude.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.gemini.singleTranslation) {
        translateClosure(
            TranslationEndpoint::Gemini,
            QJsonDocument(settings->translation.gemini.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.deepseek.singleTranslation) {
        translateClosure(
            TranslationEndpoint::DeepSeek,
            QJsonDocument(settings->translation.deepseek.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.openaiCompatible.singleTranslation) {
        translateClosure(
            TranslationEndpoint::OpenAICompatible,
            QJsonDocument(settings->translation.openaiCompatible.toJSON())
                .toJson(QJsonDocument::Compact)
        );
    }

    if (settings->translation.ollama.singleTranslation) {
        translateClosure(
            TranslationEndpoint::Ollama,
            QJsonDocument(settings->translation.ollama.toJSON())
                .toJson(QJsonDocument::Compact)
        );
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
    std::tuple<QString, TextMatch*> replacedData;

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

        return joinQSVList(lines, '\n');
    };

    modifyFile(
        QLatin1StringView(filename.toLatin1()),
        *mapSections,
        projectSettings,
        closure
    );

    emit singleReplaceFinished(replacedData);
}

void TaskWorker::purge(const QString& gameTitle, const Selected selected) {
    const QByteArray sourcePathUtf8 = projectSettings->sourcePath().toUtf8();
    const QByteArray translationPathUtf8 =
        projectSettings->translationPath().toUtf8();
    const QByteArray gameTitleUtf8 = gameTitle.toUtf8();

    const FFIString error = rpgm_purge(
        toFFIString(sourcePathUtf8),
        toFFIString(translationPathUtf8),
        projectSettings->engineType,
        projectSettings->duplicateMode,
        toFFIString(gameTitleUtf8),
        projectSettings->flags,
        selected
    );

    emit purgeFinished(error);
}