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

# Link statically
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBRARIES OFF)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

# Include external dependencies from the lib folder
add_subdirectory(lib/googletest)

include_directories(
	${CMAKE_SOURCE_DIR}
	PRIVATE
		${CMAKE_BINARY_DIR}
		lib/json/src
)

# Compile the Python code into a C++ header
add_custom_command(
	OUTPUT
		"${CMAKE_BINARY_DIR}/resource/backend/pynn/pynn_interface.hpp"
		"${CMAKE_BINARY_DIR}/resource/backend/pynn/pynn_binnf_loopback.hpp"
		"${CMAKE_BINARY_DIR}/resource/backend/nmpi/nmpi_broker.hpp"
	COMMAND
		"${CMAKE_SOURCE_DIR}/resource/build_resources.sh" "${CMAKE_BINARY_DIR}/resource/backend"
	DEPENDS
		"${CMAKE_SOURCE_DIR}/resource/build_resources.sh"
		"${CMAKE_SOURCE_DIR}/resource/backend/pynn/binnf.py"
		"${CMAKE_SOURCE_DIR}/resource/backend/pynn/cli.py"
		"${CMAKE_SOURCE_DIR}/resource/backend/pynn/constants.py"
		"${CMAKE_SOURCE_DIR}/resource/backend/pynn/cypress.py"
		"${CMAKE_SOURCE_DIR}/resource/backend/pynn/loopback.py"
		"${CMAKE_SOURCE_DIR}/resource/backend/nmpi/broker.py"
)

# Compile the library itself
add_library(cypress
	"${CMAKE_BINARY_DIR}/resource/backend/pynn/pynn_interface.hpp"
	"${CMAKE_BINARY_DIR}/resource/backend/pynn/pynn_binnf_loopback.hpp"
	"${CMAKE_BINARY_DIR}/resource/backend/nmpi/nmpi_broker.hpp"
	cypress/backend/resources
	cypress/backend/binnf/binnf
	cypress/backend/binnf/marshaller
	cypress/backend/nmpi/nmpi
	cypress/backend/pynn/pynn
	cypress/core/backend
	cypress/core/connector
	cypress/core/neurons
	cypress/core/network
	cypress/core/types
	cypress/util/clone_ptr
	cypress/util/comperator
	cypress/util/filesystem
	cypress/util/json
	cypress/util/matrix
	cypress/util/process
	cypress/util/resource
)

target_link_libraries(cypress pthread)

enable_testing()

add_subdirectory(examples)
add_subdirectory(test)

