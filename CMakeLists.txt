cmake_minimum_required(VERSION 3.16)
project(NTemplate LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
enable_language(C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NTemplate_BUILD_TESTS ON)

add_subdirectory(NTemplate)
add_subdirectory(examples)