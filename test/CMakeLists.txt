# CMake version and project metadata
cmake_minimum_required (VERSION 3.3)
project (cypress)

include_directories(
	PRIVATE
		${GTEST_INCLUDE_DIRS}
)

add_executable(test_cypress
	backend/binnf/test_binnf
	backend/pynn/test_pynn
	core/test_network
	core/test_connector
	util/test_comperator
	util/test_filesystem
	util/test_json
	util/test_process
	util/test_resource
)

target_link_libraries(test_cypress
	cypress
	${GTEST_LIBRARIES}
)

add_test(test_cypress test_cypress)

