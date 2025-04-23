# FindSDL2.cmake
#
# Find the SDL2 libraries and includes
#
# This module defines:
#  SDL2_INCLUDE_DIR - Directory containing SDL2 headers
#  SDL2_LIBRARY - SDL2 library
#  SDL2_LIBRARIES - SDL2 libraries to link against
#  SDL2_FOUND - True if SDL2 has been found
#  SDL2_VERSION - SDL2 version string (if available)

# Try using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SDL2 QUIET sdl2)
endif()

# Handle custom SDL2 path
if(SDL2_ROOT_DIR)
  set(_SDL2_INCLUDE_HINTS "${SDL2_ROOT_DIR}/include")
  
  # For Windows, handle debug and release paths
  if(WIN32)
    # Determine architecture suffix based on 32 or 64 bit
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_ARCH "x64")
    else()
      set(_ARCH "x86")
    endif()
    
    set(_SDL2_LIBRARY_HINTS_DEBUG
      "${SDL2_ROOT_DIR}/lib/Debug/${_ARCH}"
      "${SDL2_ROOT_DIR}/lib/Debug"
    )
    
    set(_SDL2_LIBRARY_HINTS_RELEASE
      "${SDL2_ROOT_DIR}/lib/Release/${_ARCH}"
      "${SDL2_ROOT_DIR}/lib/Release"
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(_SDL2_LIBRARY_HINTS ${_SDL2_LIBRARY_HINTS_DEBUG} ${_SDL2_LIBRARY_HINTS_RELEASE})
    else()
      set(_SDL2_LIBRARY_HINTS ${_SDL2_LIBRARY_HINTS_RELEASE} ${_SDL2_LIBRARY_HINTS_DEBUG})
    endif()
  else()
    # Unix/Mac paths
    set(_SDL2_LIBRARY_HINTS "${SDL2_ROOT_DIR}/lib")
  endif()
else()
  # Default system paths
  set(_SDL2_INCLUDE_HINTS 
    "/usr/include/SDL2"
    "/usr/local/include/SDL2"
    "/opt/local/include/SDL2"
    "/opt/csw/include/SDL2"
    "/opt/include/SDL2"
  )
  
  set(_SDL2_LIBRARY_HINTS 
    "/usr/lib" 
    "/usr/local/lib" 
    "/opt/local/lib"
    "/opt/csw/lib"
    "/opt/lib"
  )
  
  # On 64bit systems, search lib64 first
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(INSERT _SDL2_LIBRARY_HINTS 0 
      "/usr/lib64" 
      "/usr/local/lib64"
      "/opt/local/lib64"
    )
  endif()
endif()

# Find SDL2 include directories
find_path(SDL2_INCLUDE_DIR SDL.h
  HINTS 
    ${PC_SDL2_INCLUDEDIR}
    ${_SDL2_INCLUDE_HINTS}
  PATH_SUFFIXES SDL2
)

# Find SDL2 library
if(WIN32)
  # On Windows the library is named differently for static and dynamic libraries
  find_library(SDL2_LIBRARY
    NAMES 
      SDL2 
      SDL2main
      # Static library
      SDL2-static
      # Debug libraries
      SDL2d
      SDL2maind
      SDL2-staticd
    HINTS
      ${PC_SDL2_LIBDIR}
      ${_SDL2_LIBRARY_HINTS}
  )
  
  # Windows typically needs SDL2main as well
  find_library(SDL2MAIN_LIBRARY
    NAMES SDL2main SDL2maind
    HINTS
      ${PC_SDL2_LIBDIR}
      ${_SDL2_LIBRARY_HINTS}
  )
  
  # Add SDL2main to the list of libraries if found
  if(SDL2MAIN_LIBRARY AND NOT SDL2MAIN_LIBRARY STREQUAL SDL2_LIBRARY)
    set(SDL2_LIBRARIES ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY})
  else()
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})
  endif()
else()
  # Unix/Mac name
  find_library(SDL2_LIBRARY
    NAMES SDL2
    HINTS
      ${PC_SDL2_LIBDIR}
      ${_SDL2_LIBRARY_HINTS}
  )
  
  set(SDL2_LIBRARIES ${SDL2_LIBRARY})
endif()

# Try to get SDL2 version from SDL_version.h
if(SDL2_INCLUDE_DIR AND EXISTS "${SDL2_INCLUDE_DIR}/SDL_version.h")
  file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_PATCHLEVEL[ \t]+[0-9]+$")
  
  string(REGEX REPLACE "^#define[ \t]+SDL_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_MAJOR "${SDL2_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_MINOR "${SDL2_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_PATCH "${SDL2_VERSION_PATCH_LINE}")
  
  set(SDL2_VERSION "${SDL2_VERSION_MAJOR}.${SDL2_VERSION_MINOR}.${SDL2_VERSION_PATCH}")
  unset(SDL2_VERSION_MAJOR_LINE)
  unset(SDL2_VERSION_MINOR_LINE)
  unset(SDL2_VERSION_PATCH_LINE)
  unset(SDL2_VERSION_MAJOR)
  unset(SDL2_VERSION_MINOR)
  unset(SDL2_VERSION_PATCH)
endif()

# Standard handling of the package arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
  REQUIRED_VARS SDL2_INCLUDE_DIR SDL2_LIBRARY
  VERSION_VAR SDL2_VERSION
)

mark_as_advanced(
  SDL2_INCLUDE_DIR
  SDL2_LIBRARY
  SDL2MAIN_LIBRARY
) 