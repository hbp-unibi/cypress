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
	URL            https://github.com/google/googletest/archive/release-1.8.0.tar.gz
	URL_HASH SHA512=1dbece324473e53a83a60601b02c92c089f5d314761351974e097b2cf4d24af4296f9eb8653b6b03b1e363d9c5f793897acae1f0c7ac40149216035c4d395d9d
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(googletest SOURCE_DIR BINARY_DIR)
set(GTEST_INCLUDE_DIRS ${SOURCE_DIR}/googletest/include/)
set(GTEST_LIBRARIES
	${BINARY_DIR}/googlemock/gtest/libgtest.a
	${BINARY_DIR}/googlemock/gtest/libgtest_main.a
)

