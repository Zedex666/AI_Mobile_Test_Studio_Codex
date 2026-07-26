cmake_minimum_required(VERSION 3.19)
project(AI_Mobile_Test_Studio_Codex LANGUAGES CXX)

include(CTest)

set(AI_MOBILE_TEST_OPENCODE_EXECUTABLE "" CACHE FILEPATH
    "Absolute path to a pinned OpenCode executable staged for development builds")
set(AI_MOBILE_TEST_NODE_EXECUTABLE "" CACHE FILEPATH
    "Absolute path to a pinned Node.js executable staged for terminal hosting")
set(AI_MOBILE_TEST_NODE_PTY_MODULE "" CACHE PATH
    "Absolute path to a node-pty package staged for terminal hosting")
option(AI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME
    "Download and verify the locked OpenCode terminal runtime during Windows builds" ON)
option(AI_MOBILE_TEST_DEPLOY_QT_RUNTIME
    "Deploy Qt DLLs, plugins, and WebEngine resources beside Windows build outputs" ON)
set(AI_MOBILE_TEST_RUNTIME_STAGE_ROOT
    "${CMAKE_BINARY_DIR}/runtime-stage/windows-x64" CACHE PATH
    "Build-local staging directory for the locked Windows terminal runtime")
set(AI_MOBILE_TEST_RUNTIME_CACHE_ROOT
    "${CMAKE_BINARY_DIR}/runtime-cache" CACHE PATH
    "Download cache for locked runtime archives")

set(_terminal_runtime_override_count 0)
foreach(_runtime_variable
        AI_MOBILE_TEST_OPENCODE_EXECUTABLE
        AI_MOBILE_TEST_NODE_EXECUTABLE
        AI_MOBILE_TEST_NODE_PTY_MODULE)
    if(NOT "${${_runtime_variable}}" STREQUAL "")
        math(EXPR _terminal_runtime_override_count
            "${_terminal_runtime_override_count} + 1")
    endif()
endforeach()
if(_terminal_runtime_override_count GREATER 0
   AND _terminal_runtime_override_count LESS 3)
    message(FATAL_ERROR
        "Configure all three terminal runtime overrides together: "
        "AI_MOBILE_TEST_OPENCODE_EXECUTABLE, AI_MOBILE_TEST_NODE_EXECUTABLE, "
        "and AI_MOBILE_TEST_NODE_PTY_MODULE")
endif()

set(AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE
    "${AI_MOBILE_TEST_OPENCODE_EXECUTABLE}")
set(AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE
    "${AI_MOBILE_TEST_NODE_EXECUTABLE}")
set(AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE
    "${AI_MOBILE_TEST_NODE_PTY_MODULE}")
set(AI_MOBILE_TEST_TERMINAL_RUNTIME_MANIFEST "")
set(AI_MOBILE_TEST_TERMINAL_RUNTIME_GENERATED FALSE)

if(WIN32 AND AI_MOBILE_TEST_STAGE_TERMINAL_RUNTIME
   AND _terminal_runtime_override_count EQUAL 0)
    find_program(AI_MOBILE_TEST_POWERSHELL_EXECUTABLE
        NAMES pwsh powershell REQUIRED)
    file(READ "${CMAKE_SOURCE_DIR}/tools/runtime/runtime-lock.json"
        _terminal_runtime_lock_json)
    string(JSON AI_MOBILE_TEST_OPENCODE_RUNTIME_VERSION
        GET "${_terminal_runtime_lock_json}" components 0 version)
    set(AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE
        "${AI_MOBILE_TEST_RUNTIME_STAGE_ROOT}/opencode/opencode.exe")
    set(AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE
        "${AI_MOBILE_TEST_RUNTIME_STAGE_ROOT}/node/node.exe")
    set(AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE
        "${AI_MOBILE_TEST_RUNTIME_STAGE_ROOT}/node/node_modules/node-pty")
    set(_terminal_runtime_manifest
        "${AI_MOBILE_TEST_RUNTIME_STAGE_ROOT}/manifest.json")
    set(_terminal_runtime_stamp
        "${AI_MOBILE_TEST_RUNTIME_STAGE_ROOT}/.complete")
    set(AI_MOBILE_TEST_TERMINAL_RUNTIME_MANIFEST
        "${_terminal_runtime_manifest}")
    add_custom_command(
        OUTPUT "${_terminal_runtime_stamp}"
        BYPRODUCTS
            "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}"
            "${AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE}"
            "${AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE}/package.json"
            "${_terminal_runtime_manifest}"
        COMMAND "${AI_MOBILE_TEST_POWERSHELL_EXECUTABLE}"
            -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/tools/runtime/stage-terminal-runtime.ps1"
            -Destination "${AI_MOBILE_TEST_RUNTIME_STAGE_ROOT}"
            -CacheDirectory "${AI_MOBILE_TEST_RUNTIME_CACHE_ROOT}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/runtime/stage-terminal-runtime.ps1"
            "${CMAKE_SOURCE_DIR}/tools/runtime/runtime-lock.json"
        COMMENT "Staging locked OpenCode terminal runtime"
        VERBATIM
    )
    add_custom_target(ai_mobile_test_stage_terminal_runtime
        DEPENDS "${_terminal_runtime_stamp}")
    set(AI_MOBILE_TEST_TERMINAL_RUNTIME_GENERATED TRUE)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Widgets)
get_filename_component(QT6_CMAKE_ROOT "${Qt6_DIR}" DIRECTORY)
find_package(Qt6WebChannel 6.5 QUIET CONFIG PATHS "${QT6_CMAKE_ROOT}" NO_DEFAULT_PATH)
find_package(Qt6WebEngineWidgets 6.5 QUIET CONFIG PATHS "${QT6_CMAKE_ROOT}" NO_DEFAULT_PATH)

qt_standard_project_setup()

add_subdirectory(apps/desktop)

if(BUILD_TESTING)
    add_subdirectory(tests/cpp)
endif()
