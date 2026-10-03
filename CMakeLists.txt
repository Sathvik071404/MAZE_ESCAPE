cmake_minimum_required(VERSION 3.21)
project(MazeEscape LANGUAGES CXX)

# The game's window, audio, and OpenGL context are implemented with Windows APIs.
if(NOT WIN32)
    message(FATAL_ERROR "Maze Escape currently uses Win32 and WGL.")
endif()

# Build one self-contained C++17 Windows application.
add_executable(MazeEscape WIN32 src/main.cpp)
target_compile_features(MazeEscape PRIVATE cxx_std_17)
target_compile_definitions(MazeEscape PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
# These system libraries provide OpenGL, the window, WAV playback, and image decoding.
target_link_libraries(MazeEscape PRIVATE opengl32 user32 gdi32 winmm gdiplus)
# Copy textures, sounds, and the flashlight model beside the EXE for offline play.
add_custom_command(TARGET MazeEscape POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:MazeEscape>/assets")
