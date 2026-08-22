local SCRIPTS_DIR = "./scripts"
local BUILD_DIR = "./build/headers"
local SCRIPTS_OUTPUT = BUILD_DIR .. "/scripts.h"

local IS_WINDOWS = os.execute("cmd /c exit 0") == true

local function run(cmd, mode)
    mode = mode or "r"

    -- POSIX popen takes "r" or "w" and nothing else, so a trailing "b" (meaningless there anyway,
    -- since there's no text mode to opt out of) is rejected outright.
    if not IS_WINDOWS then
        mode = mode:sub(1, 1)
    end

    local p = io.popen(cmd, mode)
    if not p then error("failed to run: " .. cmd) end
    local out = p:read("*a")
    p:close()
    return out
end

local function shell_quote(s)
    if IS_WINDOWS then
        return '"' .. s:gsub('"', '""') .. '"'
    else
        return "'" .. s:gsub("'", "'\\''") .. "'"
    end
end

local function mkdir_p(dir)
    if IS_WINDOWS then
        os.execute('if not exist "' .. dir .. '" mkdir "' .. dir .. '"')
    else
        os.execute("mkdir -p " .. shell_quote(dir))
    end
end

local function find_files(dir)
    local files = {}

    if IS_WINDOWS then
        local out = run('dir /s /b /a-d "' .. dir .. '"')
        for line in out:gmatch("[^\r\n]+") do
            table.insert(files, (line:gsub("\\", "/")))
        end
    else
        local out = run(string.format("find %s -type f -print0", shell_quote(dir)))
        for path in out:gmatch("([^%z]+)") do
            table.insert(files, path)
        end
    end

    return files
end

local function basename(path)
    return path:match("([^/]+)$") or path
end


local function to_c_name(relpath)
    local base = basename(relpath)
    local name = base:gsub("%-", "_")
    name = name:gsub("%..*$", "_SCRIPT")
    name = name:upper()
    return name
end

local function read_file(path)
    local f = io.open(path, "rb")
    if not f then error("failed to open file: " .. path) end
    local data = f:read("*a")
    f:close()
    return data
end

local function gen_scripts()
    local lines = { "#pragma once", "#include <QStringView>" }

    local files = find_files(SCRIPTS_DIR)
    table.sort(files)

    local prefix = SCRIPTS_DIR:gsub("^%./", "") .. "/"

    for _, path in ipairs(files) do
        if path:find(".prettierrc", 1, true) then goto continue end

        local norm = path:gsub("\\", "/")
        local relpath = norm
        local idx = relpath:find(prefix, 1, true)
        if idx then
            relpath = relpath:sub(idx + #prefix)
        end

        local name = to_c_name(relpath)
        local data = read_file(path)

        table.insert(lines,
            string.format("static constexpr QStringView %s = uR\"#(%s)#\";", name, data))
        ::continue::
    end

    local f = io.open(SCRIPTS_OUTPUT, "wb")
    if not f then error("failed to open output file: " .. SCRIPTS_OUTPUT) end
    f:write(table.concat(lines, "\n"))
    f:write("\n")
    f:close()

    io.stderr:write("Done: " .. SCRIPTS_OUTPUT .. "\n")
end

local function main()
    mkdir_p(BUILD_DIR)
    gen_scripts()
end

main()
