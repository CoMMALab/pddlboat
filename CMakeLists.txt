cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0017 NEW)
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.0.0)
  cmake_policy(SET CMP0042 NEW)
endif()
project(pddlboat CXX)

# set the default build type
if (NOT CMAKE_BUILD_TYPE)
  # By default, use Release mode
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build" FORCE)

  # On 32bit architectures, use RelWithDebInfo
  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Type of build" FORCE)
  endif()
endif()

set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_ROOT_DIR}/cmake/Modules"
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(CompilerSettings)

message(STATUS "Building ${CMAKE_BUILD_TYPE}")

###
### Build universal-pddl-parser
###

file(GLOB PDDL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/universal-pddl-parser/parser/*.cpp)
add_library(upp ${PDDL_SOURCES})

###
### Dependencies
###

list(APPEND INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/universal-pddl-parser
  )
include_directories(${INCLUDES})

list(APPEND LIBRARIES
  upp
  )

###
### Build pddlboat library
###

# list(APPEND PDDLBOAT_SOURCES
#   )

# add_library(pddlboat ${PDDLBOAT_SOURCES})
# set_target_properties(pddlboat PROPERTIES VERSION 0.0.0)
# target_link_libraries(pddlboat ${LIBRARIES})

###
### Scripts
###

add_executable(demo_parser ${CMAKE_CURRENT_SOURCE_DIR}/universal-pddl-parser/examples/parser/parser.cpp)
target_link_libraries(demo_parser upp)
