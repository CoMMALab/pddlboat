cmake_minimum_required(VERSION 2.8.3)

cmake_policy(SET CMP0017 NEW)
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.0.0)
  cmake_policy(SET CMP0042 NEW)
endif()
project(pddlboat CXX)

set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_ROOT_DIR}/cmake/Modules"
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

include(CompilerSettings)
include(FeatureSummary)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)


SET (BOOST_ROOT "/usr/local/")
SET (BOOST_MIN_VERSION "1.70.0")
set (Boost_NO_BOOST_CMAKE ON)
FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "Fatal error: Boost (version >= 1.70) required.")
else()
  message(STATUS "Setting up BOOST")
  message(STATUS " Includes - ${Boost_INCLUDE_DIRS}")
  message(STATUS " Library  - ${Boost_LIBRARY_DIRS}")
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif (NOT Boost_FOUND)

find_package(Boost 1.70 REQUIRED COMPONENTS system filesystem timer)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Boost_LIBRARIES})

message([warning] "boost info:")
message([warning] ${Boost_FOUND})
message([warning] ${Boost_VERSION})
message([warning] ${Boost_INCLUDE_DIRS})
message([warning] ${Boost_LIBRARIES})
message([warning] "end boost info")



find_package(Z3 REQUIRED)
include_directories(${Z3_INCLUDE_DIR})
list(APPEND LIBRARIES ${Z3_LIBRARY})

list(APPEND SOURCES
    src/utility.cpp
    src/expressions.cpp
    src/domain.cpp
    src/keywords.cpp
    src/ast.cpp
    src/parser.cpp
    )

add_definitions(-DPDDLBOAT_RESOURCE_DIRECTORY="${CMAKE_CURRENT_LIST_DIR}/resources")

add_library(pddlboat ${SOURCES})
set_target_properties(pddlboat PROPERTIES VERSION 0.0.1)
target_link_libraries(pddlboat ${LIBRARIES})

add_executable(pddlboat_test scripts/test.cpp)
# add_executable(pddlboat_parser scripts/parser_test.cpp)
# add_executable(pddlboat_test_smt scripts/test_smt.cpp)
# add_executable(pddlboat_test_planning scripts/test_planning.cpp)

target_link_libraries(pddlboat_test pddlboat ${LIBRARIES})
# target_link_libraries(pddlboat_parser pddlboat ${LIBRARIES})
# target_link_libraries(pddlboat_test_smt pddlboat ${LIBRARIES})
# target_link_libraries(pddlboat_test_planning pddlboat ${LIBRARIES})

feature_summary(DESCRIPTION INCLUDE_QUIET_PACKAGES WHAT ALL)
