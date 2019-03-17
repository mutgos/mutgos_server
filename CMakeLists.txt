cmake_minimum_required(VERSION 2.8)
project(MUTGOS_SERVER)

# For debugging the makefile output
# set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DBOOST_ALL_DYN_LINK)

# -fno-strict-aliasing is for AngelScript
set(CMAKE_CXX_FLAGS "-g -Wall -O0 -std=c++11 -ftemplate-backtrace-limit=0 -Wno-unused-function -fno-strict-aliasing")

include_directories(BEFORE SYSTEM /data/mutgos/cots/include)
include_directories(src)

link_directories(/data/mutgos/cots/lib)

add_subdirectory(third_party)
add_subdirectory(src)
