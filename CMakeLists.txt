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

SET (BOOST_MIN_VERSION "1.65.0")
find_package(Boost ${BOOST_MIN_VERSION} QUIET)

SET (BOOST_MIN_VERSION "1.65.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system filesystem timer program_options regex)

if(NOT Boost_INCLUDE_DIRS)
SET (BOOST_ROOT "/usr/local/")
set (Boost_NO_BOOST_CMAKE ON)
FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system filesystem timer program_options regex)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "Fatal error: Boost (version >= 1.65) required.")
else()
  message(STATUS "Setting up BOOST")
  message(STATUS " Includes - ${Boost_INCLUDE_DIRS}")
  message(STATUS " Library  - ${Boost_LIBRARY_DIRS}")
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif (NOT Boost_FOUND)
endif(NOT Boost_INCLUDE_DIRS)

if (CATKIN_DEVEL_PREFIX)
find_package(catkin REQUIRED)
catkin_package(
  LIBRARIES pddlboat
  CATKIN_DEPENDS
  DEPENDS
  INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
  )
endif (CATKIN_DEVEL_PREFIX)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Boost_LIBRARIES})

find_package(Z3 REQUIRED)
include_directories(${Z3_INCLUDE_DIR})
list(APPEND LIBRARIES ${Z3_LIBRARY})

add_subdirectory(submodules)
add_definitions(-DPDDLBOAT_FASTDOWNWARD_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/fd/src/bin")

list(APPEND SOURCES
    src/translator.cpp
    src/utility.cpp
    src/expressions.cpp
    src/domain.cpp
    src/keywords.cpp
    src/ast.cpp
    src/parser.cpp
    src/planner.cpp
    src/fdplanner.cpp
    src/z3planner.cpp
    )

add_definitions(-DPDDLBOAT_RESOURCE_DIRECTORY="${CMAKE_CURRENT_LIST_DIR}/resources")

add_library(pddlboat ${SOURCES})
set_target_properties(pddlboat PROPERTIES VERSION 0.0.1)
target_link_libraries(pddlboat ${LIBRARIES})

add_executable(pddlboat_building scripts/test_building.cpp)
add_executable(pddlboat_parsing scripts/test_parsing.cpp)
add_executable(pddlboat_smt scripts/test_smt.cpp)
add_executable(pddlboat_planning scripts/test_planning.cpp)

target_link_libraries(pddlboat_building pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_parsing pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_smt pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_planning pddlboat ${LIBRARIES})

feature_summary(DESCRIPTION INCLUDE_QUIET_PACKAGES WHAT ALL)

if (CATKIN_DEVEL_PREFIX)
install(TARGETS pddlboat
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

install(DIRECTORY include/${PROJECT_NAME}/ DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
endif (CATKIN_DEVEL_PREFIX)
