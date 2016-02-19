# CMake version and project metadata
cmake_minimum_required (VERSION 3.3)
project (cypress)

add_executable(raw_binnf raw_binnf)
target_link_libraries(raw_binnf cypress)

add_executable(simple simple)
target_link_libraries(simple cypress)

