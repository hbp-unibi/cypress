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

# Download the JSON library
message(STATUS "Downloading https://github.com/nlohmann/json/releases/download/v2.1.0/json.hpp")
file(DOWNLOAD
	https://github.com/nlohmann/json/releases/download/v2.1.0/json.hpp
	${CMAKE_BINARY_DIR}/include/cypress/json.hpp
	EXPECTED_HASH SHA256=a571dee92515b685784fd527e38405cf3f5e13e96edbfe3f03d6df2e363a767b
)
