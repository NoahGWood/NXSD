add_executable(minimal_app main.cpp)
target_link_libraries(minimal_app PRIVATE nxsd)

if(MINGW)
  target_link_options(minimal_app PRIVATE -mwindows)
endif()
