cmake_minimum_required(VERSION 2.8.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_policy(SET CMP0017 NEW)
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.0.0)
  cmake_policy(SET CMP0042 NEW)
endif()
project(pddlboat
  LANGUAGES CXX
)

set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_ROOT_DIR}/cmake/Modules"
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules"
)

include(CompilerSettings)
include(FeatureSummary)

message(STATUS "Building ${CMAKE_BUILD_TYPE}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(LIBRARY_NAME pddlboat-cpp)

set(BOOST_MIN_VERSION "1.65.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED
  COMPONENTS system filesystem timer program_options regex)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Boost_LIBRARIES})

find_package(Z3 REQUIRED)
include_directories(SYSTEM ${Z3_CXX_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Z3_LIBRARIES})

list(APPEND LIBRARIES stdc++fs)

if(CATKIN_DEVEL_PREFIX)
  find_package(catkin REQUIRED)
  catkin_package(
	  LIBRARIES ${LIBRARY_NAME}
	  CATKIN_DEPENDS
	  DEPENDS
	  Boost
	  INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include)
endif(CATKIN_DEVEL_PREFIX)

add_subdirectory(submodules)
add_definitions(-DPDDLBOAT_FASTFORWARD_LOCATION="${CMAKE_CURRENT_SOURCE_DIR}/submodules/ff")
add_definitions(-DPDDLBOAT_FASTDOWNWARD_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/downward/src/bin")
add_definitions(-DPDDLBOAT_SYMK_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/symk/src/bin")

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

add_definitions(-DPDDLBOAT_RESOURCE_DIRECTORY="${CMAKE_CURRENT_LIST_DIR}/resources")

add_library(pddlboat-cpp SHARED ${SOURCES})

set_target_properties(pddlboat-cpp PROPERTIES VERSION 0.0.1)
target_link_libraries(pddlboat-cpp PUBLIC ${LIBRARIES})

add_executable(pddlboat_building scripts/building.cpp)
add_executable(pddlboat_planning scripts/planning.cpp)
add_executable(pddlboat_anytime scripts/anytime.cpp)
add_executable(pddlboat_graphml scripts/graphml.cpp)
add_executable(pddlboat_blocking_test scripts/blocking_test.cpp)

target_link_libraries(pddlboat_building pddlboat-cpp ${LIBRARIES})
target_link_libraries(pddlboat_planning pddlboat-cpp ${LIBRARIES})
target_link_libraries(pddlboat_anytime pddlboat-cpp ${LIBRARIES})
target_link_libraries(pddlboat_graphml pddlboat-cpp ${LIBRARIES})
target_link_libraries(pddlboat_blocking_test pddlboat-cpp ${LIBRARIES})

find_package(pybind11 CONFIG QUIET)
if(pybind11_FOUND)
  find_package(Eigen3 REQUIRED)
  include_directories(BEFORE SYSTEM ${EIGEN3_INCLUDE_DIRS})
  pybind11_add_module(pddlboat py/pddlboat.cpp)
  target_link_libraries(pddlboat PRIVATE pddlboat-cpp ${EIGEN3_LIBRARIES})

  add_custom_command(
    TARGET pddlboat-cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:pddlboat-cpp>
    ${CMAKE_CURRENT_SOURCE_DIR}/py/pddlboat/pddlboat-cpp.so)

  add_custom_command(
    TARGET pddlboat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:pddlboat>
    ${CMAKE_CURRENT_SOURCE_DIR}/py/pddlboat/pddlboat.so)
endif()

feature_summary(DESCRIPTION INCLUDE_QUIET_PACKAGES WHAT ALL)

if(CATKIN_DEVEL_PREFIX)
  install(
    TARGETS pddlboat-cpp
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
  install(DIRECTORY include/${PROJECT_NAME}/
    DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
  install(DIRECTORY resources DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
endif(CATKIN_DEVEL_PREFIX)
