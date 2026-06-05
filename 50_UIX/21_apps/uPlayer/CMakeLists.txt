cmake_minimum_required(VERSION 3.21)
project(UIXPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Find Qt6 ──────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Svg
)

qt_standard_project_setup()

# ── Sources ───────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Track.cpp
    src/Playlist.cpp
    src/Player.cpp
    src/MainWindow.cpp
    src/PlaylistWidget.cpp
    src/PlayerControls.cpp
    src/SeekSlider.cpp
    src/VolumeSlider.cpp
    src/TrackInfoWidget.cpp
    src/VisualizerWidget.cpp
)

set(HEADERS
    include/Track.h
    include/Playlist.h
    include/Player.h
    include/MainWindow.h
    include/PlaylistWidget.h
    include/PlayerControls.h
    include/SeekSlider.h
    include/VolumeSlider.h
    include/TrackInfoWidget.h
    include/VisualizerWidget.h
    include/StyleSheet.h
)

set(RESOURCES resources/resources.qrc)

# ── Platform-specific ─────────────────────────────────────────
if(ANDROID)
    qt_add_executable(UIXPlayer ${SOURCES} ${HEADERS} ${RESOURCES})
    set_property(TARGET UIXPlayer APPEND PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/android)
elseif(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE app_icon.png)
    qt_add_executable(UIXPlayer MACOSX_BUNDLE
        ${SOURCES} ${HEADERS} ${RESOURCES})
    set_target_properties(UIXPlayer PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER  "com.uiox.musicplayer"
        MACOSX_BUNDLE_BUNDLE_VERSION  ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
elseif(WIN32)
    qt_add_executable(UIXPlayer WIN32
        ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    qt_add_executable(UIXPlayer ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

# ── Includes & Link ───────────────────────────────────────────
target_include_directories(UIXPlayer PRIVATE include)

target_link_libraries(UIXPlayer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Svg
)

# ── Install ───────────────────────────────────────────────────
install(TARGETS UIXPlayer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET UIXPlayer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
