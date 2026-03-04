#pragma once

#include "Aliases.hpp"

// Time and data unit conversions
constexpr u16 SECOND_MS = 1000;

constexpr QLatin1StringView NEW_LINE = "\\#"_L1;
constexpr QLatin1StringView LINE_FEED = "\n"_L1;
constexpr QLatin1StringView SEPARATORL1 = "<#>"_L1;
constexpr QStringView SEPARATOR = u"<#>";

constexpr QLatin1StringView SETTINGS_PATH = "/settings.json"_L1;
constexpr QLatin1StringView PROGRAM_DATA_DIRECTORY = "/.rpgmtranslate"_L1;
constexpr QLatin1StringView MATCHES_DIRECTORY = "/matches"_L1;
constexpr QLatin1StringView TRANSLATION_DIRECTORY = "/translation"_L1;
constexpr QLatin1StringView TEMP_MAPS_DIRECTORY = "/temp-maps"_L1;
constexpr QLatin1StringView LOG_FILE = "/replacement-log.json"_L1;
constexpr QLatin1StringView PROJECT_SETTINGS_FILE = "/project-settings.json"_L1;
constexpr QLatin1StringView BACKUP_DIRECTORY = "/backups"_L1;
constexpr QLatin1StringView GLOSSARY_FILE = "/glossary.json"_L1;
constexpr QLatin1StringView OUTPUT_DIRECTORY = "/output"_L1;
constexpr QLatin1StringView RVPACKER_METADATA_FILE = "/.rvpacker-metadata"_L1;

constexpr u8 PERCENT_MULTIPLIER = 100;

constexpr u8 MIN_BACKUP_PERIOD = 60;
constexpr u16 MAX_BACKUP_PERIOD = 3600;

constexpr u8 MAX_BACKUPS = 99;

constexpr QLatin1StringView MAP_DISPLAY_NAME_COMMENT_PREFIX =
    "<!-- IN-GAME DISPLAYED NAME: "_L1;

constexpr QLatin1StringView TXT_EXTENSION = ".txt"_L1;
constexpr QLatin1StringView JSON_EXTENSION = ".json"_L1;

constexpr QLatin1StringView COMMENT_SUFFIX = " -->"_L1;
constexpr QLatin1StringView COMMENT_PREFIX = "<!--"_L1;
constexpr QLatin1StringView BOOKMARK_COMMENT = "<!-- BOOKMARK -->"_L1;
constexpr QLatin1StringView ID_COMMENT = "<!-- ID -->"_L1;
constexpr QLatin1StringView NAME_COMMENT = "<!-- NAME -->"_L1;

constexpr u16 DEFAULT_COLUMN_WIDTH = 768;

constexpr f32 DEFAULT_FUZZY_THRESHOLD = 0.8;

constexpr u8 MAX_RECENT_PROJECTS = 10;

constexpr QChar LINE_SEPARATOR = QChar(0x2028);
