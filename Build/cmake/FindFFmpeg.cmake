# FindFFmpeg.cmake
#
# Find the FFmpeg libraries and includes
#
# This module defines:
#  FFMPEG_INCLUDE_DIR - Directory containing FFmpeg headers
#  FFMPEG_LIBRARIES - FFmpeg libraries to link against
#  FFMPEG_FOUND - True if FFmpeg has been found
#  FFMPEG_VERSION - FFmpeg version string (if available)
#
# Individual components:
#  AVCODEC_FOUND, AVCODEC_INCLUDE_DIR, AVCODEC_LIBRARY
#  AVDEVICE_FOUND, AVDEVICE_INCLUDE_DIR, AVDEVICE_LIBRARY
#  AVFILTER_FOUND, AVFILTER_INCLUDE_DIR, AVFILTER_LIBRARY
#  AVFORMAT_FOUND, AVFORMAT_INCLUDE_DIR, AVFORMAT_LIBRARY
#  AVUTIL_FOUND, AVUTIL_INCLUDE_DIR, AVUTIL_LIBRARY
#  SWRESAMPLE_FOUND, SWRESAMPLE_INCLUDE_DIR, SWRESAMPLE_LIBRARY
#  SWSCALE_FOUND, SWSCALE_INCLUDE_DIR, SWSCALE_LIBRARY
#  POSTPROC_FOUND, POSTPROC_INCLUDE_DIR, POSTPROC_LIBRARY

# Try using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(AVCODEC QUIET libavcodec)
  pkg_check_modules(AVDEVICE QUIET libavdevice)
  pkg_check_modules(AVFILTER QUIET libavfilter)
  pkg_check_modules(AVFORMAT QUIET libavformat)
  pkg_check_modules(AVUTIL QUIET libavutil)
  pkg_check_modules(SWRESAMPLE QUIET libswresample)
  pkg_check_modules(SWSCALE QUIET libswscale)
  pkg_check_modules(POSTPROC QUIET libpostproc)
endif()

# Handle custom FFmpeg path
if(FFMPEG_ROOT_DIR)
  set(_FFMPEG_INCLUDE_HINTS "${FFMPEG_ROOT_DIR}/include")
  
  # For Windows, handle debug and release paths
  if(WIN32)
    # Determine architecture suffix based on 32 or 64 bit
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_ARCH "x64")
    else()
      set(_ARCH "x86")
    endif()
    
    set(_FFMPEG_LIBRARY_HINTS_DEBUG
      "${FFMPEG_ROOT_DIR}/lib/Debug/${_ARCH}"
      "${FFMPEG_ROOT_DIR}/lib/Debug"
    )
    
    set(_FFMPEG_LIBRARY_HINTS_RELEASE
      "${FFMPEG_ROOT_DIR}/lib/Release/${_ARCH}"
      "${FFMPEG_ROOT_DIR}/lib/Release"
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(_FFMPEG_LIBRARY_HINTS ${_FFMPEG_LIBRARY_HINTS_DEBUG} ${_FFMPEG_LIBRARY_HINTS_RELEASE})
    else()
      set(_FFMPEG_LIBRARY_HINTS ${_FFMPEG_LIBRARY_HINTS_RELEASE} ${_FFMPEG_LIBRARY_HINTS_DEBUG})
    endif()
  else()
    # Unix/Mac paths
    set(_FFMPEG_LIBRARY_HINTS "${FFMPEG_ROOT_DIR}/lib")
  endif()
else()
  # Default system paths
  set(_FFMPEG_INCLUDE_HINTS 
    "/usr/include" 
    "/usr/local/include"
    "/opt/local/include"
    "/opt/csw/include"
    "/opt/include"
  )
  
  set(_FFMPEG_LIBRARY_HINTS 
    "/usr/lib" 
    "/usr/local/lib" 
    "/opt/local/lib"
    "/opt/csw/lib"
    "/opt/lib"
  )
  
  # On 64bit systems, search lib64 first
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(INSERT _FFMPEG_LIBRARY_HINTS 0 
      "/usr/lib64" 
      "/usr/local/lib64"
      "/opt/local/lib64"
    )
  endif()
endif()

# Find FFmpeg include directories
find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
  HINTS ${_FFMPEG_INCLUDE_HINTS}
  PATH_SUFFIXES "ffmpeg" "libav"
  DOC "FFmpeg include directory"
)

# Helper macro to find a library
macro(find_ffmpeg_library COMPONENT LIBRARY_NAME)
  # Windows typically adds lib prefix in the lib file
  if(WIN32)
    # Try both with and without lib prefix
    find_library(${COMPONENT}_LIBRARY
      NAMES 
        "${LIBRARY_NAME}" 
        "lib${LIBRARY_NAME}"
      HINTS ${_FFMPEG_LIBRARY_HINTS}
      DOC "${COMPONENT} library"
    )
  else()
    find_library(${COMPONENT}_LIBRARY
      NAMES "${LIBRARY_NAME}"
      HINTS ${_FFMPEG_LIBRARY_HINTS}
      DOC "${COMPONENT} library"
    )
  endif()
  
  if(${COMPONENT}_LIBRARY)
    set(${COMPONENT}_FOUND TRUE)
  endif()
endmacro()

# Find all required FFmpeg libraries
find_ffmpeg_library(AVCODEC avcodec)
find_ffmpeg_library(AVDEVICE avdevice)
find_ffmpeg_library(AVFILTER avfilter)
find_ffmpeg_library(AVFORMAT avformat)
find_ffmpeg_library(AVUTIL avutil)
find_ffmpeg_library(SWRESAMPLE swresample)
find_ffmpeg_library(SWSCALE swscale)
find_ffmpeg_library(POSTPROC postproc)

# Build the full list of libraries
set(FFMPEG_LIBRARIES
  ${AVCODEC_LIBRARY}
  ${AVDEVICE_LIBRARY}
  ${AVFILTER_LIBRARY}
  ${AVFORMAT_LIBRARY}
  ${AVUTIL_LIBRARY}
  ${SWRESAMPLE_LIBRARY}
  ${SWSCALE_LIBRARY}
  ${POSTPROC_LIBRARY}
)

# Try to get FFmpeg version from avcodec.h
if(FFMPEG_INCLUDE_DIR AND EXISTS "${FFMPEG_INCLUDE_DIR}/libavcodec/avcodec.h")
  file(STRINGS "${FFMPEG_INCLUDE_DIR}/libavcodec/avcodec.h" AVCODEC_VERSION_DEFINE
    REGEX "^#define[ \t]+LIBAVCODEC_VERSION_MAJOR[ \t]+[0-9]+$")
  
  if(AVCODEC_VERSION_DEFINE)
    string(REGEX REPLACE "^#define[ \t]+LIBAVCODEC_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" AVCODEC_VERSION_MAJOR "${AVCODEC_VERSION_DEFINE}")
    
    file(STRINGS "${FFMPEG_INCLUDE_DIR}/libavcodec/avcodec.h" AVCODEC_VERSION_MINOR_DEFINE
      REGEX "^#define[ \t]+LIBAVCODEC_VERSION_MINOR[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+LIBAVCODEC_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" AVCODEC_VERSION_MINOR "${AVCODEC_VERSION_MINOR_DEFINE}")
    
    file(STRINGS "${FFMPEG_INCLUDE_DIR}/libavcodec/avcodec.h" AVCODEC_VERSION_MICRO_DEFINE
      REGEX "^#define[ \t]+LIBAVCODEC_VERSION_MICRO[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+LIBAVCODEC_VERSION_MICRO[ \t]+([0-9]+)$" "\\1" AVCODEC_VERSION_MICRO "${AVCODEC_VERSION_MICRO_DEFINE}")
    
    set(FFMPEG_VERSION "${AVCODEC_VERSION_MAJOR}.${AVCODEC_VERSION_MINOR}.${AVCODEC_VERSION_MICRO}")
  endif()
endif()

# Check if all required components were found
if(FFMPEG_INCLUDE_DIR AND 
   AVCODEC_FOUND AND 
   AVDEVICE_FOUND AND 
   AVFILTER_FOUND AND 
   AVFORMAT_FOUND AND 
   AVUTIL_FOUND AND 
   SWRESAMPLE_FOUND AND 
   SWSCALE_FOUND AND 
   POSTPROC_FOUND)
  set(FFMPEG_FOUND TRUE)
else()
  set(FFMPEG_FOUND FALSE)
endif()

# Standard handling of the package arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
  VERSION_VAR FFMPEG_VERSION
  REQUIRED_VARS FFMPEG_INCLUDE_DIR FFMPEG_LIBRARIES FFMPEG_FOUND
  HANDLE_COMPONENTS
)

mark_as_advanced(
  FFMPEG_INCLUDE_DIR
  AVCODEC_INCLUDE_DIR AVCODEC_LIBRARY
  AVDEVICE_INCLUDE_DIR AVDEVICE_LIBRARY
  AVFILTER_INCLUDE_DIR AVFILTER_LIBRARY
  AVFORMAT_INCLUDE_DIR AVFORMAT_LIBRARY
  AVUTIL_INCLUDE_DIR AVUTIL_LIBRARY
  SWRESAMPLE_INCLUDE_DIR SWRESAMPLE_LIBRARY
  SWSCALE_INCLUDE_DIR SWSCALE_LIBRARY
  POSTPROC_INCLUDE_DIR POSTPROC_LIBRARY
) 