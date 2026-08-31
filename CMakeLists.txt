cmake_minimum_required(VERSION 3.14)
project(ORBS) # Name :)

# ---  download Raylib (important) ---
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# --- Building the excecutable, ,all the includes ---
add_executable(ORBS
  main.cpp
  Theme.cpp
  atom.cpp
  log.cpp
  fileio.cpp
  Animation.h
  instancing.cpp
  config.h
  keyframe.h
  export.cpp
  smartcube.cpp
  ui.cpp
)

# --- Link Libraries ---
# This connects the graphics engine to your code
target_link_libraries(ORBS raylib)


# -----adding Raylibs gui library------
include(FetchContent)
FetchContent_Declare(
  raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(raygui)

# adding the include directory so the compiler can find it
target_include_directories(ORBS PRIVATE ${raygui_SOURCE_DIR}/src)

## --- making sure resources are always next to the build :) -----
add_custom_command(TARGET ORBS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:ORBS>/resources
)