cmake_minimum_required(VERSION 3.19)
project(AI_Mobile_Test_Studio_Codex LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

add_subdirectory(apps/desktop)
