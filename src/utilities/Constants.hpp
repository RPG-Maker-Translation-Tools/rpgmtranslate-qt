#pragma once

#include "Aliases.hpp"

// Time and data unit conversions
constexpr u16 SECOND_MS = 1000;

constexpr QL1SV NEW_LINE = "\\#"_L1;
constexpr QL1SV LINE_FEED = "\n"_L1;
constexpr QL1SV SEPARATORL1 = "<#>"_L1;
constexpr QStringView SEPARATOR = u"<#>";

constexpr QStringView SETTINGS_PATH = u"/settings.json";
constexpr QStringView PROGRAM_DATA_DIRECTORY = u"/.rpgmtranslate";
constexpr QStringView MATCHES_DIRECTORY = u"/matches";
constexpr QStringView BASELINE_DATA_DIRECTORY = u"/baseline-data";
constexpr QStringView TRANSLATION_DIRECTORY = u"/translation";
constexpr QStringView TEMP_MAPS_DIRECTORY = u"/temp-maps";
constexpr QStringView LOG_FILE = u"/replacement-log.json";
constexpr QStringView PROJECT_SETTINGS_FILE = u"/project-settings.json";
constexpr QStringView BACKUP_DIRECTORY = u"/backups";
constexpr QStringView GLOSSARY_FILE = u"/glossary.json";
constexpr QStringView OUTPUT_DIRECTORY = u"/output";
constexpr QStringView RVPACKER_METADATA_FILE = u"/.rvpacker-metadata";

constexpr u8 PERCENT_MULTIPLIER = 100;

constexpr u8 MIN_BACKUP_PERIOD = 60;
constexpr u16 MAX_BACKUP_PERIOD = 3600;

constexpr u8 MAX_BACKUPS = 99;

constexpr QL1SV MAP_DISPLAY_NAME_COMMENT_PREFIX =
    "<!-- IN-GAME DISPLAYED NAME: "_L1;

constexpr QL1SV TXT_EXTENSION = ".txt"_L1;
constexpr QL1SV JSON_EXTENSION = ".json"_L1;

constexpr QL1SV COMMENT_SUFFIX = " -->"_L1;
constexpr QL1SV COMMENT_PREFIX = "<!--"_L1;
constexpr QL1SV BOOKMARK_COMMENT = "<!-- BOOKMARK -->"_L1;
constexpr QL1SV ID_COMMENT = "<!-- ID -->"_L1;
constexpr QL1SV NAME_COMMENT = "<!-- NAME -->"_L1;

constexpr u16 DEFAULT_COLUMN_WIDTH = 768;

constexpr f32 DEFAULT_FUZZY_THRESHOLD = 0.8;

constexpr u8 MAX_RECENT_PROJECTS = 10;

constexpr QChar LINE_SEPARATOR = QChar(0x2028);
