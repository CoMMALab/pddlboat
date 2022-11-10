add_definitions(-w)
add_subdirectory(downward/src)
add_subdirectory(z3)
add_subdirectory(symk/src)
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ff/ff
  COMMAND make
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ff
)
add_custom_target(ff ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ff/ff)
