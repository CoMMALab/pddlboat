cmake_minimum_required(VERSION 2.8.3)
project(pddlboat)
set(LIBRARY_NAME ${PROJECT_NAME})

set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_ROOT_DIR}/cmake/Modules"
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(CompilerSettings)

find_package(Boost REQUIRED)
find_package(Z3 REQUIRED)

list(APPEND INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${Boost_INCLUDE_DIRS}
  ${Z3_INCLUDE_DIR}
  )

list(APPEND LIBRARIES
  ${Boost_LIBRARY_DIRS}
  ${Z3_LIBRARY}
  )

# list(APPEND SOURCES
#   scripts/test.cpp)

include_directories(${INCLUDES})

add_executable(pddlboat_test scripts/test.cpp)

add_executable(pddlboat_test_smt scripts/test_smt.cpp)
target_link_libraries(pddlboat_test_smt ${LIBRARIES})
