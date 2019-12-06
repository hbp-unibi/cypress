#  Cypress -- C++ Spiking Neural Network Simulation Framework
#  Copyright (C) 2018 Christoph Jenzen
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

# Download a matplotlib library
message(STATUS "Downloading https://raw.githubusercontent.com/costrau/matplotlib-cpp/master/matplotlibcpp.h")
file(DOWNLOAD
	https://raw.githubusercontent.com/costrau/matplotlib-cpp/master/matplotlibcpp.h
	${CMAKE_BINARY_DIR}/include/cypress/matplotlibcpp.hpp
)

execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c
            "from __future__ import print_function\nimport numpy; print(numpy.get_include(), end='')"
            OUTPUT_VARIABLE numpy_path)
            
find_path(PYTHON_NUMPY_INCLUDE_DIR numpy/arrayobject.h 
    HINTS "${numpy_path}" "${PYTHON_INCLUDE_PATH}" NO_DEFAULT_PATH)
