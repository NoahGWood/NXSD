file(GLOB_RECURSE TEST_SOURCES
  *.cpp
)
message("BUILDING TEST FILES: ${TEST_SOURCES}")
add_executable(nxsd_tests ${TEST_SOURCES})
target_include_directories(nxsd_tests PRIVATE
  include
  ${CMAKE_SOURCE_DIR}/vendor/NTest/include
  ${CMAKE_SOURCE_DIR}/vendor/NTest/vendor/NCLI/include  
)
target_compile_definitions(nxsd_tests PRIVATE NXSD_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(nxsd_tests PRIVATE nxsd tinyxml2)
add_test(NAME nxsd_tests COMMAND nxsd_tests)