if(WIN32)
    qt_add_executable(appium_service_smoke
        appium_service_smoke.cpp
        ../../apps/desktop/src/services/appium_service.cpp
        ../../apps/desktop/src/services/appium_service.h
    )
    target_compile_features(appium_service_smoke PRIVATE cxx_std_17)
    set_target_properties(appium_service_smoke PROPERTIES WIN32_EXECUTABLE FALSE)
    target_include_directories(appium_service_smoke PRIVATE ../../apps/desktop/src)
    target_link_libraries(appium_service_smoke PRIVATE Qt::Core Qt::Network)
    add_test(NAME appium_reuses_existing_server COMMAND appium_service_smoke)
    add_test(NAME appium_missing_runtime COMMAND appium_service_smoke --missing-runtime)
    set(_appium_smoke_tests appium_reuses_existing_server appium_missing_runtime)
    if(AI_MOBILE_TEST_PORTABLE_RUNTIME_ROOT)
        add_dependencies(appium_service_smoke ai_mobile_test_stage_terminal_runtime)
        add_test(NAME appium_bundled_runtime
            COMMAND appium_service_smoke
                --bundled-runtime "${AI_MOBILE_TEST_PORTABLE_RUNTIME_ROOT}")
        list(APPEND _appium_smoke_tests appium_bundled_runtime)
    endif()
    set_tests_properties(${_appium_smoke_tests} PROPERTIES
        TIMEOUT 10
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt::Core>\;$ENV{PATH}")
    if(TEST appium_bundled_runtime)
        set_tests_properties(appium_bundled_runtime PROPERTIES TIMEOUT 30)
    endif()

    add_executable(conpty_echo_helper conpty_echo_helper.cpp)
    set_target_properties(conpty_echo_helper PROPERTIES WIN32_EXECUTABLE FALSE)

    qt_add_executable(conpty_session_smoke
        conpty_session_smoke.cpp
        ../../apps/desktop/src/services/conpty_session.cpp
        ../../apps/desktop/src/services/conpty_session.h
        ../../apps/desktop/src/services/terminal_session.h
    )
    target_compile_features(conpty_session_smoke PRIVATE cxx_std_17)
    set_target_properties(conpty_session_smoke PROPERTIES WIN32_EXECUTABLE FALSE)
    target_include_directories(conpty_session_smoke PRIVATE ../../apps/desktop/src)
    target_link_libraries(conpty_session_smoke PRIVATE Qt::Core)
    if(MSVC)
        target_compile_options(appium_service_smoke PRIVATE
            /utf-8 /Zc:__cplusplus /permissive-)
        target_compile_options(conpty_session_smoke PRIVATE
            /utf-8 /Zc:__cplusplus /permissive-)
    endif()
    set(CONPTY_SMOKE_NODE_EXECUTABLE "" CACHE FILEPATH
        "Node-compatible executable used by the ConPTY smoke test")
    set(CONPTY_SMOKE_NODE_PTY_MODULE "" CACHE PATH
        "node-pty package used by the ConPTY smoke test")
    if(NOT CONPTY_SMOKE_NODE_EXECUTABLE
       AND AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE)
        set(CONPTY_SMOKE_NODE_EXECUTABLE
            "${AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE}")
    endif()
    if(NOT CONPTY_SMOKE_NODE_PTY_MODULE
       AND AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE)
        set(CONPTY_SMOKE_NODE_PTY_MODULE
            "${AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE}")
    endif()
    if(CONPTY_SMOKE_NODE_EXECUTABLE AND CONPTY_SMOKE_NODE_PTY_MODULE)
        add_dependencies(conpty_session_smoke conpty_echo_helper)
        if(TARGET ai_mobile_test_stage_terminal_runtime)
            add_dependencies(conpty_session_smoke ai_mobile_test_stage_terminal_runtime)
        endif()
        add_test(NAME conpty_session_smoke
            COMMAND conpty_session_smoke
                $<TARGET_FILE:conpty_echo_helper>
                "${CONPTY_SMOKE_NODE_EXECUTABLE}"
                "${CONPTY_SMOKE_NODE_PTY_MODULE}"
                "${CMAKE_SOURCE_DIR}/resources/terminal-host/conpty_host.js")
        set(_conpty_smoke_tests conpty_session_smoke)
        if(AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE
           AND AI_MOBILE_TEST_OPENCODE_RUNTIME_VERSION)
            add_test(NAME opencode_conpty_smoke
                COMMAND conpty_session_smoke
                    "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}"
                    "${CONPTY_SMOKE_NODE_EXECUTABLE}"
                    "${CONPTY_SMOKE_NODE_PTY_MODULE}"
                    "${CMAKE_SOURCE_DIR}/resources/terminal-host/conpty_host.js"
                    "${AI_MOBILE_TEST_OPENCODE_RUNTIME_VERSION}"
                    --version)
            list(APPEND _conpty_smoke_tests opencode_conpty_smoke)
        endif()
        set_tests_properties(${_conpty_smoke_tests} PROPERTIES
            TIMEOUT 15
            ENVIRONMENT
                "PATH=$<TARGET_FILE_DIR:Qt::Core>\\;$ENV{PATH}")
    else()
        message(STATUS
            "ConPTY smoke test not registered; configure CONPTY_SMOKE_NODE_EXECUTABLE "
            "and CONPTY_SMOKE_NODE_PTY_MODULE to enable it")
    endif()
endif()
