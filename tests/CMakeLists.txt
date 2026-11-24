add_executable(mixvera_core_tests core/test_switcher_service.cpp)
target_link_libraries(mixvera_core_tests PRIVATE mixvera_core)
add_test(NAME mixvera_core_tests COMMAND mixvera_core_tests)

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
endif()
