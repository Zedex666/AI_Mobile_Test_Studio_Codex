set(DESKTOP_TARGET AI_Mobile_Test_Studio_Codex)

qt_add_executable(${DESKTOP_TARGET}
    WIN32 MACOSX_BUNDLE
    src/app/main.cpp
    src/core/device_command_catalog.cpp
    src/core/device_command_catalog.h
    src/core/workspace_catalog.cpp
    src/core/workspace_catalog.h
    src/services/adb_control_service.cpp
    src/services/adb_control_service.h
    src/services/adb_shell_session.cpp
    src/services/adb_shell_session.h
    src/services/appium_service.cpp
    src/services/appium_service.h
    src/services/apps_service.cpp
    src/services/apps_service.h
    src/services/display_service.cpp
    src/services/display_service.h
    src/services/device_center_service.cpp
    src/services/device_center_service.h
    src/services/file_manager_service.cpp
    src/services/file_manager_service.h
    src/services/logcat_service.cpp
    src/services/logcat_service.h
    src/services/other_service.cpp
    src/services/other_service.h
    src/services/overview_service.cpp
    src/services/overview_service.h
    src/services/package_manager_service.cpp
    src/services/package_manager_service.h
    src/services/performance_service.cpp
    src/services/performance_service.h
    src/services/process_service.cpp
    src/services/process_service.h
    src/services/recovery_service.cpp
    src/services/recovery_service.h
    src/services/scrcpy_service.cpp
    src/services/scrcpy_service.h
    src/services/conpty_session.cpp
    src/services/conpty_session.h
    src/services/terminal_service.cpp
    src/services/terminal_service.h
    src/services/automation_artifact_service.cpp
    src/services/automation_artifact_service.h
    src/services/terminal_session.h
    src/services/studio_control_protocol.cpp
    src/services/studio_control_protocol.h
    src/services/studio_operation_manager.cpp
    src/services/studio_operation_manager.h
    src/services/studio_control_server.cpp
    src/services/studio_control_server.h
    src/ui/common/widget_helpers.cpp
    src/ui/common/widget_helpers.h
    src/ui/common/app_preferences.cpp
    src/ui/common/app_preferences.h
    src/ui/components/main_window_sections.cpp
    src/ui/components/main_window_sections.h
    src/ui/pages/device_control_page.cpp
    src/ui/pages/device_control_page.h
    src/ui/pages/display_page.cpp
    src/ui/pages/display_page.h
    src/ui/pages/apps_page.cpp
    src/ui/pages/apps_page.h
    src/ui/pages/files_page.cpp
    src/ui/pages/files_page.h
    src/ui/pages/package_manager_page.cpp
    src/ui/pages/package_manager_page.h
    src/ui/pages/performance_page.cpp
    src/ui/pages/performance_page.h
    src/ui/pages/recovery_page.cpp
    src/ui/pages/recovery_page.h
    src/ui/pages/settings_page.cpp
    src/ui/pages/settings_page.h
    src/ui/pages/layout_page.cpp
    src/ui/pages/layout_page.h
    src/ui/pages/logcat_page.cpp
    src/ui/pages/logcat_page.h
    src/ui/pages/mirroring_page.cpp
    src/ui/pages/mirroring_page.h
    src/ui/pages/other_page.cpp
    src/ui/pages/other_page.h
    src/ui/pages/overview_page.cpp
    src/ui/pages/overview_page.h
    src/ui/pages/process_page.cpp
    src/ui/pages/process_page.h
    src/ui/pages/terminal_page.cpp
    src/ui/pages/terminal_page.h
    src/ui/pages/terminal_bridge.cpp
    src/ui/pages/terminal_bridge.h
    src/ui/pages/automation_page.cpp
    src/ui/pages/automation_page.h
    src/ui/styles/app_style.cpp
    src/ui/styles/app_style.h
    src/ui/motion/ui_motion_controller.cpp
    src/ui/motion/ui_motion_controller.h
    src/ui/motion/motion_controller.h
    src/ui/widgets/brand_badges.cpp
    src/ui/widgets/brand_badges.h
    src/ui/windows/main_window.cpp
    src/ui/windows/main_window.h
    src/ui/windows/device_center_window.cpp
    src/ui/windows/device_center_window.h
)

set_source_files_properties(
    src/ui/motion/ui_motion_controller.cpp
    src/ui/motion/ui_motion_controller.h
    src/ui/motion/motion_controller.h
    PROPERTIES SKIP_AUTOUIC ON)

target_compile_features(${DESKTOP_TARGET} PRIVATE cxx_std_17)
target_include_directories(${DESKTOP_TARGET} PRIVATE src)

set(APP_METADATA_JAR "${CMAKE_SOURCE_DIR}/resources/tools/java/app_metadata.jar")
file(GLOB_RECURSE TERMINAL_RUNTIME_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/resources/terminal-host/*"
    "${CMAKE_SOURCE_DIR}/resources/terminal-web/*"
    "${CMAKE_SOURCE_DIR}/resources/appium-inspector/*"
    "${CMAKE_SOURCE_DIR}/resources/opencode-extension/*"
    "${CMAKE_SOURCE_DIR}/resources/images/icons/*"
    "${CMAKE_SOURCE_DIR}/resources/font/*")
set_property(TARGET ${DESKTOP_TARGET} APPEND PROPERTY LINK_DEPENDS
    "${APP_METADATA_JAR}"
    ${TERMINAL_RUNTIME_RESOURCE_FILES})
add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/android"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_METADATA_JAR}"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/android/app_metadata.jar"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/terminal-web"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/terminal-web"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/terminal-host"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/terminal-host"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/appium-inspector"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/appium-inspector"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/images/icons"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/images/icons"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/fonts/cn"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/font/cn/LXGWWenKai-Regular.ttf"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/fonts/cn/LXGWWenKai-Regular.ttf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/font/cn/LXGWWenKai-Medium.ttf"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/fonts/cn/LXGWWenKai-Medium.ttf"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/font/us/JetBrainsMono-2.304/fonts/ttf"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf"
    VERBATIM
)

if(TARGET ai_mobile_test_stage_opencode_extension)
    add_dependencies(${DESKTOP_TARGET} ai_mobile_test_stage_opencode_extension)
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${AI_MOBILE_TEST_OPENCODE_EXTENSION_STAGE_ROOT}"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/opencode-extension"
        VERBATIM
    )
else()
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/opencode-extension"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/opencode-extension"
        VERBATIM
    )
endif()

if(TARGET ai_mobile_test_stage_terminal_runtime)
    add_dependencies(${DESKTOP_TARGET} ai_mobile_test_stage_terminal_runtime)
endif()

if(AI_MOBILE_TEST_PORTABLE_RUNTIME_ROOT)
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${AI_MOBILE_TEST_PORTABLE_RUNTIME_ROOT}"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime"
        VERBATIM
    )
endif()

if(AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE)
    if(NOT AI_MOBILE_TEST_TERMINAL_RUNTIME_GENERATED
       AND NOT EXISTS "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}")
        message(FATAL_ERROR
            "AI_MOBILE_TEST_OPENCODE_EXECUTABLE does not exist: "
            "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}")
    endif()
    get_filename_component(OPENCODE_EXECUTABLE_NAME
        "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}" NAME)
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/opencode"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/opencode/${OPENCODE_EXECUTABLE_NAME}"
        VERBATIM
    )
endif()

if(AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE)
    if(NOT AI_MOBILE_TEST_TERMINAL_RUNTIME_GENERATED
       AND NOT EXISTS "${AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE}")
        message(FATAL_ERROR
            "AI_MOBILE_TEST_NODE_EXECUTABLE does not exist: "
            "${AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE}")
    endif()
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/node"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE}"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/node/node.exe"
        VERBATIM
    )
endif()

if(AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE)
    if(NOT AI_MOBILE_TEST_TERMINAL_RUNTIME_GENERATED
       AND NOT EXISTS "${AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE}/package.json")
        message(FATAL_ERROR
            "AI_MOBILE_TEST_NODE_PTY_MODULE is not a node-pty package: "
            "${AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE}")
    endif()
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE}"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/node/node_modules/node-pty"
        VERBATIM
    )
endif()

if(AI_MOBILE_TEST_TERMINAL_RUNTIME_MANIFEST)
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AI_MOBILE_TEST_TERMINAL_RUNTIME_MANIFEST}"
            "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/manifest.json"
        VERBATIM
    )
endif()

if(WIN32 AND AI_MOBILE_TEST_DEPLOY_QT_RUNTIME)
    if(NOT TARGET Qt6::windeployqt)
        message(FATAL_ERROR
            "Qt6::windeployqt is unavailable; install the Qt 6 Core tools component "
            "or configure with -DAI_MOBILE_TEST_DEPLOY_QT_RUNTIME=OFF")
    endif()
    add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
        COMMAND "$<TARGET_FILE:Qt6::windeployqt>"
            "$<IF:$<CONFIG:Debug>,--debug,--release>"
            --compiler-runtime
            "$<TARGET_FILE:${DESKTOP_TARGET}>"
        COMMENT "Deploying Qt runtime beside ${DESKTOP_TARGET}"
        VERBATIM
    )
endif()

if(MSVC)
    target_compile_options(${DESKTOP_TARGET} PRIVATE /utf-8 /Zc:__cplusplus /permissive-)
endif()

target_link_libraries(${DESKTOP_TARGET}
    PRIVATE
        Qt::Core
        Qt::Network
        Qt::Widgets
)

if(TARGET Qt6::WebChannel AND TARGET Qt6::WebEngineWidgets)
    target_compile_definitions(${DESKTOP_TARGET}
        PRIVATE
            AI_MOBILE_TEST_STUDIO_HAS_WEBENGINE=1
            AI_MOBILE_TEST_STUDIO_HAS_WEB_TERMINAL=1)
    target_link_libraries(${DESKTOP_TARGET}
        PRIVATE Qt6::WebChannel Qt6::WebEngineWidgets)
else()
    message(STATUS
        "Qt WebEngine terminal unavailable (WebChannel=${Qt6WebChannel_FOUND}, "
        "WebEngineWidgets=${Qt6WebEngineWidgets_FOUND}); using the basic renderer")
endif()

include(GNUInstallDirs)

install(TARGETS ${DESKTOP_TARGET}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES "${APP_METADATA_JAR}" DESTINATION "runtime/android")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/terminal-web/"
    DESTINATION "runtime/terminal-web")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/terminal-host/"
    DESTINATION "runtime/terminal-host")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/appium-inspector/"
    DESTINATION "runtime/appium-inspector")
if(TARGET ai_mobile_test_stage_opencode_extension)
    install(DIRECTORY "${AI_MOBILE_TEST_OPENCODE_EXTENSION_STAGE_ROOT}/"
        DESTINATION "runtime/opencode-extension")
else()
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/opencode-extension/"
        DESTINATION "runtime/opencode-extension")
endif()
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/images/icons/"
    DESTINATION "runtime/images/icons")
install(FILES
    "${CMAKE_SOURCE_DIR}/resources/font/cn/LXGWWenKai-Regular.ttf"
    "${CMAKE_SOURCE_DIR}/resources/font/cn/LXGWWenKai-Medium.ttf"
    DESTINATION "runtime/fonts/cn")
install(DIRECTORY
    "${CMAKE_SOURCE_DIR}/resources/font/us/JetBrainsMono-2.304/fonts/ttf/"
    DESTINATION "runtime/fonts/us/JetBrainsMono-2.304/fonts/ttf"
    FILES_MATCHING PATTERN "*.ttf")
if(AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE)
    install(PROGRAMS "${AI_MOBILE_TEST_OPENCODE_RUNTIME_EXECUTABLE}"
        DESTINATION "runtime/opencode")
endif()
if(AI_MOBILE_TEST_PORTABLE_RUNTIME_ROOT)
    install(DIRECTORY "${AI_MOBILE_TEST_PORTABLE_RUNTIME_ROOT}/"
        DESTINATION "runtime")
endif()
if(AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE)
    install(PROGRAMS "${AI_MOBILE_TEST_NODE_RUNTIME_EXECUTABLE}"
        DESTINATION "runtime/node" RENAME "node.exe")
endif()
if(AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE)
    install(DIRECTORY "${AI_MOBILE_TEST_NODE_PTY_RUNTIME_MODULE}/"
        DESTINATION "runtime/node/node_modules/node-pty")
endif()
if(AI_MOBILE_TEST_TERMINAL_RUNTIME_MANIFEST)
    install(FILES "${AI_MOBILE_TEST_TERMINAL_RUNTIME_MANIFEST}"
        DESTINATION "runtime")
endif()

qt_generate_deploy_app_script(
    TARGET ${DESKTOP_TARGET}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
