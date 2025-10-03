cmake_minimum_required(VERSION 3.16)
project(eve-w-preview LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_FIND_DEBUG_MODE FALSE)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4.2 EXACT REQUIRED COMPONENTS Widgets Core DBus Gui)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.20)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

qt_add_executable(
        ${PROJECT_NAME}
        src/main.cpp
        src/StreamManager.cpp
        src/StreamManager.h
        src/Data/PortalStreamInfo.h
        src/Thumbnail.ui
        src/Thumbnail.h
        src/Thumbnail.cpp
        src/KWinManager.cpp
        src/KWinManager.h
        resources.qrc
        src/ConfigManager.cpp
        src/ConfigManager.h
        src/Data/ThumbnailPosition.h
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PIPEWIRE_INCLUDE_DIRS})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::DBus
        Qt::Gui
        ${GST_LIBRARIES}
        ${PIPEWIRE_LIBRARIES}
)
