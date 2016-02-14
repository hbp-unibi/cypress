# CMake version and project metadata
cmake_minimum_required (VERSION 3.3)
project (cypress)

# Enable C++14 support
set(CMAKE_CXX_STANDARD 14)

# Enable all warnings on MSVC and GCC/Clang/Intel
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR
		("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR
		("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Set a default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

enable_testing()
find_package(GTest)

include_directories(${CMAKE_SOURCE_DIR})

add_library(cypress
	cypress/core/binnf
	cypress/core/neurons
	cypress/core/network
	cypress/util/matrix
	cypress/util/process
	cypress/util/resource
)

target_link_libraries(cypress pthread)

#
# Example programs
#

add_executable(raw_binnf examples/raw_binnf)

target_link_libraries(raw_binnf cypress)

#
# Unit tests -- only enable if gtest is present
#
if(${GTEST_FOUND})
	include_directories(${GTEST_INCLUDE_DIR})

	add_executable(test_cypress
		test/core/test_binnf
		test/core/test_network
		test/util/test_process
		test/util/test_resource
	)

	target_link_libraries(test_cypress
		cypress
		${GTEST_BOTH_LIBRARIES}
	)

	add_test(test_cypress test_cypress)
endif()

