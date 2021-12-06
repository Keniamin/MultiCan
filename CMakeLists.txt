cmake_minimum_required(VERSION 3.22)

project(MultiCan)

add_executable(${PROJECT_NAME}
    CanAn.cpp
    MainWindow.cpp
    MultiCan.cpp
    Common/ExtString.cpp
    Common/GridWindow.cpp
    Common/nPoint.cpp
    Common/TitleWindow.cpp
    MultiCan/Resources.rc
)

target_link_libraries(${PROJECT_NAME}
    -mwindows
    -static-libgcc
    -static-libstdc++
    comctl32
    winmm
    "C:/Program Files (x86)/HTML Help Workshop/lib/htmlhelp.lib"
)
