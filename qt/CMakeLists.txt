cmake_minimum_required(VERSION 3.16)

project(qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

qt_add_executable(appqt
    main.cpp
    mainwindow.h
    mainwindow.cpp
)

target_link_libraries(appqt
    PRIVATE Qt6::Widgets
    PRIVATE Qt6::Multimedia
    PRIVATE Qt6::MultimediaWidgets
)

set_target_properties(appqt PROPERTIES
    WIN32_EXECUTABLE TRUE
)
