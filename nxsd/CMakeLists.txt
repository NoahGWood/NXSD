file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(nxsd
    ${SOURCES}
    # ${CMAKE_SOURCE_DIR}/vendor/
)
target_include_directories(nxsd PUBLIC
    include/
    # ${CMAKE_SOURCE_DIR}/vendor/
)
target_link_libraries(nxsd PRIVATE tinyxml2)

if(NXSD_BUILD_TESTS)
    add_subdirectory(tests)
endif()