file(GLOB_RECURSE TEST_SOURCES
  *.cpp
)
message("BUILDING TEST FILES: ${TEST_SOURCES}")
add_executable(NTemplate_tests ${TEST_SOURCES})
target_include_directories(NTemplate_tests PRIVATE
  include
  ${CMAKE_SOURCE_DIR}/vendor/NTest/include
  ${CMAKE_SOURCE_DIR}/vendor/NTest/vendor/NCLI/include  
)
# If NTemplate is library:
# target_link_libraries(NTemplate_tests PRIVATE NTemplate)
add_test(NAME NTemplate_tests COMMAND NTemplate_tests)