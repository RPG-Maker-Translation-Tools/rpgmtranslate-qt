# FindFFmpeg.cmake
#
# Result variables:
#   FFmpeg_FOUND
#   FFMPEG_INCLUDE_DIR
#   FFMPEG_LIBRARIES
#
# Imported targets:
#   FFmpeg::avcodec, FFmpeg::avformat, FFmpeg::avutil,
#   FFmpeg::avfilter, FFmpeg::swresample, FFmpeg::swscale

include(FindPackageHandleStandardArgs)

set(_FFMPEG_COMPONENTS avfilter avformat avcodec avutil swresample swscale)

find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    HINTS ${FFMPEG_ROOT} ${FFMPEG_ROOT}/include
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES ffmpeg
)

set(_FFMPEG_REQUIRED_VARS FFMPEG_INCLUDE_DIR)

# FFmpeg has no version of its own in the headers; libavutil's is the one reported
if(FFMPEG_INCLUDE_DIR AND EXISTS "${FFMPEG_INCLUDE_DIR}/libavutil/version.h")
    foreach(_part MAJOR MINOR MICRO)
        file(STRINGS "${FFMPEG_INCLUDE_DIR}/libavutil/version.h" _line
            REGEX "^#define[ \t]+LIBAVUTIL_VERSION_${_part}[ \t]+[0-9]+")
        string(REGEX REPLACE ".*LIBAVUTIL_VERSION_${_part}[ \t]+([0-9]+).*" "\\1" _FFMPEG_VERSION_${_part} "${_line}")
    endforeach()
    set(FFmpeg_VERSION "${_FFMPEG_VERSION_MAJOR}.${_FFMPEG_VERSION_MINOR}.${_FFMPEG_VERSION_MICRO}")
endif()

foreach(_lib ${_FFMPEG_COMPONENTS})
    string(TOUPPER ${_lib} _LIB_UPPER)

    find_library(${_LIB_UPPER}_LIBRARY
        NAMES ${_lib}
        HINTS ${FFMPEG_ROOT} ${FFMPEG_ROOT}/lib ${FFMPEG_ROOT}/bin
        PATHS /usr/lib64 /usr/lib
    )

    list(APPEND _FFMPEG_REQUIRED_VARS ${_LIB_UPPER}_LIBRARY)

    if(${_LIB_UPPER}_LIBRARY AND NOT TARGET FFmpeg::${_lib})
        add_library(FFmpeg::${_lib} UNKNOWN IMPORTED)
        set_target_properties(FFmpeg::${_lib} PROPERTIES
            IMPORTED_LOCATION "${${_LIB_UPPER}_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
        )
    endif()

    list(APPEND FFMPEG_LIBRARIES ${${_LIB_UPPER}_LIBRARY})
endforeach()

find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS ${_FFMPEG_REQUIRED_VARS}
    VERSION_VAR FFmpeg_VERSION
    FAIL_MESSAGE "FFmpeg not found. Install it or set -DFFMPEG_ROOT=<path>"
)

mark_as_advanced(FFMPEG_INCLUDE_DIR ${_FFMPEG_REQUIRED_VARS})