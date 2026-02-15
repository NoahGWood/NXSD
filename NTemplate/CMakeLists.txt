file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(NTemplate
    ${SOURCES}
    # ${CMAKE_SOURCE_DIR}/vendor/
)
target_include_directories(NTemplate PUBLIC
    include/
    # ${CMAKE_SOURCE_DIR}/vendor/
)
# target_link_libraries(NTemplate PRIVATE)

if(NTemplate_BUILD_TESTS)
    add_subdirectory(tests)
endif()