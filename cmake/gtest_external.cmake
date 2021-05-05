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
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 11.0)
ExternalProject_Add(googletest
	URL            https://github.com/google/googletest/archive/release-1.10.0.tar.gz
	URL_HASH SHA512=bd52abe938c3722adc2347afad52ea3a17ecc76730d8d16b065e165bc7477d762bce0997a427131866a89f1001e3f3315198204ffa5d643a9355f1f4d0d7b1a9
	INSTALL_COMMAND ""
	CMAKE_ARGS
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} 
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} 
    PATCH_COMMAND sed -i "s/int\ dummy$<SEMICOLON>/int\ dummy=0$<SEMICOLON>/g" ${CMAKE_CURRENT_BINARY_DIR}/googletest-prefix/src/googletest/googletest/src/gtest-death-test.cc
)
else()
ExternalProject_Add(googletest
	URL            https://github.com/google/googletest/archive/release-1.10.0.tar.gz
	URL_HASH SHA512=bd52abe938c3722adc2347afad52ea3a17ecc76730d8d16b065e165bc7477d762bce0997a427131866a89f1001e3f3315198204ffa5d643a9355f1f4d0d7b1a9
	INSTALL_COMMAND ""
	CMAKE_ARGS
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} 
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} 
)
endif()
ExternalProject_Get_Property(googletest SOURCE_DIR BINARY_DIR)
set(GTEST_INCLUDE_DIRS ${SOURCE_DIR}/googletest/include/)
set(GTEST_LIBRARIES
	${BINARY_DIR}/lib/libgtest.a
	${BINARY_DIR}/lib/libgtest_main.a
)
set(GTEST_LIBRARY_NO_MAIN
	${BINARY_DIR}/lib/libgtest.a
)
