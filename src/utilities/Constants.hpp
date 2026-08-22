#pragma once

#include "Aliases.hpp"
#include "scripts.h"

#include <QTextFormat>

static constexpr i32 SECOND_MS = 1000;

// rpgm_decrypt_asset decrypts in place, so the RPG Maker header stays at the
// front of the returned buffer and must be skipped by consumers.
static constexpr i32 HEADER_LENGTH = 16;

static constexpr QL1SV NEW_LINE = "\\#"_L1;
static constexpr QL1SV LINE_FEED = "\n"_L1;
static constexpr QStringView SEPARATOR = u"<#>";

static constexpr QStringView PROGRAM_DATA_DIRECTORY = u"/.rpgmtranslate";
static constexpr QStringView MATCHES_DIRECTORY = u"/matches";
static constexpr QStringView BASELINE_DATA_DIRECTORY = u"/baseline-data";
static constexpr QStringView TRANSLATION_DIRECTORY = u"/translation";
static constexpr QStringView TEMP_MAPS_DIRECTORY = u"/temp-maps";
static constexpr QStringView LOG_FILE = u"/replacement-log.json";
static constexpr QStringView PROJECT_SETTINGS_FILE = u"/project-settings.json";
static constexpr QStringView BACKUP_DIRECTORY = u"/backups";
static constexpr QStringView GLOSSARY_FILE = u"/glossary.json";
static constexpr QStringView OUTPUT_DIRECTORY = u"/output";

static constexpr i32 MIN_BACKUP_PERIOD = 60;
static constexpr i32 MAX_BACKUP_PERIOD = 3600;
static constexpr i32 MAX_BACKUPS = 99;

static constexpr QStringView MAP_DISPLAY_NAME_COMMENT_PREFIX = u"<!-- IN-GAME DISPLAYED NAME: ";

static constexpr QStringView COMMENT_PREFIX = u"<!--";
static constexpr QStringView BOOKMARK_COMMENT = u"<!-- BOOKMARK -->";
static constexpr QStringView NAME_COMMENT = u"<!-- NAME -->";

static constexpr i32 DEFAULT_COLUMN_WIDTH = 768;

static constexpr f32 DEFAULT_FUZZY_THRESHOLD = 0.8;

static constexpr QChar LINE_SEPARATOR = QChar(0x2028);

static constexpr i32 LINT_ENTRIES_ID = QTextFormat::UserProperty + 10;

static constexpr i32 TOOLTIP_DELAY_MS = 1250;
