#include "BatchMenu.hpp"
#include "Constants.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "MainWindow.hpp"
#include "Notice.hpp"
#include "TranslationTable.hpp"
#include "TranslationTableModel.hpp"
#include "Types.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStyleHints>
#include <QUuid>
#include <archive.h>
#include <archive_entry.h>

namespace {
constexpr i32 ARCHIVE_ENTRY_PERM = 0644;
constexpr QStringView SETTINGS_PATH = u"/settings.json";
constexpr QStringView RVPACKER_METADATA_FILE = u"/.rvpacker-metadata";

// LanguageTool has one shared API key (not per-endpoint), so it gets a fixed keyring account.
constexpr QByteArrayView LANGUAGETOOL_CREDENTIAL_ACCOUNT = "languagetool";
}  // namespace

auto MainWindow::saveCurrentTab(QString tabName) -> bool {
    QString* mapSection = nullptr;

    if (tabName.isEmpty()) {
        tabName = ui->tabPanel->currentTabName();

        if (tabName.isEmpty()) {
            return true;
        }
    }

    if (tabName.startsWith(u"map")) {
        mapSection = &mapSections.find(stoa<u16>(QStringView(tabName).sliced(3)))->second;
        mapSection->clear();
    }

    unique_ptr<QFile> file;
    unique_ptr<QTextStream> stream;

    if (mapSection == nullptr) {
        QString filePath;

        if (!tabName.startsWith(u"map")) {
            filePath = projectSettings->translationPath();
        }

        filePath = filePath % u'/' % tabName % u".txt";

        const QString suffix = u'/' % tabName % u".txt";
        auto opened = openForWriting(this, filePath, suffix);

        if (!opened) {
            return false;
        }

        file = std::move(opened.value());

        if (file == nullptr) {
            return true;
        }

        stream = make_unique<QTextStream>(file.get());
        stream->setEncoding(QStringConverter::Utf8);
    } else {
        stream = make_unique<QTextStream>(mapSection, QFile::ReadOnly);
    }

    TranslationTableModel* const model = ui->translationTable->model();

    for (const auto row : range(0, model->rowCount())) {
        if ((model->flags(model->index(row, 1)) & Qt::ItemIsEditable) == 0) {
            *stream << *model->item(row, 0).text();
        } else {
            auto fields = QStringList(model->columnCount());

            for (const auto column : range(0, model->columnCount())) {
                const auto item = model->item(row, column);

                if (item.text()->isNull()) {
                    qWarning().noquote() << u"Item at row %1 and column %2 is nullptr."_qsv.arg(
                        itos(row).qsv(),
                        itos(column).qsv()
                    );
                    continue;
                }

                const QString* const text = item.text();
                fields[column] = qsvReplace(QStringView(text->data(), text->size()), LINE_FEED, NEW_LINE);
            }

            *stream << fields.join(SEPARATOR);
        }

        *stream << '\n';
    }

    if (tabName == u"system"_qsv) {
        const QString placeholder = ui->gameTitleInput->placeholderText();
        const QString text = ui->gameTitleInput->text();

        *stream << placeholder;
        *stream << SEPARATOR;

        if (text != placeholder) {
            *stream << ui->gameTitleInput->text();
        }
    }

    present(this, NOTICE("Tab %1 saved.", Info, Status, tabName.isEmpty() ? ui->tabPanel->currentTabName() : tabName));

    return true;
}

auto MainWindow::saveMaps() -> bool {
    QString mapsPath = projectSettings->translationPath() + u"/maps.txt";

    auto opened = openForWriting(this, mapsPath, u"/maps.txt");

    if (!opened) {
        return false;
    }

    auto mapsFile = std::move(opened.value());

    if (mapsFile == nullptr) {
        return true;
    }

    auto stream = QTextStream(mapsFile.get());

    auto keys = ranges::to<vector>(views::keys(mapSections));
    ranges::sort(keys);

    for (const auto key : keys) {
        const auto& content = mapSections[key];
        stream << content;
    }

    present(this, NOTICE("maps.txt saved.", Info, Status));

    return true;
}

void MainWindow::saveBackup() {
    const auto saveSuccess = saveCurrentTab();
    if (!saveSuccess) {
        return;
    }

    const QString backupPath = projectSettings->backupPath();
    const QList<QFileInfo> entries = QDir(backupPath).entryInfoList(QDir::Dirs, QDir::Time);

    if (entries.size() > settings->core.backup.max) {
        QDir(entries.first().filePath()).removeRecursively();
    }

    const auto date = QDate::currentDate();
    const auto time = QTime::currentTime();

    const QString archivePath = u"%1/%2-%3-%4_%5-%6-%7.tar.xz"_qsv.arg(
        backupPath,
        itos(date.day(), 2, '0').qsv(),
        itos(date.month(), 2, '0').qsv(),
        itos(date.year()).qsv(),
        itos(time.hour(), 2, '0').qsv(),
        itos(time.minute(), 2, '0').qsv(),
        itos(time.second(), 2, '0').qsv()
    );

    struct archive* arch = archive_write_new();
    archive_write_add_filter_xz(arch);
    archive_write_set_format_pax_restricted(arch);

    archive_write_set_filter_option(arch, "xz", "compression-level", "9");

    if (archive_write_open_filename(arch, archivePath.toStdString().c_str()) != ARCHIVE_OK) {
        qWarning().noquote() << u"Failed to open archive %1: %2"_qsv.arg(
            archivePath,
            svtostr(QUtf8SV(archive_error_string(arch)))
        );
        archive_write_free(arch);
        return;
    }

    auto listing =
        QDirListing(projectSettings->translationPath(), { u"*.txt"_s }, QDirListing::IteratorFlag::FilesOnly);

    for (const auto& entry : listing) {
        const QString filePath = entry.filePath();
        archive_entry* const aentry = archive_entry_new();

        const QString filename = entry.fileName();
        archive_entry_set_pathname(aentry, filename.toStdString().c_str());

        archive_entry_set_filetype(aentry, AE_IFREG);
        archive_entry_set_perm(aentry, ARCHIVE_ENTRY_PERM);
        archive_entry_set_size(aentry, entry.size());

        if (archive_write_header(arch, aentry) != ARCHIVE_OK) {
            qWarning().noquote() << u"Failed to write archive header"_qsv;
            archive_entry_free(aentry);
            continue;
        }

        auto file = QFile(filePath);

        if (file.open(QFile::ReadOnly)) {
            const QByteArray buffer = file.readAll();
            archive_write_data(arch, buffer.constData(), buffer.size());
        } else {
            qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(filePath, file.errorString());
        }

        archive_entry_free(aentry);
    }

    archive_write_close(arch);
    archive_write_free(arch);

    saveGlossary();
    saveProjectSettings();

    present(this, NOTICE("Backup %1 created.", Info, Status, lastPathComponent(archivePath)));
}

auto MainWindow::saveSettings() -> bool {
    QString path = qApp->property("data-location").toString() + SETTINGS_PATH;

    auto opened = openForWriting(this, path, SETTINGS_PATH);

    if (!opened) {
        return false;
    }

    auto settingsFile = std::move(opened.value());

    if (settingsFile == nullptr) {
        return true;
    }

    // API keys never touch settings.json - they're moved into the OS credential store here
    // and stripped from the copy that actually gets written.
    auto redacted = *settings;

    for (auto& endpoint : redacted.translation.endpoints) {
        if (!endpoint.apiKey.isEmpty()) {
            rpgm_store_credential(strtoffi(endpoint.id.toUtf8()), strtoffi(endpoint.apiKey.toUtf8()));
        }

        endpoint.apiKey.clear();
    }

    if (!redacted.translation.languageTool.apiKey.isEmpty()) {
        rpgm_store_credential(
            strtoffi(LANGUAGETOOL_CREDENTIAL_ACCOUNT),
            strtoffi(redacted.translation.languageTool.apiKey.toUtf8())
        );
        redacted.translation.languageTool.apiKey.clear();
    }

    const auto result = glz::write_json(redacted);
    settingsFile->write(result.value().data());
    return true;
}

auto MainWindow::saveGlossary() -> bool {
    QString path = projectSettings->glossaryPath();

    auto opened = openForWriting(this, path, GLOSSARY_FILE);

    if (!opened) {
        return false;
    }

    auto glossaryFile = std::move(opened.value());

    if (glossaryFile == nullptr) {
        return true;
    }

    const auto result = glz::write_json(glossaryMenu->glossary());

    glossaryFile->write(result.value().data());
    return true;
}

auto MainWindow::saveProjectSettings() -> bool {
    QString path = projectSettings->projectSettingsPath();

    auto opened = openForWriting(this, path, PROJECT_SETTINGS_FILE);

    if (!opened) {
        return false;
    }

    if (const auto& projectSettingsFile = opened.value(); projectSettingsFile != nullptr) {
        const auto result = glz::write_json(projectSettings);
        projectSettingsFile->write(result.value().data());
    }

    QString metadataPath = projectSettings->translationPath() + RVPACKER_METADATA_FILE;

    auto openedMetadata = openForWriting(this, metadataPath, RVPACKER_METADATA_FILE);

    if (!openedMetadata) {
        return false;
    }

    auto metadataFile = std::move(openedMetadata.value());

    if (metadataFile == nullptr) {
        return true;
    }

    QVariantHash metadata;

    metadata[u"disableCustomProcessing"_s] = (projectSettings->flags & BaseFlags_DisableCustomProcessing) != 0;
    metadata[u"trim"_s] = (projectSettings->flags & BaseFlags_Trim) != 0;
    metadata[u"duplicateMode"_s] = scast<u8>(projectSettings->duplicateMode);

    QJsonObject hashes;

    for (const auto [key, hash] : projectSettings->hashes) {
        hashes[QL1SV(key.data())] = scast<qint64>(hash);
    }

    metadata[u"hashes"_s] = hashes;

    metadataFile->write(QJsonDocument(QJsonObject::fromVariantHash(metadata)).toJson(QJsonDocument::Compact));

    return true;
}

auto MainWindow::saveEverything() -> bool {
    if (!saveSettings()) {
        return false;
    }

    if (projectSettings != nullptr) {
        if (!saveCurrentTab()) {
            return false;
        }

        if (!saveMaps()) {
            return false;
        }

        if (!saveProjectSettings()) {
            return false;
        }

        if (!saveGlossary()) {
            return false;
        }

        if (!batchMenu->save()) {
            return false;
        }
    }

    return true;
}

void MainWindow::loadBackup(const QString& backupPath) {
    auto file = QFile(backupPath);

    if (!file.open(QFile::ReadOnly)) {
        present(this, NOTICE("Opening backup %1 failed: %2", Warning, Modal, backupPath, file.errorString()));
        return;
    }

    QByteArray content = file.readAll();

    archive* const arch = archive_read_new();
    archive_read_support_format_tar(arch);
    archive_read_support_filter_xz(arch);

    if (archive_read_open_memory(arch, content.data(), content.size()) != ARCHIVE_OK) {
        present(this, NOTICE("Reading archive failed with %1", Warning, Modal, QUtf8SV(archive_error_string(arch))));
        archive_read_free(arch);
        return;
    }

    archive_entry* entry = nullptr;

    while (archive_read_next_header(arch, &entry) == ARCHIVE_OK) {
        const char* const pathStr = archive_entry_pathname(entry);

        const QString path = projectSettings->translationPath() % u'/' % QString::fromUtf8(pathStr);

        auto file = QFile(path);

        if (!file.open(QFile::WriteOnly)) {
            present(this, NOTICE("Writing entry %1 failed: %2", Warning, Modal, path, file.errorString()));
            archive_read_data_skip(arch);
            continue;
        }

        const i64 size = archive_entry_size(entry);
        char* const buf = new char[size];

        archive_read_data(arch, buf, size);
        file.write(buf, size);

        delete[] buf;
    }

    archive_read_close(arch);
    archive_read_free(arch);
}

void MainWindow::loadSettings() {
    qApp->setStyle(settings->appearance.style);
    qApp->styleHints()->setColorScheme(settings->appearance.theme);

    QString fontName;
    u8 fontSize;

    if (settings->appearance.translationTableFont.isEmpty()) {
        fontName = font().family();
    } else {
        fontName = settings->appearance.translationTableFont;
    }

    if (settings->appearance.translationTableFontSize == 0) {
        fontSize = font().pointSize();
    } else {
        fontSize = settings->appearance.translationTableFontSize;
    }

    ui->translationTable->setFont(QFont(fontName, fontSize));

    ui->tabPanel->setProgressDisplay(settings->appearance.displayPercents);

    actionTabPanel->setShortcut(settings->controls.tabPanel);
    actionSearchPanel->setShortcut(settings->controls.searchPanel);
    actionGoToRow->setShortcut(settings->controls.goToRow);
    actionBatchMenu->setShortcut(settings->controls.batchMenu);
    actionBookmarkMenu->setShortcut(settings->controls.bookmarkMenu);
    actionLintMenu->setShortcut(settings->controls.lintMenu);
    actionGlossaryMenu->setShortcut(settings->controls.glossaryMenu);
    actionTranslationsMenu->setShortcut(settings->controls.translationsMenu);

#ifdef ENABLE_NUSPELL
    if (projectSettings != nullptr) {
        initDictionary();
    }
#endif

    batchMenu->setEndpoints(settings->translation.endpoints);

    retranslate(settings->appearance.language);
}

void MainWindow::initializeSettings() {
    auto settingsFile = QFile(qApp->property("data-location").toString() + SETTINGS_PATH);

    if (settingsFile.open(QFile::ReadOnly)) {
        const QByteArray json = settingsFile.readAll();
        auto tSettings = glz::read_json<Settings>(string_view(json.data(), json.size()));

        if (tSettings) {
            settings = make_shared<Settings>(std::move(tSettings.value()));
        } else {
            qWarning().noquote() << u"Parsing settings.json failed: %1"_qsv.arg(
                glz::format_error(tSettings.error(), string_view(json.data(), json.size()))
            );
            settings = make_shared<Settings>();
        }
    } else {
        qWarning().noquote() << u"Failed to open settings.json: %1"_qsv.arg(settingsFile.errorString());
        settings = make_shared<Settings>();
    }

    // API keys live in the OS credential store, not settings.json - pull them back in here.
    // A missing entry just means this endpoint's key hasn't been saved since it was created
    // (or, for an endpoint from before this migration, hasn't been saved since); leaving
    // `apiKey` as whatever was already parsed from JSON is the right fallback in that case.
    for (auto& endpoint : settings->translation.endpoints) {
        if (endpoint.id.isEmpty()) {
            endpoint.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }

        FFIString storedKey;

        if (rpgm_get_credential(strtoffi(endpoint.id.toUtf8()), &storedKey)) {
            endpoint.apiKey = QString::fromUtf8(storedKey.ptr, storedKey.len);
            rpgm_string_free(storedKey);
        }
    }

    FFIString storedLtKey;

    if (rpgm_get_credential(strtoffi(LANGUAGETOOL_CREDENTIAL_ACCOUNT), &storedLtKey)) {
        settings->translation.languageTool.apiKey = QString::fromUtf8(storedLtKey.ptr, storedLtKey.len);
        rpgm_string_free(storedLtKey);
    }

    loadSettings();
}