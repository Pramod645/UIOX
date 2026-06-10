UIOX_Game/
├── CMakeLists.txt
├── android/
│   └── AndroidManifest.xml
├── assets/
│   ├── sprites/      ← PNG sprite sheets
│   └── sounds/       ← WAV/OGG audio
├── include/
│   ├── Game.h
│   ├── GameState.h
│   ├── Renderer.h
│   ├── InputManager.h
│   ├── AssetManager.h
│   ├── AudioManager.h
│   ├── Camera.h
│   ├── Physics.h
│   ├── Level.h
│   ├── Tile.h
│   ├── Entity.h
│   ├── Player.h
│   ├── Enemy.h
│   ├── Goomba.h
│   ├── Koopa.h
│   ├── Coin.h
│   ├── PowerUp.h
│   ├── Projectile.h
│   ├── ParticleSystem.h
│   ├── HUD.h
│   ├── Menu.h
│   └── Utils.h
└── src/
    ├── Game.cpp
    ├── Renderer.cpp
    ├── InputManager.cpp
    ├── AssetManager.cpp
    ├── AudioManager.cpp
    ├── Camera.cpp
    ├── Physics.cpp
    ├── Level.cpp
    ├── Tile.cpp
    ├── Entity.cpp
    ├── Player.cpp
    ├── Enemy.cpp
    ├── Goomba.cpp
    ├── Koopa.cpp
    ├── Coin.cpp
    ├── PowerUp.cpp
    ├── Projectile.cpp
    ├── ParticleSystem.cpp
    ├── HUD.cpp
    ├── Menu.cpp
    └── main.cpp
===================================================
Build Instructions:
===============================================
Install SDL2
# macOS
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf

# Ubuntu / Debian
sudo apt install libsdl2-dev libsdl2-image-dev \
                 libsdl2-mixer-dev libsdl2-ttf-dev

# Windows (vcpkg)
vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf

======================================================
macOS / Linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./MarioGame
======================================================
Windows
mkdir build; cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
.\Release\MarioGame.exe
==============================================================
Android
# Requires Android NDK + SDL2 Android project
mkdir build-android && cd build-android
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build .
# Copy libMarioGame.so into SDL2 Android project then gradle build
=======================================================================
Keyboard Controls:
Key	Action
← / →	Move left / right
Space / ↑	Jump
Shift / Z	Run
Ctrl / X	Shoot fireball (fire power)
P	Pause
ESC	Menu / back
====================
Touch Controls (Android)
Zone	Action
Bottom-left	Move left
Bottom-left +110px	Move right
Bottom-right 130px	Jump
Bottom-right 260px	Run / Shoot
==============================
Summary of Architecture:
main()
  └── Game                        ← state machine + main loop
        ├── Renderer              ← SDL window + renderer
        ├── InputManager          ← keyboard + gamepad + touch
        ├── AssetManager          ← textures + fonts (singleton)
        ├── AudioManager          ← music + SFX (singleton)
        ├── Menu                  ← main menu rendering + selection
        ├── HUD                   ← score / lives / time overlay
        └── Level                 ← world simulation
              ├── LevelData       ← 2D tile grid
              ├── Player          ← physics + input-driven movement
              ├── Goomba/Koopa    ← enemy AI + physics
              ├── Coin/PowerUp    ← collectibles
              ├── Projectile      ← fireballs
              ├── Camera          ← smooth scroll with lerp
              └── ParticleSystem  ← visual effects

Every frame the flow is:

handleEvents() — pump SDL events → InputManager
update(dt) — advance the active state (menu / playing / paused)
render() — draw the current state to screen
SDL_RenderPresent — swap the back buffer to the display
////////
The current simplified version runs physics once per render frame using FIXED_DT — good enough at 60 FPS target but physics would slow down at lower frame rates.

Component	Memory Strategy	Reason
Player	std::unique_ptr<Player> in Level	Level owns the player; destroyed when Level is destroyed
Entity list	std::vector<std::unique_ptr<Entity>>	Polymorphic ownership; automatic cleanup
SDL_Texture*	Raw pointer in AssetManager	SDL2 manages GPU memory; we call SDL_DestroyTexture in shutdown
TTF_Font*	Raw pointer in HUD/Menu	SDL_ttf manages font memory; we call TTF_CloseFont in destructor
Particle	std::vector<Particle> (value type)	Particles are small, non-polymorphic; value storage avoids heap allocation per particle
InputManager	Stack-allocated in Game	Single instance, known lifetime, no heap needed
Tile grid	std::vector<std::vector<Tile>>	2D grid of value types; Tile is a plain struct with no vtable
