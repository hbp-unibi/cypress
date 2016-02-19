# CMake version and project metadata
cmake_minimum_required (VERSION 3.3)
project (cypress)

set(GTEST_INCLUDE_DIRS ${GTEST_SOURCE_DIR}/include ${GTEST_SOURCE_DIR})
set(GTEST_LIBRARIES gtest gtest_main)
set(GTEST ON)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test_cypress
	backend/binnf/test_binnf
	backend/pynn/test_pynn
	core/test_network
	util/test_comperator
	util/test_filesystem
	util/test_process
	util/test_resource
)

target_link_libraries(test_cypress
	cypress
	${GTEST_LIBRARIES}
)

add_test(test_cypress test_cypress)

