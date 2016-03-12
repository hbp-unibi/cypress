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

