UIXPlayer/
├── CMakeLists.txt
├── android/
│   ├── AndroidManifest.xml
│   └── build.gradle
├── resources/
│   ├── icons/
│   │   ├── play.png
│   │   ├── pause.png
│   │   ├── stop.png
│   │   ├── next.png
│   │   ├── prev.png
│   │   ├── volume.png
│   │   ├── shuffle.png
│   │   ├── repeat.png
│   │   └── app_icon.png
│   └── resources.qrc
├── include/
│   ├── Track.h
│   ├── Playlist.h
│   ├── Player.h
│   ├── MainWindow.h
│   ├── PlaylistWidget.h
│   ├── PlayerControls.h
│   ├── SeekSlider.h
│   ├── VolumeSlider.h
│   ├── TrackInfoWidget.h
│   ├── VisualizerWidget.h
│   └── StyleSheet.h
└── src/
    ├── Track.cpp
    ├── Playlist.cpp
    ├── Player.cpp
    ├── MainWindow.cpp
    ├── PlaylistWidget.cpp
    ├── PlayerControls.cpp
    ├── SeekSlider.cpp
    ├── VolumeSlider.cpp
    ├── TrackInfoWidget.cpp
    ├── VisualizerWidget.cpp
    └── main.cpp
=====================================
Build Instructions
Prerequisites
# Install Qt6 with Multimedia
# Linux / macOS
brew install qt@6               # macOS
sudo apt install qt6-base-dev \
  qt6-multimedia-dev            # Linux

# Windows: Qt online installer from qt.io
# Android: Qt for Android via Qt Maintenance Tool
================================================
macOS
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
make -j$(nproc)
# Creates UIXPlayer.app — drag to /Applications
=====================================================
Windows
mkdir build; cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
cmake --build . --config Release
windeployqt Release/UIXPlayer.exe
=====================================================
Android
mkdir build-android && cd build-android
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DQT_HOST_PATH=$(qt-cmake --host-path) \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/android_arm64_v8a
cmake --build .
# Generates .apk in android-build/
===============================================
Keyboard Shortcuts
Key	Action
Space	Play / Pause
→	Next track
←	Previous track
Shift+→	Seek forward 10s
Shift+←	Seek back 10s
↑	Volume up
↓	Volume down
S	Stop
R	Cycle repeat
X	Toggle shuffle
M	Mute
F / F11	Fullscreen
Esc	Exit fullscreen
Ctrl+O	Open files
Ctrl+L	Toggle playlist
Ctrl+Q	Quit