include("E:/Project/AI_Mobile_Test_Studio_Codex/build/Desktop_Qt_6_8_3_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/AI_Mobile_Test_Studio_Codex-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE E:/Project/AI_Mobile_Test_Studio_Codex/build/Desktop_Qt_6_8_3_MinGW_64_bit-Debug/AI_Mobile_Test_Studio_Codex.exe
    GENERATE_QT_CONF
)
