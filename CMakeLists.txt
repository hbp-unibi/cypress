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

set(BUILD_TEST_EXAMPLES TRUE CACHE BOOL "Set to true for building examples and tests")

# If Python Library is already found, continue, otherwise default to python3
if(Python_VERSION_MAJOR)
    find_package(PythonInterp Python_VERSION_MAJOR REQUIRED)
    find_package(PythonLibs Python_VERSION_MAJOR REQUIRED)
    SET(PYBIND11_PYTHON_VERSION Python_VERSION_MAJOR)
else()
    find_package(PythonInterp 3 REQUIRED)
    find_package(PythonLibs 3 REQUIRED)
    SET(PYBIND11_PYTHON_VERSION 3)
endif()

# Automatically download and compile Google-Test and the JSON library
include(gtest_external)
include(json_external)
include(matplotlib_external)

set(PYBIND11_INSTALL TRUE CACHE BOOL "pybind11 install" FORCE INTERNAL)
add_subdirectory(external/pybind11)

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
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()

# Set a default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Add configuration option for static linking 
set(STATIC_LINKING TRUE CACHE BOOL "True for static linking")


# Link statically
if(STATIC_LINKING)
    execute_process (
                COMMAND ${CMAKE_C_COMPILER} -print-file-name=libc.so.6
                OUTPUT_VARIABLE GLIBC
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/glibc_helper.sh ${GLIBC}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/include/
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${CMAKE_BINARY_DIR}/include/glibc.h")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_BINARY_DIR}/include/glibc.h")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(BUILD_SHARED_LIBRARIES OFF)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++  -Wl,--no-export-dynamic")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -Wl,--no-export-dynamic")
    else()
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
    endif()
endif()

include_directories(
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/include
        ${PYTHON_NUMPY_INCLUDE_DIR}
)



# Add configuration option for setting the width of the floating point values
set(CYPRESS_REAL_WIDTH 8 CACHE STRING "Width of the floating point values in bytes (one of 4, 8, 10, 16)")

configure_file(cypress/config.h.in include/cypress/config.h)

# Compile the Python code into a C++ header
add_custom_command(
	OUTPUT
		"${CMAKE_BINARY_DIR}/include/resource/backend/nmpi/nmpi_broker.hpp"
	COMMAND
		"${CMAKE_SOURCE_DIR}/resource/build_resources.sh" "${CMAKE_BINARY_DIR}/include/resource/backend"
	DEPENDS
		"${CMAKE_SOURCE_DIR}/resource/build_resources.sh"
		"${CMAKE_SOURCE_DIR}/resource/backend/nmpi/broker.py"
)

# Compile the library itself
add_library(cypress
	"${CMAKE_BINARY_DIR}/include/resource/backend/nmpi/nmpi_broker.hpp"
	cypress/backend/resources
	cypress/backend/brainscales/brainscales_lib
	cypress/backend/brainscales/slurm
	cypress/backend/genn/genn_lib
	cypress/backend/nest/nest
	cypress/backend/nest/sli
	cypress/backend/nmpi/nmpi
	cypress/backend/power/energenie
	cypress/backend/power/netio4
	cypress/backend/power/power
	cypress/backend/pynn/pynn
	cypress/backend/serialize/to_json
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
	cypress/core/synapses
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
	cypress/util/neuron_parameters
	cypress/util/optional
	cypress/util/process
	cypress/util/range
	cypress/util/resource
	cypress/util/spiking_utils
	cypress/util/terminal
	cypress/util/to_dot
)

set_target_properties(cypress PROPERTIES POSITION_INDEPENDENT_CODE ON)
message(STATUS "Downloading BrainScaleS library")
file(DOWNLOAD https://github.com/hbp-unibi/BS2Cypress/raw/master/bin/libBS2CYPRESS.so
	${CMAKE_BINARY_DIR}/libBS2CYPRESS.so
)
target_compile_definitions(cypress PRIVATE BS_LIBRARY_PATH="${CMAKE_BINARY_DIR}/libBS2CYPRESS.so")
target_compile_definitions(cypress PRIVATE BS_LIBRARY_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/lib/libBS2CYPRESS.so")

## Json parser
add_executable(cypress_from_json cypress/backend/serialize/json_exec)
add_dependencies(cypress_from_json cypress)
target_link_libraries(cypress_from_json cypress)
target_compile_definitions(cypress PRIVATE JSON_EXEC_PATH="${CMAKE_BINARY_DIR}/cypress_from_json")

## GeNN backend module
add_library(gennbck SHARED
	cypress/backend/genn/genn
	cypress/backend/genn/genn_models
)
include(genn_external)
add_dependencies(gennbck genn_ext)
target_compile_definitions(cypress PRIVATE GENN_LIBRARY_PATH="${CMAKE_BINARY_DIR}/libgennbck.so")
target_compile_definitions(cypress PRIVATE GENN_LIBRARY_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/lib/libgennbck.so")
target_link_libraries(gennbck PRIVATE
            ${GENN_LIBRARIES}
)
            

if(STATIC_LINKING)
	set_target_properties(cypress PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
	target_link_libraries(cypress PUBLIC
            pybind11::embed
            PRIVATE
            ${PYTHON_LIBRARY}
            dl
            -pthread
            )
else()
	target_link_libraries(cypress ${GENN_LIBRARIES} pthread pybind11::embed dl)
endif()

# Check if cypress is built as a subproject
if (BUILD_TEST_EXAMPLES)
    # Installation configuration
    install(TARGETS cypress ARCHIVE DESTINATION lib)
    install(TARGETS cypress_from_json RUNTIME DESTINATION bin)
    install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/cypress
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
    )
    install(
        FILES ${CMAKE_BINARY_DIR}/include/cypress/config.h
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
    install(FILES ${CMAKE_BINARY_DIR}/libBS2CYPRESS.so
        DESTINATION lib)
    
    install(FILES ${CMAKE_BINARY_DIR}/libBS2CYPRESS.so
        DESTINATION lib)

    # Submodules
    enable_testing()
    add_subdirectory(examples)
    add_subdirectory(test)
endif()

