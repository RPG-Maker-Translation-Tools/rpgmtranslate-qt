#!/usr/bin/env lua

local WINDOWS = os.execute("cmd /c exit 0") == true
local CMAKE_FILE = "CMakeLists.txt"

local function shell_quote(s)
  if WINDOWS then
    return '"' .. s:gsub('"', '\\"') .. '"'
  else
    return "'" .. s:gsub("'", "'\\''") .. "'"
  end
end

local function show_help()
  print("Usage: ./configure [OPTIONS]\n")
  print("Standard options:")
  print("  --fresh                          Fresh reconfigure")
  print("  -B=DIR --dir=DIR --build-dir=DIR Build directory (build/ by default)")
  print("  -G=GEN --generator=GEN           CMake generator (Ninja, Unix Makefiles, etc.)")
  print("  --cc=PATH                        C compiler")
  print("  --cxx=PATH                       C++ compiler")
  print("  --ld=PATH                        Linker")
  print("  --ar=PATH                        Archiver")
  print("  --cflags=FLAGS                   C compiler flags")
  print("  --cxxflags=FLAGS                 C++ compiler flags")
  print("  --ldflags=FLAGS                  Linker flags\n")
  print("Project options (from " .. CMAKE_FILE .. "):\n")

  local f = io.open(CMAKE_FILE, "r")
  if f then
    local content = f:read("*a")
    f:close()

    for name, desc, def in content:gmatch(
      'option%s*%(%s*([%w_]+)%s*"([^"]*)"%s*([%w_]*)%s*%)'
    ) do
      -- collapse internal newlines/extra whitespace in multiline descriptions
      desc = desc:gsub("%s+", " ")
      local out = string.format("  %-30s %s", name, desc)
      if def ~= "" then out = out .. string.format(" (default: %s)", def) end
      print(out)
    end
  end
  print()
  os.exit(0)
end

-- match_opt("-B=dir", "-B", "--dir") -> "dir"; auto-escapes punctuation in prefixes
local function match_opt(a, ...)
  for _, prefix in ipairs({...}) do
    local pattern = "^" .. prefix:gsub("(%p)", "%%%1") .. "=(.*)$"
    local v = a:match(pattern)
    if v then return v end
  end
  return nil
end

local fresh, builddir, generator = false, "build", nil
local cc, cxx, ld, ar, cflags, cxxflags, ldflags
local extra = {}

for _, a in ipairs(arg) do
  if a == "--help" or a == "-h" then
    show_help()
  elseif a == "--fresh" then
    fresh = true
  else
    local v
    v = match_opt(a, "-B", "--dir", "--build-dir"); if v then builddir = v; goto continue end
    v = match_opt(a, "-G", "--generator"); if v then generator = v; goto continue end
    v = match_opt(a, "--cc"); if v then cc = v; goto continue end
    v = match_opt(a, "--cxx"); if v then cxx = v; goto continue end
    v = match_opt(a, "--ld"); if v then ld = v; goto continue end
    v = match_opt(a, "--ar"); if v then ar = v; goto continue end
    v = match_opt(a, "--cflags"); if v then cflags = v; goto continue end
    v = match_opt(a, "--cxxflags"); if v then cxxflags = v; goto continue end
    v = match_opt(a, "--ldflags"); if v then ldflags = v; goto continue end
    table.insert(extra, "-D" .. a)
  end
  ::continue::
end

local cmake_args = { "-S", ".", "-B", builddir }
if fresh then table.insert(cmake_args, "--fresh") end
if generator then table.insert(cmake_args, "-G"); table.insert(cmake_args, generator) end
if cc then table.insert(cmake_args, "-DCMAKE_C_COMPILER=" .. cc) end
if cxx then table.insert(cmake_args, "-DCMAKE_CXX_COMPILER=" .. cxx) end
if ld then table.insert(cmake_args, "-DCMAKE_LINKER=" .. ld) end
if ar then table.insert(cmake_args, "-DCMAKE_AR=" .. ar) end
if cflags then table.insert(cmake_args, "-DCMAKE_C_FLAGS=" .. cflags) end
if cxxflags then table.insert(cmake_args, "-DCMAKE_CXX_FLAGS=" .. cxxflags) end
if ldflags then table.insert(cmake_args, "-DCMAKE_EXE_LINKER_FLAGS=" .. ldflags) end
for _, e in ipairs(extra) do table.insert(cmake_args, e) end

local quoted = {}
for _, v in ipairs(cmake_args) do table.insert(quoted, shell_quote(v)) end

local ok, _, code = os.execute("cmake " .. table.concat(quoted, " "))
os.exit((ok == true or ok == 0) and 0 or (code or 1))