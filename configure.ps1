#!/usr/bin/env pwsh

$CMakeFile = "CMakeLists.txt"

function Show-Help {
    Write-Host "Usage: ./configure.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Standard options:"
    Write-Host "  --fresh                                   Fresh reconfigure"
    Write-Host "  -B=DIR --dir=DIR --build-dir=DIR          Build directory (./ by default)"
    Write-Host "  -G=GEN --generator=GEN                    CMake generator (Ninja, Unix Makefiles, etc.)"
    Write-Host "  --cc=PATH                                 C compiler"
    Write-Host "  --cxx=PATH                                C++ compiler"
    Write-Host "  --ld=PATH                                 Linker"
    Write-Host "  --ar=PATH                                 Archiver"
    Write-Host "  --cflags=FLAGS                            C compiler flags"
    Write-Host "  --cxxflags=FLAGS                          C++ compiler flags"
    Write-Host "  --ldflags=FLAGS                           Linker flags"
    Write-Host ""
    Write-Host "Project options (from $CMakeFile):"
    Write-Host ""

    Get-Content $CMakeFile | ForEach-Object {
        if ($_ -match 'option\s*\(\s*([A-Za-z0-9_]+)\s*"([^"]*)"\s*([A-Za-z0-9_]*)') {
            "{0,-48} {1} (default: {2})" -f $Matches[1], $Matches[2], $Matches[3]
        }
    }
    exit
}

$CMakeFlags = @()
$ExtraArgs  = @()
$Generator  = $null

foreach ($arg in $args) {
    switch -regex ($arg) {
        '^--help$|^-h$'     { Show-Help }
        '^--fresh$' { $CMakeFlags += $($matches[0]) }
        '^-B=(.+)|^--dir=(.+)|^--build-dir=(.+)' { $CMakeFlags += $($matches[1]) }
        '^-G=(.+)|^--generator=(.+)' { $Generator = $matches[1] }
        '^--cc=(.+)'        { $CMakeFlags += "-DCMAKE_C_COMPILER=$($matches[1])" }
        '^--cxx=(.+)'       { $CMakeFlags += "-DCMAKE_CXX_COMPILER=$($matches[1])" }
        '^--ld=(.+)'        { $CMakeFlags += "-DCMAKE_LINKER=$($matches[1])" }
        '^--ar=(.+)'        { $CMakeFlags += "-DCMAKE_AR=$($matches[1])" }
        '^--cflags=(.+)'    { $CMakeFlags += "-DCMAKE_C_FLAGS=`"$($matches[1])`"" }
        '^--cxxflags=(.+)'  { $CMakeFlags += "-DCMAKE_CXX_FLAGS=`"$($matches[1])`"" }
        '^--ldflags=(.+)'   { $CMakeFlags += "-DCMAKE_EXE_LINKER_FLAGS=`"$($matches[1])`"" }
        default { $ExtraArgs += "-D" + $arg }
    }
}

if ($Generator) {
    cmake -S . -B build -G $Generator @CMakeFlags @ExtraArgs
} else {
    cmake -S . -B build @CMakeFlags @ExtraArgs
}
