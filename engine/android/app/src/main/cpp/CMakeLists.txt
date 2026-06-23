cmake_minimum_required(VERSION 3.22.1)
project(mmorpg VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Source files ----
add_library(mmorpg SHARED
    # JNI bridge
    GameBridge.cpp

    # Engine
    engine/Shader.cpp
    engine/Mesh.cpp
    engine/Camera.cpp
    engine/Renderer.cpp
    engine/Terrain.cpp
    engine/GLTFLoader.cpp

    # Game
    game/Warrior.cpp
    game/Enemy.cpp
    game/World.cpp
)

# ---- Include paths ----
target_include_directories(mmorpg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# ---- Android system libraries ----
find_library(log-lib    log)
find_library(android-lib android)
find_library(gles3-lib  GLESv3)
find_library(egl-lib    EGL)

target_link_libraries(mmorpg
    ${log-lib}
    ${android-lib}
    ${gles3-lib}
    ${egl-lib}
)

# ---- Compiler flags ----
target_compile_options(mmorpg PRIVATE
    -O2
    -ffast-math
    -Wall
    -Wno-unused-parameter
    # Suppress tinygltf warnings
    -Wno-missing-field-initializers
    -Wno-unused-variable
)

# ---- Defines ----
target_compile_definitions(mmorpg PRIVATE
    TINYGLTF_NO_STB_IMAGE_WRITE
    TINYGLTF_USE_CPP14
)
