set(DESKTOP_TARGET AI_Mobile_Test_Studio_Codex)

qt_add_executable(${DESKTOP_TARGET}
    WIN32 MACOSX_BUNDLE
    src/app/main.cpp
    src/core/device_command_catalog.cpp
    src/core/device_command_catalog.h
    src/services/adb_control_service.cpp
    src/services/adb_control_service.h
    src/services/apps_service.cpp
    src/services/apps_service.h
    src/services/file_manager_service.cpp
    src/services/file_manager_service.h
    src/services/package_manager_service.cpp
    src/services/package_manager_service.h
    src/services/performance_service.cpp
    src/services/performance_service.h
    src/services/recovery_service.cpp
    src/services/recovery_service.h
    src/services/scrcpy_service.cpp
    src/services/scrcpy_service.h
    src/ui/common/widget_helpers.cpp
    src/ui/common/widget_helpers.h
    src/ui/components/main_window_sections.cpp
    src/ui/components/main_window_sections.h
    src/ui/pages/device_control_page.cpp
    src/ui/pages/device_control_page.h
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
    src/ui/styles/app_style.cpp
    src/ui/styles/app_style.h
    src/ui/widgets/brand_badges.cpp
    src/ui/widgets/brand_badges.h
    src/ui/windows/main_window.cpp
    src/ui/windows/main_window.h
)

target_compile_features(${DESKTOP_TARGET} PRIVATE cxx_std_17)
target_include_directories(${DESKTOP_TARGET} PRIVATE src)

set(APP_METADATA_JAR "${CMAKE_SOURCE_DIR}/resources/tools/java/app_metadata.jar")
add_custom_command(TARGET ${DESKTOP_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/android"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_METADATA_JAR}"
        "$<TARGET_FILE_DIR:${DESKTOP_TARGET}>/runtime/android/app_metadata.jar"
    VERBATIM
)

if(MSVC)
    target_compile_options(${DESKTOP_TARGET} PRIVATE /utf-8 /Zc:__cplusplus /permissive-)
endif()

target_link_libraries(${DESKTOP_TARGET}
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS ${DESKTOP_TARGET}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES "${APP_METADATA_JAR}" DESTINATION "runtime/android")

qt_generate_deploy_app_script(
    TARGET ${DESKTOP_TARGET}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
