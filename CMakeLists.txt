cmake_minimum_required(VERSION 3.12)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_policy(SET CMP0017 NEW)
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.0.0)
  cmake_policy(SET CMP0042 NEW)
endif()

project(pddlboat
  LANGUAGES CXX
)

option(PDDLBOAT_BUILD_SCRIPTS "Build scripts" ON)
option(PDDLBOAT_BUILD_PYBIND "Build Python bindings" ON)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output." ON)
if(FORCE_COLORED_OUTPUT)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options(-fdiagnostics-color=always)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options(-fcolor-diagnostics)
    endif()
endif()

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


list(APPEND PDDLBOAT_SOURCES
  src/translator.cpp
  src/utility.cpp
  src/expressions.cpp
  src/domain.cpp
  src/keywords.cpp
  src/ast.cpp
  src/parser.cpp
  src/planner.cpp
  src/dependency.cpp
  src/z3planner.cpp
  src/madagascarplanner.cpp
  src/astarplanner.cpp)

add_definitions(-w)

## Fast-forward
# add_compile_definitions(PDDLBOAT_FASTFORWARD_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/ff:${CMAKE_INSTALL_PREFIX}/share/pddlboat/ff")
# list(APPEND PDDLBOAT_SOURCES src/ffplanner.cpp)
# add_subdirectory(submodules/ff)

## Fast-downward
add_compile_definitions(PDDLBOAT_FASTDOWNWARD_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/downward/src/bin:${CMAKE_INSTALL_PREFIX}/share/pddlboat/fd")
list(APPEND PDDLBOAT_SOURCES src/fdplanner.cpp)
add_subdirectory(submodules/downward/src)

## Sym-k
add_compile_definitions(PDDLBOAT_SYMK_LOCATION="${CMAKE_CURRENT_BINARY_DIR}/submodules/symk/src/bin:${CMAKE_INSTALL_PREFIX}/share/pddlboat/symk")
list(APPEND PDDLBOAT_SOURCES src/symkplanner.cpp)
add_subdirectory(submodules/symk/src)

add_compile_definitions(PDDLBOAT_RESOURCE_DIRECTORY="${CMAKE_CURRENT_LIST_DIR}/resources:${CMAKE_INSTALL_PREFIX}/share/pddlboat/resources")

add_library(pddlboat-cpp SHARED ${PDDLBOAT_SOURCES})

set_target_properties(pddlboat-cpp PROPERTIES VERSION 0.1.0)
target_link_libraries(pddlboat-cpp PUBLIC ${LIBRARIES})

if(PDDLBOAT_BUILD_SCRIPTS)
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
endif()

if(PDDLBOAT_BUILD_PYBIND)
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

else()
  install(TARGETS pddlboat-cpp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
  install(DIRECTORY include DESTINATION include)
  install(DIRECTORY resources/ DESTINATION share/pddlboat/resources/)

  ## FF executable
  install(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/submodules/ff/ff
    DESTINATION share/pddlboat/ff)

  ## FD executable
  install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/submodules/downward/src/bin/
    DESTINATION share/pddlboat/fd
    FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ
  )

  ## Sym-k executable
  install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/submodules/symk/src/bin/
    DESTINATION share/pddlboat/symk
    FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ
  )
endif()
