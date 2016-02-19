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

#include <cypress/util/json.hpp>

namespace cypress {

TEST(json, join) {
	Json o1 {
		{"key1", {1, 2, 3, 4}},
		{"key2", {
			{"key21", 1},
			{"key22", 2},
			{"key23", 3},
		}},
		{"key3", "Hello World"}
	};

	Json o2 {
		{"key2", {
			{"key23", 4},
			{"key24", {
				{"key241", 1},
				{"key242", 2},
			}},
		}},
		{"key3", "Hello World2"}
	};

	EXPECT_TRUE(Json({
		{"key1", {1, 2, 3, 4}},
		{"key2", {
			{"key21", 1},
			{"key22", 2},
			{"key23", 4},
			{"key24", {
				{"key241", 1},
				{"key242", 2},
			}},
		}},
		{"key3", "Hello World2"}
	}) == join(o1, o2));
}
}
