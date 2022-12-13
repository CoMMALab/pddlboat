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

find_package(Z3 REQUIRED)
include_directories(SYSTEM ${Z3_CXX_INCLUDE_DIRS})
list(APPEND LIBRARIES ${Z3_LIBRARIES})

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

set(LIBRARY_NAME pddlboat-c++)
add_library(${LIBRARY_NAME} SHARED ${SOURCES})

set_target_properties(${LIBRARY_NAME} PROPERTIES VERSION 0.0.1)
target_link_libraries(${LIBRARY_NAME} PUBLIC ${LIBRARIES})

add_executable(pddlboat_building scripts/building.cpp)
add_executable(pddlboat_planning scripts/planning.cpp)
add_executable(pddlboat_anytime scripts/anytime.cpp)
add_executable(pddlboat_graphml scripts/graphml.cpp)
add_executable(pddlboat_blocking_test scripts/blocking_test.cpp)

target_link_libraries(pddlboat_building ${LIBRARY_NAME} ${LIBRARIES})
target_link_libraries(pddlboat_planning ${LIBRARY_NAME} ${LIBRARIES})
target_link_libraries(pddlboat_anytime ${LIBRARY_NAME} ${LIBRARIES})
target_link_libraries(pddlboat_graphml ${LIBRARY_NAME} ${LIBRARIES})
target_link_libraries(pddlboat_blocking_test ${LIBRARY_NAME} ${LIBRARIES})

find_package(pybind11 CONFIG QUIET)
if(pybind11_FOUND)
  find_package(Eigen3 REQUIRED)
  include_directories(BEFORE SYSTEM ${EIGEN3_INCLUDE_DIRS})
  pybind11_add_module(pddlboat py/pddlboat.cpp)
  target_link_libraries(pddlboat PRIVATE ${LIBRARY_NAME} ${EIGEN3_LIBRARIES})
endif()

feature_summary(DESCRIPTION INCLUDE_QUIET_PACKAGES WHAT ALL)

if(CATKIN_DEVEL_PREFIX)
  install(
    TARGETS ${LIBRARY_NAME}
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
  install(DIRECTORY include/${PROJECT_NAME}/
    DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
  install(DIRECTORY resources DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
endif(CATKIN_DEVEL_PREFIX)
