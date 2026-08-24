local IS_WINDOWS = os.execute("cmd /c exit 0") == true

local function list_dir(path)
    local entries = {}
    local cmd
    if IS_WINDOWS then
        cmd = string.format('dir "%s" /b', path)
    else
        cmd = string.format('ls -A "%s"', path)
    end
    local p = io.popen(cmd)
    if not p then return entries end
    local line = p:read("l")
    while line do
        local new_line = line:gsub("[\r\n]+$", "")

        if new_line ~= "" and new_line ~= "." and new_line ~= ".." then
            table.insert(entries, new_line)
        end

        line = p:read("l")
    end
    p:close()
    return entries
end

local function is_dir(path)
    if IS_WINDOWS then
        local p = io.popen(string.format('if exist "%s\\*" (echo YES) else (echo NO)', path))
        local res = p:read("*l")
        p:close()
        return res == "YES"
    else
        local p = io.popen(string.format('[ -d "%s" ] && echo YES || echo NO', path))
        local res = p:read("*l")
        p:close()
        return res == "YES"
    end
end

-- Shared UPX invocation for the binaries this project builds. --best --lzma --brute --ultra-brute
-- trades runtime (this only runs once per release) for maximum compression; --compress-exports=0
-- keeps export-table compression off.
local function upx_compress(paths)
    local ok = os.execute(
        "upx --best --lzma --brute --ultra-brute --compress-exports=0 " .. table.concat(paths, " "))
    if ok ~= true then
        io.stderr:write("upx failed to compress: " .. table.concat(paths, ", ") .. "\n")
        os.exit(1)
    end
end

local function join(a, b)
    if a:sub(-1) == "/" or a:sub(-1) == "\\" then
        return a .. b
    end
    return a .. "/" .. b
end

local LICENSE_ROOT = "./licenses"
local NOTICE_OUTPUT_FILE = "./THIRD-PARTY-NOTICE.md"

-- Single source of truth for each bundled library's license, keyed by its
-- subdirectory name under LICENSE_ROOT. The license/notice/patents files
-- present in a library's directory are still discovered automatically (that's
-- just a filename pattern, not a decision about what license applies), but
-- the SPDX identifier and any extra notice text are never inferred from file
-- contents - some libraries (e.g. libgit2, icu) merge multiple licenses into
-- one file, or package licenses in ways that don't reduce to "count the
-- files", so guessing from text is unreliable. Add an entry here for every
-- new dependency; the script refuses to run with an undocumented one, and
-- equally refuses to run when a documented one has no license text on disk yet.
--
-- Fields:
--   spdx           (required) SPDX identifier or expression for the library.
--   notes          (optional) extra strings appended to the notice's "info" column.
--   license_files  (optional) explicit file list, if automatic discovery
--                  (find_license_files) doesn't pick the right file(s) for
--                  this library.
--   vendored       (optional) true if the library's source lives directly under
--                  src/3rdparty and is compiled into rpgmtranslate regardless of
--                  platform. Everything else here is fetched/linked by CMake as a
--                  separate static library, which on Linux comes from the distro's
--                  shared packages instead - so only vendored (and Rust) libraries
--                  get bundled into the Linux notice/archive. See main().
-- An array rather than a keyed table: the bundled Lua has no pairs(), so the reverse check below
-- needs a defined iteration order. LIBRARIES is the name-keyed view, built from it right after.
local LIBRARY_LIST = {
    { name = "double-conversion", spdx = "BSD-3-Clause" },
    {
        name = "fast_float",
        spdx = "MIT",
        vendored = true,
        notes = {
            "Upstream is dual Apache-2.0/MIT; only the MIT license text is vendored here.",
            "Header-only.",
        }
    },
    {
        name = "ffmpeg",
        spdx = "LGPL-2.1-only",
        notes = { "The copy of FFmpeg linked is built with only built-in decoders and simple filters also licensed under LGPLv2.1." }
    },
    { name = "freetype",          spdx = "FTL" },
    { name = "glaze",             spdx = "MIT" },
    { name = "harfbuzz",          spdx = "LicenseRef-HarfBuzz-Old-MIT" },
    {
        name = "icu",
        spdx = "Unicode-3.0",
        notes = { "LICENSE.txt also bundles licenses for embedded third-party data (e.g. TaBE); those apply only to that data, not to ICU itself." }
    },
    { name = "jeaiii-itoa", spdx = "MIT",         vendored = true },
    {
        name = "libarchive",
        spdx = "BSD-2-Clause",
        notes = { "A handful of individual source files carry their own (compatible) licenses; see COPYING.txt for specifics." }
    },
    {
        name = "libgit2",
        spdx = "GPL-2.0-only",
        notes = { "COPYING.txt merges the GPLv2 text with libgit2's own linking exception, which permits linking the compiled library into other programs without extending the GPL to them." }
    },
    {
        name = "libjpeg-turbo",
        spdx = "IJG AND BSD-3-Clause",
        notes = { "The project relies on static linking terms, which also require us to say that this software is based in part on the work of the Independent JPEG Group. IJG README file is included (README.ijg)." }
    },
    { name = "liblzma",     spdx = "0BSD" },
    { name = "libpng",      spdx = "libpng-2.0" },
    { name = "libwebp",     spdx = "BSD-3-Clause" },
    {
        name = "lua",
        spdx = "MIT",
        notes = { "Build-time only: lua.exe drives configure.lua, prepare-headers.lua and this script. Not linked, not shipped." }
    },
    { name = "magic_enum",            spdx = "MIT",       vendored = true },
    { name = "material-design-icons", spdx = "Apache-2.0" },
    {
        name = "miniaudio",
        spdx = "Unlicense",
        vendored = true,
        notes = { "LICENSE file also includes MIT-0 text, but the project relies on Unlicense terms." }
    },
    { name = "nuspell", spdx = "LGPL-3.0-only" },
    { name = "pcre2",   spdx = "BSD-3-Clause WITH PCRE2-exception" },
    {
        name = "qt6",
        spdx = "GPL-3.0-only OR LGPL-3.0-only",
        notes = { "Both GPLv3 and LGPLv3 are present - the project relies on LGPLv3 terms." }
    },
    { name = "quickjs-ng", spdx = "MIT" },
    { name = "zlib-ng",    spdx = "Zlib" },
    { name = "zmij",       spdx = "MIT", vendored = true },
}

local LIBRARIES = {}
for _, entry in ipairs(LIBRARY_LIST) do
    LIBRARIES[entry.name] = entry
end

local function find_files_by_prefix(all_entries, prefix)
    local matches = {}
    local p = prefix:lower()
    for _, e in ipairs(all_entries) do
        if e:lower():match("^" .. p) then
            table.insert(matches, e)
        end
    end
    table.sort(matches)
    return matches
end

local function find_license_files(all_entries)
    local matches = {}
    for _, e in ipairs(all_entries) do
        if e:lower():match("license") or e:lower():match("licence") or e:lower():match("copying") or e == "FTL.TXT" then
            table.insert(matches, e)
        end
    end
    table.sort(matches)
    return matches
end

-- vendored_only restricts the notice to libraries whose source lives under src/3rdparty (see the
-- LIBRARY_LIST "vendored" field comment) - used on Linux, where every other library comes from the
-- distro's shared packages instead of being bundled into the archive.
local function gen_notice(vendored_only)
    local top_entries = list_dir(LICENSE_ROOT)
    local library_dirs = {}
    local present = {}
    local missing = {}

    for _, name in ipairs(top_entries) do
        -- licenses/rust isn't a LIBRARY_LIST entry - gen_rust_notice() generates it, and it gets
        -- its own hardcoded row in the notice output below instead of going through this table.
        if name ~= "rust" and is_dir(join(LICENSE_ROOT, name)) then
            local library = LIBRARIES[name]
            if not library then
                table.insert(missing, name)
            elseif not vendored_only or library.vendored then
                table.insert(library_dirs, name)
                present[name] = true
            end
        end
    end

    if #missing > 0 then
        io.stderr:write("No LIBRARIES entry for: " .. table.concat(missing, ", ") .. "\n")
        io.stderr:write("Add each one to the LIBRARIES table at the top of prepare-release.lua before running it.\n")
        os.exit(1)
    end

    -- The reverse check. A documented library with no directory means the license text was never
    -- fetched, and shipping a notice that points at a path which isn't in the archive is worse
    -- than not shipping one at all. Out-of-scope libraries (non-vendored, when vendored_only) are
    -- skipped here too - Linux never fetches their license text in the first place.
    local undocumented = {}
    for _, entry in ipairs(LIBRARY_LIST) do
        if (not vendored_only or entry.vendored) and not present[entry.name] then
            table.insert(undocumented, entry.name)
        end
    end

    if #undocumented > 0 then
        table.sort(undocumented)
        io.stderr:write("No license text under " .. LICENSE_ROOT .. " for: " .. table.concat(undocumented, ", ") .. "\n")
        io.stderr:write("Fetch each library's LICENSE/COPYING file from upstream into its own subdirectory.\n")
        os.exit(1)
    end

    local rows = {}

    for _, name in ipairs(library_dirs) do
        local subpath = join(LICENSE_ROOT, name)
        local library = LIBRARIES[name]
        local entries = list_dir(subpath)

        local license_files = library.license_files or find_license_files(entries)
        local notice_files = find_files_by_prefix(entries, "notice")
        local patents_files = find_files_by_prefix(entries, "patents")

        local rel_paths = {}
        for _, f in ipairs(license_files) do
            table.insert(rel_paths, subpath .. "/" .. f)
        end

        local info_parts = {}
        for _, note in ipairs(library.notes or {}) do
            table.insert(info_parts, note)
        end
        if #notice_files > 0 then
            table.insert(info_parts, "NOTICE file is preserved (" .. table.concat(notice_files, ", ") .. ")")
        end
        if #patents_files > 0 then
            table.insert(info_parts, "PATENTS file is preserved (" .. table.concat(patents_files, ", ") .. ")")
        end

        local info = table.concat(info_parts, "; ")
        if info == "" then info = "-" end

        local path_str = #rel_paths > 0 and table.concat(rel_paths, ", ") or "-"

        table.insert(rows, {
            name = name,
            spdx = library.spdx,
            path = path_str,
            info = info,
        })
    end

    table.sort(rows, function(a, b) return a.name:lower() < b.name:lower() end)

    local out = io.open(NOTICE_OUTPUT_FILE, "w")
    if not out then
        io.stderr:write("Failed to open " .. NOTICE_OUTPUT_FILE .. " for writing\n")
        os.exit(1)
    end

    out:write("# Third-Party Notices\n\n")
    out:write("| Subdirectory | SPDX Identifier | License Path | Additional Info |\n")
    out:write("|---|---|---|---|\n")

    for _, row in ipairs(rows) do
        out:write(string.format(
            "| %s | %s | %s | %s |\n",
            row.name, row.spdx, row.path, row.info
        ))
    end

    out:write(
        "| rust | - | ./licenses/rust/THIRD-PARTY-NOTICE.md | Check THIRD-PARTY-NOTICE.md for information about Rust crates linked statically. |\n")

    out:close()

    print(string.format("Wrote %d entries to %s", #rows, NOTICE_OUTPUT_FILE))
end

local function gen_rust_notice()
    if IS_WINDOWS then os.execute("mkdir licenses\\rust") else os.execute("mkdir licenses/rust") end
    os.execute("cd rust && cargo-about generate about.hbs -o ../licenses/rust/THIRD-PARTY-NOTICE.md")
end

-- Windows releases are static builds - every third-party library in LIBRARY_LIST is actually
-- linked into rpgmtranslate.exe, so the full notice and every licenses/ subdirectory get bundled.
local function main_windows()
    gen_notice()
    gen_rust_notice();

    os.execute("del /q rpgmtranslate.7z")
    os.execute("copy build\\target\\bin\\rpgmtranslate.exe rpgmtranslate.exe")
    upx_compress({ "rpgmtranslate.exe" })
    os.execute(
        "\"C:\\Program Files\\7-Zip\\7z.exe\" a -t7z -m0=lzma2 -mx=9 -md=128m -mfb=128 rpgmtranslate.7z THIRD-PARTY-NOTICE.md rpgmtranslate.exe licenses/")
    os.execute("del /q rpgmtranslate.exe")

    os.execute("rmdir /s /q licenses\\rust")
    os.execute("del /q THIRD-PARTY-NOTICE.md")
end

-- Linux builds link against distro-provided shared libraries, so bundling a notice (or license
-- text) for those would point at licenses for libraries that aren't actually in the archive. Only
-- the two categories that are genuinely compiled into rpgmtranslate regardless of platform still
-- apply: the Rust staticlib (gen_rust_notice) and the header/source files vendored under
-- src/3rdparty (LIBRARY_LIST entries with vendored = true).
local function main_linux()
    gen_notice(true)
    gen_rust_notice();

    local archive_paths = { "THIRD-PARTY-NOTICE.md", "licenses/rust" }
    for _, entry in ipairs(LIBRARY_LIST) do
        if entry.vendored then
            table.insert(archive_paths, "licenses/" .. entry.name)
        end
    end
    local archive_paths_str = table.concat(archive_paths, " ")

    os.execute("rm -f rpgmtranslate.tar.xz")
    os.execute("cp build/target/bin/rpgmtranslate rpgmtranslate")

    if (arg[1] == "--appimage") then
        upx_compress({ "rpgmtranslate" })
    else
        upx_compress({ "RPGMTranslate-x86_64.AppImage" })
    end

    os.execute("tar -cJf rpgmtranslate.tar.xz rpgmtranslate " .. archive_paths_str)
    os.execute("rm -f rpgmtranslate")

    os.execute("rm -rf licenses/rust")
    os.execute("rm -f THIRD-PARTY-NOTICE.md")
end

local function main()
    if IS_WINDOWS then
        main_windows()
    else
        main_linux()
    end
end

main();
