add_executable(mixvera_core_tests core/test_switcher_service.cpp)
target_link_libraries(mixvera_core_tests PRIVATE mixvera_core)
add_test(NAME mixvera_core_tests COMMAND mixvera_core_tests)

add_executable(mixvera_project_tests core/test_project.cpp)
target_link_libraries(mixvera_project_tests PRIVATE mixvera_core)
add_test(NAME mixvera_project_tests COMMAND mixvera_project_tests)

if(MIXVERA_BUILD_UI)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(mixvera_ui_smoke_tests ui/test_ui_smoke.cpp)
    target_link_libraries(mixvera_ui_smoke_tests
        PRIVATE mixvera_ui mixvera_engine Qt6::Test)
    add_test(NAME mixvera_ui_smoke_tests COMMAND mixvera_ui_smoke_tests)
    set_tests_properties(mixvera_ui_smoke_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(mixvera_media_tests media/test_media.cpp)
    target_link_libraries(mixvera_media_tests
        PRIVATE mixvera_ui Qt6::Widgets Qt6::Test)
    add_test(NAME mixvera_media_tests COMMAND mixvera_media_tests)
    set_tests_properties(mixvera_media_tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()