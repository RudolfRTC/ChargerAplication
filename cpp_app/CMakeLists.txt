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
    can_factory.cpp
    can_socketcan_linux.cpp
    can_pcan_windows.cpp
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
    can_iface.h
    can_protocol.h
    can_worker.h
    can_factory.h
    can_socketcan_linux.h
    can_pcan_windows.h
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

# Qt resource file (embeds logo.png)
set(RESOURCES resources.qrc)

# Windows executable icon
if(WIN32)
    set(WIN_RC app.rc)
endif()

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES} ${WIN_RC})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# --- Platform-specific CAN backend configuration ---

if(WIN32)
    # PCAN-Basic: uses dynamic DLL loading (LoadLibrary) at runtime,
    # so no compile-time .lib dependency is required.
    # PCANBasic.dll must be in PATH or next to the executable at runtime.
    #
    # Optional: if you have the PCAN-Basic SDK installed and want static
    # linking instead of dynamic loading, uncomment the following lines
    # and set the correct path to your PCAN-Basic SDK:
    #
    # set(PCAN_SDK_DIR "C:/Program Files/PEAK-System/PCAN-Basic API")
    # target_include_directories(${PROJECT_NAME} PRIVATE "${PCAN_SDK_DIR}/Include")
    # target_link_libraries(${PROJECT_NAME} PRIVATE "${PCAN_SDK_DIR}/x64/VC_LIB/PCANBasic.lib")
    # target_compile_definitions(${PROJECT_NAME} PRIVATE PCAN_STATIC_LINK=1)
    #
    # Post-build step: copy PCANBasic.dll to the output directory (if available).
    # Adjust the path if your DLL is located elsewhere.
    set(PCAN_DLL_PATH "C:/Program Files/PEAK-System/PCAN-Basic API/x64/PCANBasic.dll")
    if(EXISTS "${PCAN_DLL_PATH}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${PCAN_DLL_PATH}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/PCANBasic.dll"
            COMMENT "Copying PCANBasic.dll to output directory"
        )
    endif()
endif()

if(UNIX AND NOT APPLE)
    # SocketCAN uses the system's CAN socket API (no extra libraries needed).
    # pthread is typically linked automatically by Qt, but ensure it's available.
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()
