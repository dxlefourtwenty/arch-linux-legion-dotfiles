cmake_minimum_required(VERSION 3.20)
project(livepaper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(LayerShellQt REQUIRED)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(livepaper
    src/main.cpp
    src/livepapercontroller.cpp
    src/wallpapermodel.cpp
)

qt_add_qml_module(livepaper
    URI LivePaper
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
)

target_include_directories(livepaper PRIVATE src)

target_link_libraries(livepaper PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    LayerShellQt::Interface
)
