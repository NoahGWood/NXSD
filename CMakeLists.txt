cmake_minimum_required(VERSION 3.16)
project(NXSD LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
enable_language(C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NXSD_BUILD_TESTS ON)

add_subdirectory(vendor/tinyxml2) # NXSD depends on TinyXML
add_subdirectory(nxsd)
add_subdirectory(examples)