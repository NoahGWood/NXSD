add_executable(minimal_app main.cpp)
# target_link_libraries(minimal_app PRIVATE NTemplate) # If NTemplate is a library

if(MINGW)
  target_link_options(minimal_app PRIVATE -mwindows)
endif()
