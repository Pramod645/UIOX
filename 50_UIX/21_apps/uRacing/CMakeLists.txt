#project needs at least CMake 3.21 — ensures modern features like qt_add_executable are available.
cmake_minimum_required(VERSION 3.21)
#sets the version to 1.0.0, and declares that only C++ is used.
project(MarioGame VERSION 1.0.0 LANGUAGES CXX)
#Forces C++17. REQUIRED ON means the build will fail immediately if the compiler doesn't support it — no silent downgrade.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#Searches the system for SDL2 and its three extension libraries. REQUIRED makes the build stop with an error if any library is missing rather than silently skipping it.
# ── Find SDL2 ─────────────────────────────────────────────────
find_package(SDL2       REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf   REQUIRED)

# ── Sources ───────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Renderer.cpp
    src/InputManager.cpp
    src/AssetManager.cpp
    src/AudioManager.cpp
    src/Camera.cpp
#    src/Physics.cpp
    src/Level.cpp
    src/Tile.cpp
    src/Entity.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/Goomba.cpp
    src/Koopa.cpp
    src/Coin.cpp
    src/PowerUp.cpp
    src/Projectile.cpp
    src/ParticleSystem.cpp
    src/HUD.cpp
    src/Menu.cpp
)
#On Android the game must be a shared library (.so) because the Android SDL2 Java activity loads it at runtime via System.loadLibrary().
# ── Platform target ───────────────────────────────────────────
if(ANDROID)
    add_library(MarioGame SHARED ${SOURCES})
else()
#On every other platform (macOS, Windows, Linux) we build a normal executable binary.
    add_executable(MarioGame ${SOURCES})
endif()
#Adds the include/ folder to the compiler's search path so #include "Game.h" works without specifying the full path.
target_include_directories(MarioGame PRIVATE include)
#Links the four SDL2 libraries into the final binary. PRIVATE means these dependencies are internal — nothing that links against MarioGame inherits them automatically.
target_link_libraries(MarioGame PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)
#On macOS wraps the binary into a proper .app bundle with a unique reverse-DNS identifier — required for App Store distribution and Gatekeeper signing.
# macOS bundle
if(APPLE)
    set_target_properties(MarioGame PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mario.game"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        RESOURCE "${CMAKE_SOURCE_DIR}/assets"
    )
endif()

#After the binary is built, automatically copies the assets/ folder next to it. 
#$<TARGET_FILE_DIR:MarioGame> expands to wherever the compiled binary ends up — so assets are always found at runtime.
# Copy assets to build dir (non-Android)
if(NOT ANDROID)
    add_custom_command(TARGET MarioGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:MarioGame>/assets
    )
endif()
