/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2016  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "gtest/gtest.h"

#include <cypress/util/filesystem.hpp>

namespace cypress {
namespace filesystem {

TEST(filesystem, longest_common_path)
{
	EXPECT_EQ("/foo/bar/fuum",
	          longest_common_path(std::vector<std::string>({"/foo/bar/fuum"})));
	EXPECT_EQ("/foo/bar/fuum",
	          longest_common_path(std::vector<std::string>({"/foo/bar/fuum/"})));
	EXPECT_EQ("/foo/bar", longest_common_path(std::vector<std::string>(
	                          {"/foo/bar/fuum", "/foo/bar/fool"})));
	EXPECT_EQ("/foo", longest_common_path(std::vector<std::string>(
	                          {"/foo/bar/fuum", "/foo/bar/fool", "/foo/"})));
	EXPECT_EQ("/foo", longest_common_path(std::vector<std::string>(
	                          {"/foo/bar/fuum", "/foo/bar/fool", "/foo"})));
	EXPECT_EQ("/", longest_common_path(std::vector<std::string>(
	                          {"/foo/bar/fuum", "/foo/bar/fool", "/fo"})));
}

TEST(filesystem, canonicalise)
{
	EXPECT_TRUE(!canonicalise("test_cypress").empty());
	EXPECT_EQ('/', canonicalise("test_cypress")[0]);
}
}
}
