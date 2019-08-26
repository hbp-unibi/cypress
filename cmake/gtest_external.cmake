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

include(ExternalProject)

# Compile and include gtest -- set GTEST_CAN_STREAM_RESULTS_ to 0 to work around
# the confusing warning regarding getaddrinfo() being issued when statically
# linking
ExternalProject_Add(googletest
	URL            https://github.com/google/googletest/archive/release-1.8.1.tar.gz
	URL_HASH SHA512=e6283c667558e1fd6e49fa96e52af0e415a3c8037afe1d28b7ff1ec4c2ef8f49beb70a9327b7fc77eb4052a58c4ccad8b5260ec90e4bceeac7a46ff59c4369d7
	INSTALL_COMMAND ""
	CMAKE_ARGS
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} 
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} 
)
ExternalProject_Get_Property(googletest SOURCE_DIR BINARY_DIR)
set(GTEST_INCLUDE_DIRS ${SOURCE_DIR}/googletest/include/)
message(warning ${BINARY_DIR})
set(GTEST_LIBRARIES
	${BINARY_DIR}/googlemock/gtest/libgtest.a
	${BINARY_DIR}/googlemock/gtest/libgtest_main.a
)
set(GTEST_LIBRARY_NO_MAIN
	${BINARY_DIR}/googlemock/gtest/libgtest.a
)
message(warning ${GTEST_LIBRARIES})

