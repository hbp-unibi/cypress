/*
 *  CppNAM -- C++ Neural Associative Memory Simulator
 *  Copyright (C) 2016  Christoph Jenzen, Andreas Stöckel
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

#include "gtest/gtest.h"

#include <cstdint>
#include <limits>

#include <cypress/util/comperator.hpp>

namespace cypress {

TEST(comperator, smaller) {
	EXPECT_EQ(1, Comperator<int>::smaller(0, 1).res);
	EXPECT_EQ(0, Comperator<int>::smaller(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0).res);

	EXPECT_EQ(1, Comperator<int>::smaller(0, 1)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::smaller(0, 1)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller(0, 1)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::smaller(0, 0)(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0)(0, 0).res);

	EXPECT_EQ(1, Comperator<int>::smaller(0, 1)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(0, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::smaller(0, 1)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller(0, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller(0, 1)(0, 0)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::smaller(0, 0)(0, 0)(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller(1, 0)(0, 0)(0, 0).res);

	EXPECT_TRUE(Comperator<int>::smaller(0, 1)());
	EXPECT_FALSE(Comperator<int>::smaller(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)());

	EXPECT_TRUE(Comperator<int>::smaller(0, 1)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::smaller(0, 1)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller(0, 1)(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)(0, 0)());

	EXPECT_TRUE(Comperator<int>::smaller(0, 1)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(0, 0)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::smaller(0, 1)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller(0, 0)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller(0, 1)(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(0, 0)(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller(1, 0)(0, 0)(0, 0)());
}

TEST(comperator, smaller_equals) {
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1).res);
	EXPECT_EQ(0, Comperator<int>::smaller_equals(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0).res);

	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::smaller_equals(0, 0)(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0)(0, 0).res);

	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(0, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::smaller_equals(0, 1)(0, 0)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::smaller_equals(0, 0)(0, 0)(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::smaller_equals(1, 0)(0, 0)(0, 0).res);

	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)());

	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)(0, 0)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)(0, 0)());

	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(0, 0)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 0)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 1)(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::smaller_equals(0, 0)(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::smaller_equals(1, 0)(0, 0)(0, 0)());
}

TEST(comperator, larger) {
	EXPECT_EQ(-1, Comperator<int>::larger(0, 1).res);
	EXPECT_EQ(0, Comperator<int>::larger(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0).res);

	EXPECT_EQ(-1, Comperator<int>::larger(0, 1)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::larger(0, 1)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger(0, 1)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::larger(0, 0)(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0)(0, 0).res);

	EXPECT_EQ(-1, Comperator<int>::larger(0, 1)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(0, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::larger(0, 1)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger(0, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger(0, 1)(0, 0)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::larger(0, 0)(0, 0)(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger(1, 0)(0, 0)(0, 0).res);

	EXPECT_FALSE(Comperator<int>::larger(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)());

	EXPECT_FALSE(Comperator<int>::larger(0, 1)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::larger(0, 1)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger(0, 1)(0, 0)());
	EXPECT_FALSE(Comperator<int>::larger(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)(0, 0)());

	EXPECT_FALSE(Comperator<int>::larger(0, 1)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger(0, 0)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::larger(0, 1)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger(0, 0)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger(0, 1)(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::larger(0, 0)(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger(1, 0)(0, 0)(0, 0)());
}

TEST(comperator, larger_equals) {
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1).res);
	EXPECT_EQ(0, Comperator<int>::larger_equals(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0).res);

	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::larger_equals(0, 0)(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0)(0, 0).res);

	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(0, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::larger_equals(0, 1)(0, 0)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::larger_equals(0, 0)(0, 0)(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::larger_equals(1, 0)(0, 0)(0, 0).res);

	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)());
	EXPECT_TRUE(Comperator<int>::larger_equals(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)());

	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger_equals(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)(0, 0)());

	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(0, 0)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger_equals(0, 0)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::larger_equals(0, 1)(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(0, 0)(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::larger_equals(1, 0)(0, 0)(0, 0)());
}

TEST(comperator, equals) {
	EXPECT_EQ(-1, Comperator<int>::equals(0, 1).res);
	EXPECT_EQ(0, Comperator<int>::equals(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0).res);

	EXPECT_EQ(-1, Comperator<int>::equals(0, 1)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 1)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 1)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::equals(0, 0)(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0)(0, 0).res);

	EXPECT_EQ(-1, Comperator<int>::equals(0, 1)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 1)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(-1, Comperator<int>::equals(0, 1)(0, 0)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::equals(0, 0)(0, 0)(0, 0).res);
	EXPECT_EQ(-1, Comperator<int>::equals(1, 0)(0, 0)(0, 0).res);

	EXPECT_FALSE(Comperator<int>::equals(0, 1)());
	EXPECT_TRUE(Comperator<int>::equals(0, 0)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)());

	EXPECT_FALSE(Comperator<int>::equals(0, 1)(1, 0)());
	EXPECT_FALSE(Comperator<int>::equals(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::equals(0, 1)(0, 1)());
	EXPECT_FALSE(Comperator<int>::equals(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::equals(0, 1)(0, 0)());
	EXPECT_TRUE(Comperator<int>::equals(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)(0, 0)());

	EXPECT_FALSE(Comperator<int>::equals(0, 1)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::equals(0, 0)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)(0, 0)(1, 0)());
	EXPECT_FALSE(Comperator<int>::equals(0, 1)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::equals(0, 0)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)(0, 0)(0, 1)());
	EXPECT_FALSE(Comperator<int>::equals(0, 1)(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::equals(0, 0)(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::equals(1, 0)(0, 0)(0, 0)());
}

TEST(comperator, inequal) {
	EXPECT_EQ(1, Comperator<int>::inequal(0, 1).res);
	EXPECT_EQ(0, Comperator<int>::inequal(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0).res);

	EXPECT_EQ(1, Comperator<int>::inequal(0, 1)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 1)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 1)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::inequal(0, 0)(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0)(0, 0).res);

	EXPECT_EQ(1, Comperator<int>::inequal(0, 1)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0)(0, 0)(1, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 1)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0)(0, 0)(0, 1).res);
	EXPECT_EQ(1, Comperator<int>::inequal(0, 1)(0, 0)(0, 0).res);
	EXPECT_EQ(0, Comperator<int>::inequal(0, 0)(0, 0)(0, 0).res);
	EXPECT_EQ(1, Comperator<int>::inequal(1, 0)(0, 0)(0, 0).res);

	EXPECT_TRUE(Comperator<int>::inequal(0, 1)());
	EXPECT_FALSE(Comperator<int>::inequal(0, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)());

	EXPECT_TRUE(Comperator<int>::inequal(0, 1)(1, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 1)(0, 1)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 1)(0, 0)());
	EXPECT_FALSE(Comperator<int>::inequal(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)(0, 0)());

	EXPECT_TRUE(Comperator<int>::inequal(0, 1)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 0)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)(0, 0)(1, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 1)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 0)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)(0, 0)(0, 1)());
	EXPECT_TRUE(Comperator<int>::inequal(0, 1)(0, 0)(0, 0)());
	EXPECT_FALSE(Comperator<int>::inequal(0, 0)(0, 0)(0, 0)());
	EXPECT_TRUE(Comperator<int>::inequal(1, 0)(0, 0)(0, 0)());
}
}
