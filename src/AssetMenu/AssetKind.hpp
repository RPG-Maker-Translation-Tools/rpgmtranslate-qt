#pragma once

#include "Aliases.hpp"

#include <QStringList>

// What AssetMenu lists and what AssetPreviewWidget can open are both decided by this one table, for every engine.
enum class AssetKind : u8 {
    Image,
    Font,
    Audio,
    Video,
    Data,
    Script,
    Save,
    Unknown,
};

struct AssetExtension {
    QL1SV extension;
    AssetKind kind;
};

// Lowercase, without the dot. Includes the encrypted MV/MZ variants.
static constexpr auto ASSET_EXTENSIONS = std::to_array<AssetExtension>({
    { "png_"_L1, AssetKind::Image },   { "rpgmvp"_L1, AssetKind::Image }, { "png"_L1, AssetKind::Image },
    { "jpg"_L1, AssetKind::Image },    { "bmp"_L1, AssetKind::Image },

    { "ttf"_L1, AssetKind::Font },     { "otf"_L1, AssetKind::Font },

    { "ogg_"_L1, AssetKind::Audio },   { "m4a_"_L1, AssetKind::Audio },   { "rpgmvo"_L1, AssetKind::Audio },
    { "rpgmvm"_L1, AssetKind::Audio }, { "ogg"_L1, AssetKind::Audio },    { "m4a"_L1, AssetKind::Audio },
    { "wav"_L1, AssetKind::Audio },    { "wma"_L1, AssetKind::Audio },    { "mp3"_L1, AssetKind::Audio },
    { "mid"_L1, AssetKind::Audio },    { "midi"_L1, AssetKind::Audio },

    { "webm"_L1, AssetKind::Video },   { "mp4"_L1, AssetKind::Video },    { "avi"_L1, AssetKind::Video },
    { "mpg"_L1, AssetKind::Video },    { "mpeg"_L1, AssetKind::Video },   { "wmv"_L1, AssetKind::Video },
    { "ogv"_L1, AssetKind::Video },

    { "json"_L1, AssetKind::Data },    { "rxdata"_L1, AssetKind::Data },  { "rvdata"_L1, AssetKind::Data },
    { "rvdata2"_L1, AssetKind::Data }, { "lmu"_L1, AssetKind::Data },     { "lmt"_L1, AssetKind::Data },
    { "ldb"_L1, AssetKind::Data },

    { "js"_L1, AssetKind::Script },

    { "lsd"_L1, AssetKind::Save },     { "rpgsave"_L1, AssetKind::Save },
});

// `extension` must be lowercase.
[[nodiscard]] inline auto assetKind(const QStringView extension) -> AssetKind {
    const auto found = ranges::find_if(ASSET_EXTENSIONS, [extension](const AssetExtension& entry) -> bool {
        return extension.compare(entry.extension) == 0;
    });

    return found == ASSET_EXTENSIONS.end() ? AssetKind::Unknown : found->kind;
}

// Name filters ("*.png", ...) for every extension of `kind`.
[[nodiscard]] inline auto assetFilters(const AssetKind kind) -> QStringList {
    QStringList filters;
    filters.reserve(ranges::count(ASSET_EXTENSIONS, kind, &AssetExtension::kind));

    for (const auto& entry : ASSET_EXTENSIONS) {
        if (entry.kind == kind) {
            filters.append(u"*."_s + entry.extension);
        }
    }

    return filters;
}
