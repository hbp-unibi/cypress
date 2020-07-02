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
message(STATUS "Download https://github.com/nlohmann/json/releases/download/v3.8.0/json.hpp")
file(DOWNLOAD
	https://github.com/nlohmann/json/releases/download/v3.8.0/json.hpp
	${CMAKE_BINARY_DIR}/include/cypress/json.hpp
	EXPECTED_HASH SHA256=be7bbfbc7c62c35aa6f27dfbdd950384f247dd9f2281b5930bdaa3a6ab95eef7
)
