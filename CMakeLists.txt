#  Cypress -- C++ Spiking Neural Network Simulation Framework
#  Copyright (C) 2016  Andreas Stöckel
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# CMake version and project metadata
cmake_minimum_required(VERSION 3.3)
project(cypress)

# Add the local "cmake" directory to the search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Automatically download and compile Google-Test and the JSON library
include(gtest_external)
include(json_external)

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

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}/include
	PRIVATE
		${CMAKE_BINARY_DIR}
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
	cypress/backend/pynn/transformation
	cypress/core/backend
	cypress/core/connector
	cypress/core/data
	cypress/core/exceptions
	cypress/core/neurons
	cypress/core/neurons_base
	cypress/core/network
	cypress/core/network_base
	cypress/core/network_base_objects
	cypress/core/network_mixins
	cypress/core/types
	cypress/nef/spike_train
	cypress/util/comperator
	cypress/util/filesystem
	cypress/util/json
	cypress/util/matrix
	cypress/util/optional
	cypress/util/process
	cypress/util/resource
)

target_link_libraries(cypress pthread)

# Installation configuration
install(TARGETS cypress ARCHIVE DESTINATION lib)
install(
	DIRECTORY ${CMAKE_SOURCE_DIR}/cypress
	DESTINATION include
	FILES_MATCHING PATTERN "*.hpp"
)
install(
	DIRECTORY ${CMAKE_BINARY_DIR}/include/cypress
	DESTINATION include
	FILES_MATCHING PATTERN "*.hpp"
)

# Submodules
enable_testing()
add_subdirectory(examples)
add_subdirectory(test)

