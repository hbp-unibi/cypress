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
cmake_minimum_required (VERSION 3.0)
project (cypress)

include_directories(
	PRIVATE
		${GTEST_INCLUDE_DIRS}
)

#
# Core library test
#

add_executable(test_cypress_core
	core/test_network
	core/test_connector
	core/test_transformation
)
add_dependencies(test_cypress_core googletest)
target_link_libraries(test_cypress_core
	cypress
	${GTEST_LIBRARIES}
)
add_test(test_cypress_core test_cypress_core)

#
# NEF tests
#

add_executable(test_cypress_nef
	nef/test_delta_sigma
	nef/test_tuning_curve
)
add_dependencies(test_cypress_nef googletest)
target_link_libraries(test_cypress_nef
	cypress
	${GTEST_LIBRARIES}
)
add_test(test_cypress_nef test_cypress_nef)

#
# Utility library tests
#

add_executable(test_cypress_util
	util/test_comperator
	util/test_filesystem
	util/test_json
	util/test_process
	util/test_range
	util/test_resource
	util/test_spiking_utils
)
add_dependencies(test_cypress_util googletest)
target_link_libraries(test_cypress_util
	cypress
	${GTEST_LIBRARIES}
)
add_test(test_cypress_util test_cypress_util)

#
# Backend tests
#

add_executable(test_pynn_backend
	backend/pynn/test_pynn
)
add_dependencies(test_pynn_backend googletest)
# Only link gtest to define own main function
target_link_libraries(test_pynn_backend
	cypress
	${BINARY_DIR}/googlemock/gtest/libgtest.a
)
add_test(test_pynn_backend test_pynn_backend)

add_executable(test_json_backend
	backend/test_toJson.cpp
)
add_dependencies(test_json_backend googletest)
# Only link gtest to define own main function
target_link_libraries(test_json_backend
	cypress
	${GTEST_LIBRARIES}
)
add_test(test_json_backend test_json_backend)

#
# Integration tests
#

add_executable(test_cypress_integration
	integration/test_v_init
)

add_dependencies(test_cypress_integration googletest)
target_link_libraries(test_cypress_integration
	cypress
	${GTEST_LIBRARIES}
)
add_test(test_cypress_integration test_cypress_integration)

#
# Transformation tests
#

add_executable(test_cypress_transformations
	transformations/test_spikey_if_cond_exp
)
add_dependencies(test_cypress_transformations googletest)
target_link_libraries(test_cypress_transformations
	cypress
	${GTEST_LIBRARIES}
)
add_test(test_cypress_transformations test_cypress_transformations)

