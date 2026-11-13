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
static constexpr auto ASSET_EXTENSIONS = std::to_array<AssetExtension>(
    { { .extension = "png_"_L1, .kind = AssetKind::Image },   { .extension = "rpgmvp"_L1, .kind = AssetKind::Image },
      { .extension = "png"_L1, .kind = AssetKind::Image },    { .extension = "jpg"_L1, .kind = AssetKind::Image },
      { .extension = "bmp"_L1, .kind = AssetKind::Image },

      { .extension = "ttf"_L1, .kind = AssetKind::Font },     { .extension = "otf"_L1, .kind = AssetKind::Font },

      { .extension = "ogg_"_L1, .kind = AssetKind::Audio },   { .extension = "m4a_"_L1, .kind = AssetKind::Audio },
      { .extension = "rpgmvo"_L1, .kind = AssetKind::Audio }, { .extension = "rpgmvm"_L1, .kind = AssetKind::Audio },
      { .extension = "ogg"_L1, .kind = AssetKind::Audio },    { .extension = "m4a"_L1, .kind = AssetKind::Audio },
      { .extension = "wav"_L1, .kind = AssetKind::Audio },    { .extension = "wma"_L1, .kind = AssetKind::Audio },
      { .extension = "mp3"_L1, .kind = AssetKind::Audio },    { .extension = "mid"_L1, .kind = AssetKind::Audio },
      { .extension = "midi"_L1, .kind = AssetKind::Audio },

      { .extension = "webm"_L1, .kind = AssetKind::Video },   { .extension = "mp4"_L1, .kind = AssetKind::Video },
      { .extension = "avi"_L1, .kind = AssetKind::Video },    { .extension = "mpg"_L1, .kind = AssetKind::Video },
      { .extension = "mpeg"_L1, .kind = AssetKind::Video },   { .extension = "wmv"_L1, .kind = AssetKind::Video },
      { .extension = "ogv"_L1, .kind = AssetKind::Video },

      { .extension = "json"_L1, .kind = AssetKind::Data },    { .extension = "rxdata"_L1, .kind = AssetKind::Data },
      { .extension = "rvdata"_L1, .kind = AssetKind::Data },  { .extension = "rvdata2"_L1, .kind = AssetKind::Data },
      { .extension = "lmu"_L1, .kind = AssetKind::Data },     { .extension = "lmt"_L1, .kind = AssetKind::Data },
      { .extension = "ldb"_L1, .kind = AssetKind::Data },

      { .extension = "js"_L1, .kind = AssetKind::Script },

      { .extension = "lsd"_L1, .kind = AssetKind::Save },     { .extension = "rpgsave"_L1, .kind = AssetKind::Save },
      { .extension = "rmmzsave"_L1, .kind = AssetKind::Save } }
);

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
