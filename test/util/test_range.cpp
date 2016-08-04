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

#include <vector>

#include "gtest/gtest.h"

#include <cypress/util/range.hpp>

namespace cypress {
namespace {
template <typename T>
std::vector<T> to_vector(Range<T> r)
{
	std::vector<T> res;
	for (T x : r) {
		res.push_back(x);
	}
	return res;
}
}

TEST(range, integer_range)
{
	{
		std::vector<size_t> res = to_vector(range(1));
		std::vector<size_t> expected = {0};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<size_t> res = to_vector(range(10));
		std::vector<size_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<size_t> res = to_vector(range(0));
		std::vector<size_t> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<size_t> res = to_vector(range(-1));
		std::vector<size_t> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<size_t> res = to_vector(range(-10));
		std::vector<size_t> expected = {};
		EXPECT_EQ(expected, res);
	}
}

TEST(range, dual_integer_range)
{
	{
		std::vector<ptrdiff_t> res = to_vector(range(0, 1));
		std::vector<ptrdiff_t> expected = {0};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(1, 1));
		std::vector<ptrdiff_t> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(0, 10));
		std::vector<ptrdiff_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(3, 10));
		std::vector<ptrdiff_t> expected = {3, 4, 5, 6, 7, 8, 9};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(0, 0));
		std::vector<ptrdiff_t> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(0, -1));
		std::vector<ptrdiff_t> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(-2, -1));
		std::vector<ptrdiff_t> expected = {-2};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(0, -10));
		std::vector<ptrdiff_t> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<ptrdiff_t> res = to_vector(range(-15, -10));
		std::vector<ptrdiff_t> expected = {-15, -14, -13, -12, -11};
		EXPECT_EQ(expected, res);
	}
}

TEST(range, float_range)
{
	{
		std::vector<float> res = to_vector(range(0.0f, 1.0f, 0.5f));
		std::vector<float> expected = {0.0f, 0.5f};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(range(0.0f, 1.0f, 0.25f));
		std::vector<float> expected = {0.0f, 0.25f, 0.5f, 0.75f};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(range(0.0f, 0.76f, 0.25f));
		std::vector<float> expected = {0.0f, 0.25f, 0.5f, 0.75f};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(range(0.0f, 0.76f, 0.25f));
		std::vector<float> expected = {0.0f, 0.25f, 0.5f, 0.75f};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(range(-5.0f, -3.0f, 0.25f));
		std::vector<float> expected = {-5.0f, -4.75f, -4.5f, -4.25f,
		                               -4.0f, -3.75f, -3.5f, -3.25f};
		EXPECT_EQ(expected, res);
	}
}

TEST(range, linspace)
{
	{
		std::vector<float> res = to_vector(linspace(0.0f, 10.0f, 11));
		std::vector<float> expected = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
		                               6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(linspace(0.0f, 10.0f, 0));
		std::vector<float> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(linspace(0.0f, 10.0f, -1));
		std::vector<float> expected = {};
		EXPECT_EQ(expected, res);
	}

	{
		std::vector<float> res = to_vector(linspace(0.0f, -10.0f, 11));
		std::vector<float> expected = {0.0f,  -1.0f, -2.0f, -3.0f, -4.0f, -5.0f,
		                               -6.0f, -7.0f, -8.0f, -9.0f, -10.0f};
		EXPECT_EQ(expected, res);
	}
}
}
