cmake_minimum_required(VERSION 3.16)
project(OBC_Charger_Controller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

set(SOURCES
    main.cpp
    can_protocol.cpp
    can_worker.cpp
    simulator.cpp
    settings.cpp
    profiles.cpp
    ui/theme.cpp
    ui/main_window.cpp
    ui/connection_panel.cpp
    ui/control_panel.cpp
    ui/telemetry_panel.cpp
    ui/graph_panel.cpp
    ui/log_panel.cpp
)

set(HEADERS
    can_protocol.h
    can_worker.h
    simulator.h
    settings.h
    profiles.h
    ui/theme.h
    ui/main_window.h
    ui/connection_panel.h
    ui/control_panel.h
    ui/telemetry_panel.h
    ui/graph_panel.h
    ui/log_panel.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
