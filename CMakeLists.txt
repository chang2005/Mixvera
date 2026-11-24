cmake_minimum_required(VERSION 3.28)

project(Mixvera
    VERSION 0.1.0
    DESCRIPTION "Mixvera professional live production switcher"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

option(MIXVERA_BUILD_UI "Build the Qt Widgets frontend" ON)
option(MIXVERA_BUILD_TESTS "Build unit tests" ON)
option(MIXVERA_ENABLE_LIBOBS "Link against a real libobs installation" OFF)
option(MIXVERA_ENABLE_WEBSOCKET "Build the WebSocket control surface" ON)

add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

add_library(mixvera_core STATIC
    src/mixvera/core/event_bus.cpp
    src/mixvera/core/service_container.cpp
    src/mixvera/core/json_document.cpp
    src/mixvera/core/application.cpp
    src/mixvera/core/domain_types.cpp
    src/mixvera/services/switcher_service.cpp
    src/mixvera/services/input_service.cpp
    src/mixvera/services/transition_service.cpp
    src/mixvera/services/audio_service.cpp
    src/mixvera/services/output_service.cpp
    src/mixvera/services/replay_service.cpp
    src/mixvera/services/device_service.cpp
    src/mixvera/control/control_server.cpp
)
target_include_directories(mixvera_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

add_library(mixvera_engine STATIC
    src/mixvera/engine/media_engine.cpp
)
target_include_directories(mixvera_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mixvera_engine PUBLIC mixvera_core)

if(MIXVERA_ENABLE_LIBOBS)
    find_package(libobs CONFIG REQUIRED)
    target_compile_definitions(mixvera_engine PUBLIC MIXVERA_HAS_LIBOBS=1)
    target_link_libraries(mixvera_engine PRIVATE libobs::libobs)
else()
    target_compile_definitions(mixvera_engine PUBLIC MIXVERA_STUB_ENGINE=1)
endif()

if(MIXVERA_BUILD_UI)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
    add_library(mixvera_ui STATIC
        src/mixvera/ui/main_window.cpp
        src/mixvera/ui/program_view.cpp
        src/mixvera/ui/preview_view.cpp
        src/mixvera/ui/multiview_panel.cpp
        src/mixvera/ui/tbar_widget.cpp
        src/mixvera/ui/mixer_panel.cpp
        src/mixvera/ui/status_bar.cpp
    )
    target_link_libraries(mixvera_ui PUBLIC mixvera_core Qt6::Core Qt6::Gui Qt6::Widgets)

    add_executable(Mixvera apps/mixvera/main.cpp)
    target_link_libraries(Mixvera PRIVATE mixvera_ui mixvera_engine Qt6::Widgets)
endif()

if(MIXVERA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(DIRECTORY src/mixvera/ DESTINATION include/mixvera FILES_MATCHING PATTERN "*.hpp")
