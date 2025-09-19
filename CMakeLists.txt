cmake_minimum_required(VERSION 3.16)
project(PrevEve LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_FIND_DEBUG_MODE FALSE)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core DBus)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.20)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

qt_add_executable(
        PrevEve
        src/main.cpp
        src/MainWindow.cpp
        src/MainWindow.h
        src/MainWindow.ui
        src/ScreenCastHandler.cpp
        src/ScreenCastHandler.h
        src/StreamInfo.h
)

target_link_libraries(
        PrevEve
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::DBus
            ${GST_LIBRARIES}
)