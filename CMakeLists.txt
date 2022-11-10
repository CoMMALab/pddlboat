cmake_minimum_required(VERSION 2.8.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_policy(SET CMP0017 NEW)
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.0.0)
  cmake_policy(SET CMP0042 NEW)
endif()
project(pddlboat CXX)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_ROOT_DIR}/cmake/Modules"
                      "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

include(CompilerSettings)
include(FeatureSummary)

message(STATUS "Building ${CMAKE_BUILD_TYPE}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)


set(BOOST_MIN_VERSION "1.65.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED
             COMPONENTS system filesystem timer program_options regex)
if(CATKIN_DEVEL_PREFIX)
  find_package(catkin REQUIRED)
  catkin_package(LIBRARIES pddlboat CATKIN_DEPENDS DEPENDS INCLUDE_DIRS
                 ${CMAKE_CURRENT_LIST_DIR}/include)
endif(CATKIN_DEVEL_PREFIX)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Boost_LIBRARIES})

add_subdirectory(submodules)
add_definitions(
  -DPDDLBOAT_FASTFORWARD_LOCATION="${CMAKE_CURRENT_SOURCE_DIR}/submodules/ff"
)
add_definitions(
  -DPDDLBOAT_FASTDOWNWARD_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/downward/src/bin"
)
add_definitions(
  -DPDDLBOAT_SYMK_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/symk/src/bin"
)

file(WRITE ${CMAKE_BINARY_DIR}/submodules/z3/Z3Targets.cmake "")
find_package(Z3 PATHS ${Z3_BINARY_DIR} NO_DEFAULT_PATH)
include_directories(BEFORE SYSTEM ${Z3_CXX_INCLUDE_DIRS})
list(APPEND LIBRARIES libz3)

list(
  APPEND
  SOURCES
  src/translator.cpp
  src/utility.cpp
  src/expressions.cpp
  src/domain.cpp
  src/keywords.cpp
  src/ast.cpp
  src/parser.cpp
  src/planner.cpp
  src/dependency.cpp
  src/ffplanner.cpp
  src/fdplanner.cpp
  src/symkplanner.cpp
  src/z3planner.cpp
  src/astarplanner.cpp)

add_definitions(
  -DPDDLBOAT_RESOURCE_DIRECTORY="${CMAKE_CURRENT_LIST_DIR}/resources")

add_library(pddlboat ${SOURCES})
add_dependencies(pddlboat libz3)

set_target_properties(pddlboat PROPERTIES VERSION 0.0.1)
target_link_libraries(pddlboat ${LIBRARIES})

add_executable(pddlboat_building scripts/test_building.cpp)
add_executable(pddlboat_parsing scripts/test_parsing.cpp)
add_executable(pddlboat_smt scripts/test_smt.cpp)
add_executable(pddlboat_planning scripts/test_planning.cpp)
add_executable(pddlboat_anytime scripts/test_anytime.cpp)
add_executable(pddlboat_blocking scripts/test_blocking.cpp)
add_executable(pddlboat_state_action_blocking
               scripts/state_action_blocker_test.cpp)
add_executable(pddlboat_graphml scripts/graphml.cpp)

target_link_libraries(pddlboat_building pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_parsing pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_smt pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_planning pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_anytime pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_blocking pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_state_action_blocking pddlboat ${LIBRARIES})
target_link_libraries(pddlboat_graphml pddlboat ${LIBRARIES})

feature_summary(DESCRIPTION INCLUDE_QUIET_PACKAGES WHAT ALL)

if(CATKIN_DEVEL_PREFIX)
  install(
    TARGETS pddlboat
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

  install(DIRECTORY include/${PROJECT_NAME}/
          DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
  install(DIRECTORY resources DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

  install(
    DIRECTORY submodules/z3/
    DESTINATION ${CATKIN_GLOBAL_INCLUDE_DESTINATION}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.cmake")

endif(CATKIN_DEVEL_PREFIX)
