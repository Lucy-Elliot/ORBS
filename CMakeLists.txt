cmake_minimum_required(VERSION 3.14)
project(ORBS) # Renamed to ORBS based on your idea!

# --- 1. DOWNLOAD RAYLIB AUTOMATICALLY ---
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# --- 2. BUILD THE EXECUTABLE ---
add_executable(ORBS main.cpp)

# --- 3. LINK LIBRARIES ---
# This connects the graphics engine to your code
target_link_libraries(ORBS raylib)