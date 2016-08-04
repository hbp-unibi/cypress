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

/**
 * @file range.hpp
 *
 * Contains some helper functions for iterating over ranges -- similar to those
 * found in Octave or Python.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_UTIL_RANGE_HPP
#define CYPRESS_UTIL_RANGE_HPP

#include <cstddef>
#include <cmath>

namespace cypress {

/**
 * Base range class.
 */
template <typename T>
struct Range {
	T x0;
	T step;
	size_t n;

	struct Iterator {
		T x0;
		T step;
		size_t i;

		T operator*() const { return x0 + step * T(i); }
		T operator->() const { return x0 + step * T(i); }
		Iterator &operator++()
		{
			++i;
			return *this;
		}
		Iterator operator++(int)
		{
			Iterator tmp = *this;
			++i;
			return tmp;
		}

		bool operator==(Iterator &o) const { return i == o.i; }
		bool operator!=(Iterator &o) const { return i != o.i; }
		bool operator<(Iterator &o) const { return i < o.i; }
	};

	Iterator begin() const { return Iterator{x0, step, 0}; }
	Iterator end() const { return Iterator{x0, step, n}; }
};

constexpr Range<float> linspace(float x0, float x1, ptrdiff_t num)
{
	return Range<float>{x0, (x1 - x0) / (num - 1),
	                    (num > 0) ? size_t(num) : 0U};
}

constexpr Range<double> linspace(double x0, double x1, ptrdiff_t num)
{
	return Range<double>{x0, (x1 - x0) / (num - 1),
	                     (num > 0) ? size_t(num) : 0U};
}

constexpr Range<size_t> range(ptrdiff_t i)
{
	return Range<size_t>{0U, 1U, (i > 0) ? size_t(i) : 0U};
}

constexpr Range<ptrdiff_t> range(ptrdiff_t i0, ptrdiff_t i1)
{
	return Range<ptrdiff_t>{i0, 1, (i1 > i0) ? size_t(i1 - i0) : 0U};
}

constexpr Range<float> range(float x0, float x1, float step)
{
	return Range<float>{x0, step,
	                    (x1 > x0) ? size_t(std::ceil((x1 - x0) / step)) : 0U};
}

constexpr Range<double> range(double x0, double x1, double step)
{
	return Range<double>{x0, step,
	                     (x1 > x0) ? size_t(std::ceil((x1 - x0) / step)) : 0U};
}
}

#endif /* CYPRESS_UTIL_RANGE_HPP */
