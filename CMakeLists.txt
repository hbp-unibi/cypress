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
cmake_minimum_required(VERSION 3.0)
project(cypress)

# Add the local "cmake" directory to the search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Automatically download and compile Google-Test and the JSON library
include(gtest_external)
include(json_external)
include(matplotlib_external)

SET(PYBIND11_PYTHON_VERSION "2.7")
add_subdirectory(external/pybind11)
find_package(PythonLibs 2.7 REQUIRED )

# Enable C++14 support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")

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

# Add configuration option for static linking 
set(STATIC_LINKING TRUE CACHE BOOL "True for static linking")


# Link statically
if(STATIC_LINKING)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(BUILD_SHARED_LIBRARIES OFF)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc  -Wl,--no-export-dynamic")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -Wl,--no-export-dynamic")
    else()
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc ")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
    endif()
endif()


include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}/include
	PRIVATE
		${CMAKE_BINARY_DIR}
    ${PYTHON_NUMPY_INCLUDE_DIR}
)


# Add configuration option for setting the width of the floating point values
set(CYPRESS_REAL_WIDTH 8 CACHE STRING "Width of the floating point values in bytes (one of 4, 8, 10, 16)")

configure_file(cypress/config.h.in cypress/config.h)

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
	cypress/backend/nest/nest
	cypress/backend/nest/sli
	cypress/backend/nmpi/nmpi
	cypress/backend/power/netio4
	cypress/backend/power/power
	cypress/backend/pynn/pynn
	cypress/backend/pynn/pynn2
	cypress/backend/pynn/slurm
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
	cypress/core/spike_time_generators
	cypress/core/transformation
	cypress/core/transformation_util
	cypress/core/types
	cypress/nef/delta_sigma
	cypress/nef/tuning_curve
	cypress/transformations/registry
	cypress/transformations/spike_sources
	cypress/transformations/spikey_if_cond_exp
	cypress/util/comperator
	cypress/util/filesystem
	cypress/util/json
	cypress/util/logger
	cypress/util/matplotlib_cpp
	cypress/util/matrix
	cypress/util/optional
	cypress/util/process
	cypress/util/range
	cypress/util/resource
	cypress/util/terminal
)


if(STATIC_LINKING)
	set_target_properties(cypress PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
	target_link_libraries(cypress
            pybind11::embed
            -pthread
            #-Wl,--whole-archive -lpthread -Wl,--no-whole-archive
            )
else()
	target_link_libraries(cypress pthread pybind11::embed)
endif()

# Installation configuration
install(TARGETS cypress ARCHIVE DESTINATION lib)
install(
	DIRECTORY ${CMAKE_SOURCE_DIR}/cypress
	DESTINATION include
	FILES_MATCHING PATTERN "*.hpp"
)
install(
	FILES ${CMAKE_BINARY_DIR}/cypress/config.h
	DESTINATION include/cypress
)
install(
	DIRECTORY ${CMAKE_BINARY_DIR}/include/cypress
	DESTINATION include
	FILES_MATCHING PATTERN "*.hpp"
)
install(
	FILES ${CMAKE_SOURCE_DIR}/cmake/FindCypress.cmake
	DESTINATION share/cmake/Cypress/
	RENAME CypressConfig.cmake
)

# Submodules
enable_testing()
add_subdirectory(examples)
add_subdirectory(test)

