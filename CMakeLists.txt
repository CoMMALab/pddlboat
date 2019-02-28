cmake_minimum_required(VERSION 2.8.3)
project(pddlboat)
set(CMAKE_CXX_STANDARD 14)

set(LIBRARY_NAME ${PROJECT_NAME})

# set(CMAKE_MODULE_PATH
#   "${CMAKE_MODULE_PATH}"
#   "${CMAKE_ROOT_DIR}/cmake/Modules"
#   "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package(Boost REQUIRED)

list(APPEND INCULDES
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${Boost_INCLUDE_DIRS}
  )

list(APPEND SOURCES
  scripts/test.cpp)

include_directories(${INCLUDES})
link_directories(${Boost_LIBRARY_DIRS})

add_executable(pddlboat_test scripts/test.cpp)
