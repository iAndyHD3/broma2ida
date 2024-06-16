cmake_minimum_required(VERSION 3.21)
include(CPM.cmake)

project(broma2ida LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(${PROJECT_NAME} src/main.cpp)

CPMAddPackage("gh:geode-sdk/broma#4974750")

target_link_libraries(${PROJECT_NAME} Broma)