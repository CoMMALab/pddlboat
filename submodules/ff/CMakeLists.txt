project(fast-forward)
# NOTE: -fcommon is used here because the FF code does some questionable things with globals and the
# default behavior was changed to -fno-common in GCC 10
# (https://gcc.gnu.org/gcc-10/porting_to.html). This is a hacky workaround, but the FF code isn't
# ours to maintain and this is the simplest way to make it build
set(CMAKE_C_FLAGS "-Wall -g -std=gnu99 -O6 -fcommon")

find_package(BISON)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# NOTE: Because the original FF parser uses a cursed build step of #include-ing the generated lexer,
# and because this doesn't seem to play nice with CMake's Bison/Flex utilities, we've hacked this up
# a bit to manually include the generated C in the relevant .y files...
bison_target(ff-fct-parser
  scan-fct_pddl.y
  ${CMAKE_CURRENT_BINARY_DIR}/scan-fct_pddl.tab.c
  COMPILE_FLAGS "-pfct_pddl -bscan-fct_pddl")

bison_target(ff-ops-parser
  scan-ops_pddl.y
  ${CMAKE_CURRENT_BINARY_DIR}/scan-ops_pddl.tab.c
  COMPILE_FLAGS "-pops_pddl -bscan-ops_pddl")

list(
  APPEND
  PARSER_SOURCES
  ${BISON_ff-fct-parser_OUTPUT_SOURCE}
  ${BISON_ff-ops-parser_OUTPUT_SOURCE})

list(
  APPEND
  SOURCES
  main.c
  memory.c
  output.c
  parse.c
  inst_pre.c
  inst_easy.c
  inst_hard.c
  inst_final.c
  orderings.c
  relax.c
  search.c)

add_executable(ff ${SOURCES} ${PARSER_SOURCES})
target_link_libraries(ff m)
add_dependencies(ff ff-fct-lexer ff-ops-lexer)
