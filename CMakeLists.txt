cmake_minimum_required(VERSION 3.10)
project(CYPlayer VERSION 1.0.0 LANGUAGES C CXX)

# Set C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add custom modules path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Define build types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_WARNINGS -DCYPLAYER_EXPORTS -DCYPLAYER_USE_DLL)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
elseif(APPLE)
    add_definitions(-DMACOS -D_DARWIN -D_CRT_SECURE_NO_WARNINGS -DCYPLAYER_EXPORTS -DCYPLAYER_USE_DLL)
    set(CMAKE_MACOSX_RPATH ON)
elseif(UNIX)
    add_definitions(-DUNIX -D_LINUX -D_CRT_SECURE_NO_WARNINGS -DCYPLAYER_EXPORTS -DCYPLAYER_USE_DLL)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Set paths for dependencies
set(FFMPEG_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/ffmpeg" CACHE PATH "FFmpeg root directory")
set(SDL2_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/sdl2" CACHE PATH "SDL2 root directory")

# Find FFmpeg and SDL2 using our custom modules
find_package(FFmpeg REQUIRED)
find_package(SDL2 REQUIRED)

# Source files
set(SOURCES
    Src/CYPlayerFactory.cpp
    Src/CYPlayerImpl.cpp
    Src/ChainFilter/ChainFilterManager.cpp
    Src/ChainFilter/Common/cmdutils.c
    Src/ChainFilter/Common/CYAudioFilters.cpp
    Src/ChainFilter/Common/CYBaseFilter.cpp
    Src/ChainFilter/Common/CYDecoder.cpp
    Src/ChainFilter/Common/CYHWAccel.cpp
    Src/ChainFilter/Common/CYMediaClock.cpp
    Src/ChainFilter/Common/CYRenderer.cpp
    Src/ChainFilter/Common/CYVideoFilters.cpp
    Src/ChainFilter/Context/CYMediaContext.cpp
    Src/ChainFilter/DecodeFilter/CYAudioDecodeFilter.cpp
    Src/ChainFilter/DecodeFilter/CYSubTitleDecodeFilter.cpp
    Src/ChainFilter/DecodeFilter/CYVideoDecodeFilter.cpp
    Src/ChainFilter/DemuxFilter/CYDemuxFilter.cpp
    Src/ChainFilter/ProcessFilter/CYProcessFilter.cpp
    Src/ChainFilter/RenderFilter/CYAudioRenderFilter.cpp
    Src/ChainFilter/RenderFilter/CYVideoRenderFilter.cpp
    Src/ChainFilter/SourceFilter/CYSourceFilter.cpp
    Src/Common/Exception/CYBaseException.cpp
    Src/Common/Message/CYBaseMessage.cpp
    Src/Common/Queue/CYFrameQueue.cpp
    Src/Common/Queue/CYPacketQueue.cpp
    Src/Common/Structure/CYStringUtils.cpp
    Src/Common/Thread/CYCondition.cpp
    Src/Common/Time/CYTimeStamps.cpp
    Src/Logger/CYDebugString.cpp
    Src/Logger/CYLoggerManager.cpp
    Src/PipeLine/PipeLine.cpp
)

# Header files
set(HEADERS
    Inc/CYPlayer/CYPlayerDefine.hpp
    Inc/CYPlayer/CYPlayerFactory.hpp
    Inc/CYPlayer/ICYPlayer.hpp
    Src/ChainFilter/ChainFilterManager.hpp
    Src/ChainFilter/Common/cmdutils.h
    Src/ChainFilter/Common/CYAudioFilters.hpp
    Src/ChainFilter/Common/CYBaseFilter.hpp
    Src/ChainFilter/Common/CYDecoder.hpp
    Src/ChainFilter/Common/CYHWAccel.hpp
    Src/ChainFilter/Common/CYMediaClock.hpp
    Src/ChainFilter/Common/CYRenderer.hpp
    Src/ChainFilter/Common/CYVideoFilters.hpp
    Src/ChainFilter/Context/CYMediaContext.hpp
    Src/ChainFilter/DecodeFilter/CYAudioDecodeFilter.hpp
    Src/ChainFilter/DecodeFilter/CYSubTitleDecodeFilter.hpp
    Src/ChainFilter/DecodeFilter/CYVideoDecodeFilter.hpp
    Src/ChainFilter/DemuxFilter/CYDemuxFilter.hpp
    Src/ChainFilter/ProcessFilter/CYProcessFilter.hpp
    Src/ChainFilter/RenderFilter/CYAudioRenderFilter.hpp
    Src/ChainFilter/RenderFilter/CYVideoRenderFilter.hpp
    Src/ChainFilter/SourceFilter/CYSourceFilter.hpp
    Src/Common/CYCommonDefine.hpp
    Src/Common/CYFFmpegDefine.hpp
    Src/Common/Exception/CYBaseException.hpp
    Src/Common/Exception/CYException.hpp
    Src/Common/Message/CYBaseMessage.hpp
    Src/Common/Queue/CYFrameQueue.hpp
    Src/Common/Queue/CYPacketQueue.hpp
    Src/Common/Structure/CYStringUtils.hpp
    Src/Common/Thread/CYCondition.hpp
    Src/Common/Time/CYTimeStamps.hpp
    Src/CYPlayerImpl.hpp
    Src/CYPlayerPrivDefine.hpp
    Src/Logger/CYDebugString.hpp
    Src/Logger/CYLoggerManager.hpp
    Src/PipeLine/PipeLine.hpp
)

# Define the library target
add_library(CYPlayer SHARED ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(CYPlayer
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${FFMPEG_INCLUDE_DIR}
        ${SDL2_INCLUDE_DIR}
)

# Link against FFmpeg and SDL2 libraries
target_link_libraries(CYPlayer PRIVATE ${FFMPEG_LIBRARIES} ${SDL2_LIBRARIES})

# Platform-specific libraries
if(WIN32)
    target_link_libraries(CYPlayer PRIVATE winmm ws2_32)
elseif(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(COREMEDIA_LIBRARY CoreMedia)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(CYPlayer PRIVATE
        ${COREFOUNDATION_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COCOA_LIBRARY}
    )
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(CYPlayer PRIVATE ${CMAKE_THREAD_LIBS_INIT} dl)
endif()

# Set compiler flags
if(MSVC)
    # MSVC specific flags
    target_compile_options(CYPlayer PRIVATE 
        /W3 /MP /wd4996
        $<$<CONFIG:Release>:/O2 /Ob2>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
    
    # Set character set
    target_compile_definitions(CYPlayer PRIVATE _MBCS)
    
    # Set runtime library
    set_property(TARGET CYPlayer PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
else()
    # GCC/Clang flags
    target_compile_options(CYPlayer PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-O0 -g>
    )
endif()

# Install targets
install(TARGETS CYPlayer
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY Inc/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*"
)

# Add a simple test executable
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(Tests)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "CYPlayer configuration summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  FFmpeg found: ${FFMPEG_FOUND}")
message(STATUS "  FFmpeg version: ${FFMPEG_VERSION}")
message(STATUS "  SDL2 found: ${SDL2_FOUND}")
message(STATUS "  SDL2 version: ${SDL2_VERSION}")
message(STATUS "") 