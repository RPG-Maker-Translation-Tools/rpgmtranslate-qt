#!/usr/bin/env lua

local WINDOWS = os.execute("cmd /c exit 0") == true
local CMAKE_FILE = "CMakeLists.txt"

local ALIAS_STRIP = { "ENABLE_", "WITH_", "USE_" }

local function shell_quote(s)
  if WINDOWS then
    return '"' .. s:gsub('"', '\\"') .. '"'
  else
    return "'" .. s:gsub("'", "'\\''") .. "'"
  end
end

local OPTS = {
  { s = "-f", l = { "--fresh" },              desc = "Fresh reconfigure" },
  { s = "-n", l = { "--dry-run" },            desc = "Print the cmake arguments instead of running cmake" },
  { s = "-B", l = { "--dir", "--build-dir" }, arg = "DIR",                                                desc = "Build directory (build/ by default)" },
  { s = "-G", l = { "--generator" },          arg = "GEN",                                                desc = "CMake generator (Ninja, Unix Makefiles, etc.)" },
  { s = "-t", l = { "--toolchain" },          arg = "FILE",                                               var = "CMAKE_TOOLCHAIN_FILE",                          desc = "CMake toolchain file" },
  { s = "-c", l = { "--cc" },                 arg = "PATH",                                               var = "CMAKE_C_COMPILER",                              desc = "C compiler" },
  { s = "-x", l = { "--cxx" },                arg = "PATH",                                               var = "CMAKE_CXX_COMPILER",                            desc = "C++ compiler" },
  { s = "-l", l = { "--ld" },                 arg = "PATH",                                               var = "CMAKE_LINKER",                                  desc = "Linker" },
  { s = "-a", l = { "--ar" },                 arg = "PATH",                                               var = "CMAKE_AR",                                      desc = "Archiver" },
  { s = "-C", l = { "--cflags" },             arg = "FLAGS",                                              var = "CMAKE_C_FLAGS",                                 desc = "C compiler flags" },
  { s = "-X", l = { "--cxxflags" },           arg = "FLAGS",                                              var = "CMAKE_CXX_FLAGS",                               desc = "C++ compiler flags" },
  { s = "-L", l = { "--ldflags" },            arg = "FLAGS",                                              var = "CMAKE_EXE_LINKER_FLAGS",                        desc = "Linker flags" },
}

local byname = {}
for _, o in ipairs(OPTS) do
  byname[o.s] = o
  for _, n in ipairs(o.l) do byname[n] = o end
end

-- User-facing project settings: option() calls and documented cache entries,
-- set(NAME value CACHE TYPE "doc"). Each is { name, doc, default, bool, alias }.
local function scan_cmake()
  local project = {}
  local f = io.open(CMAKE_FILE, "r")
  if not f then return project end
  local content = f:read("a")
  f:close()

  for name, doc, default in content:gmatch(
    '%f[%w_]option%s*%(%s*([%w_]+)%s*"([^"]*)"%s*([%w_]*)%s*%)'
  ) do
    project[#project + 1] = { name = name, doc = doc:gsub("%s+", " "), default = default, bool = true }
  end

  for name, value, kind, doc in content:gmatch(
    '%f[%w_]set%s*%(%s*([%w_]+)%s+([^%)]-)%s+CACHE%s+(%u+)%s+"([^"]*)"'
  ) do
    if kind ~= "INTERNAL" then
      project[#project + 1] = {
        name = name,
        doc = doc:gsub("%s+", " "),
        default = value:match('^%s*"?(.-)"?%s*$'),
        bool = kind == "BOOL",
      }
    end
  end
  return project
end

-- Aliases are derived from the CMake names on every run, so there is nothing to regenerate
-- when CMakeLists.txt changes. Wrapper options win on a clash, then the first project entry.
local function register_aliases(project)
  for _, e in ipairs(project) do
    local base = e.name
    for _, p in ipairs(ALIAS_STRIP) do
      if base:sub(1, #p) == p then
        base = base:sub(#p + 1)
        break
      end
    end
    base = base:lower():gsub("_", "-")

    if e.bool then
      if not byname["--" .. base] and not byname["--no-" .. base] then
        byname["--" .. base] = { var = e.name, fixed = "ON", optval = true }
        byname["--no-" .. base] = { var = e.name, fixed = "OFF" }
        e.alias = "--[no-]" .. base
      end
    elseif not byname["--" .. base] then
      byname["--" .. base] = { var = e.name, arg = true }
      e.alias = "--" .. base
    end
  end
end

local function print_rows(rows)
  local width = 0
  for _, r in ipairs(rows) do
    if #r[1] > width then width = #r[1] end
  end
  local fmt = string.format("  %%-%ds  %%s", width)
  for _, r in ipairs(rows) do print(string.format(fmt, r[1], r[2])) end
  print()
end

local function show_help(project)
  print("Usage: ./configure [OPTIONS] [NAME=VALUE...]\n")
  print("Values can be given as `-B DIR`, `-BDIR`, `-B=DIR`, `--dir DIR` or `--dir=DIR`.")
  print("NAME=VALUE sets a project option/variable (passed to CMake as -DNAME=VALUE); -D... is passed as is.")
  print("Project settings also have the CLI aliases shown in parentheses below: `--[no-]x` for on/off")
  print("options (`--x=OFF` also works), `--x VALUE` for the rest.\n")
  print("Standard options:")

  local rows = { { "-h, --help", "Show this help" } }
  for _, o in ipairs(OPTS) do
    local left = o.s .. ", " .. table.concat(o.l, ", ")
    if o.arg then left = left .. " " .. o.arg end
    rows[#rows + 1] = { left, o.desc }
  end
  print_rows(rows)

  for _, section in ipairs({ { true, "options" }, { false, "variables" } }) do
    rows = {}
    for _, e in ipairs(project) do
      if (e.bool or false) == section[1] then
        local text = e.doc
        if e.alias then text = "(" .. e.alias .. ") " .. text end
        if e.default ~= "" then text = text .. " (default: " .. e.default .. ")" end
        rows[#rows + 1] = { e.name, text }
      end
    end
    print("Project " .. section[2] .. " (from " .. CMAKE_FILE .. "):")
    print_rows(rows)
  end
  os.exit(0)
end

local function die(msg)
  io.stderr:write("configure: ", msg, "\n")
  os.exit(2)
end

local project = scan_cmake()
register_aliases(project)

local vals, defs = {}, {}
local i = 1
while i <= #arg do
  local a = arg[i]
  i = i + 1
  if a == "-h" or a == "--help" then show_help(project) end

  local name, val
  if a:sub(1, 2) == "--" then
    name, val = a:match("^(%-%-[^=]+)=(.*)$")
    name = name or a
  elseif a:sub(1, 1) == "-" and #a > 2 then
    name = a:sub(1, 2)
    val = a:sub(3):gsub("^=", "")
  else
    name = a
  end

  local opt = byname[name]
  if opt then
    if opt.arg then
      if val == nil then
        val = arg[i]
        i = i + 1
        if val == nil then die(name .. " requires a value") end
      end
    elseif opt.fixed then
      if val == nil then
        val = opt.fixed
      elseif not opt.optval then
        die(name .. " takes no value")
      end
    elseif val ~= nil then
      die(name .. " takes no value")
    else
      val = true
    end

    if opt.var then
      defs[#defs + 1] = "-D" .. opt.var .. "=" .. val
    else
      vals[opt.l[1]] = val
    end
  elseif a:sub(1, 2) == "-D" then
    defs[#defs + 1] = a
  elseif a:sub(1, 1) == "-" then
    die("unknown option: " .. a)
  else
    defs[#defs + 1] = "-D" .. a
  end
end

local cmake_args = { "-S", ".", "-B", vals["--dir"] or "build" }
if vals["--fresh"] then cmake_args[#cmake_args + 1] = "--fresh" end
if vals["--generator"] then
  cmake_args[#cmake_args + 1] = "-G"
  cmake_args[#cmake_args + 1] = vals["--generator"]
end
for _, d in ipairs(defs) do cmake_args[#cmake_args + 1] = d end

local quoted = {}
for _, v in ipairs(cmake_args) do quoted[#quoted + 1] = shell_quote(v) end

if vals["--dry-run"] then
  print(table.concat(quoted, " "))
  os.exit(0)
end

local ok, _, code = os.execute("cmake " .. table.concat(quoted, " "))
os.exit((ok == true or ok == 0) and 0 or (code or 1))
