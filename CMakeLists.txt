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

find_package(Boost REQUIRED COMPONENTS system filesystem timer)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Boost_LIBRARIES})

find_package(Z3 REQUIRED)
include_directories(${Z3_INCLUDE_DIR})
list(APPEND LIBRARIES ${Z3_LIBRARY})

add_subdirectory(submodules)

list(APPEND SOURCES
    src/translator.cpp
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
add_executable(pddlboat_parser scripts/parser_test.cpp)
add_executable(pddlboat_load scripts/test_parsing.cpp)
# add_executable(pddlboat_test_smt scripts/test_smt.cpp)
# add_executable(pddlboat_test_planning scripts/test_planning.cpp)

target_link_libraries(pddlboat_test pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_parser pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_load pddlboat ${LIBRARIES})
# target_link_libraries(pddlboat_test_smt pddlboat ${LIBRARIES})
# target_link_libraries(pddlboat_test_planning pddlboat ${LIBRARIES})

feature_summary(DESCRIPTION INCLUDE_QUIET_PACKAGES WHAT ALL)
