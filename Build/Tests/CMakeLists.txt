# Tests CMakeLists.txt

# Add the test executable
add_executable(CYPlayerTest
    CYPlayerTest.cpp
)

# Link against CYPlayer library
target_link_libraries(CYPlayerTest PRIVATE CYPlayer)

# Copy the executable to the bin directory
set_target_properties(CYPlayerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add include directories
target_include_directories(CYPlayerTest PRIVATE
    ${CMAKE_SOURCE_DIR}/Inc
)

# On Windows, make sure we can find the runtime dependencies
if(WIN32)
    # Copy FFmpeg and SDL2 DLLs to the executable directory after build
    add_custom_command(TARGET CYPlayerTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<$<CONFIG:Debug>:${FFMPEG_ROOT_DIR}/lib/Debug/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/*.dll>
            $<$<CONFIG:Release>:${FFMPEG_ROOT_DIR}/lib/Release/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/*.dll>
            $<$<CONFIG:Debug>:${SDL2_ROOT_DIR}/lib/Debug/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/*.dll>
            $<$<CONFIG:Release>:${SDL2_ROOT_DIR}/lib/Release/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/*.dll>
            $<TARGET_FILE_DIR:CYPlayerTest>
    )
endif() 