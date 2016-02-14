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

#pragma once

#ifndef CPPNAM_UTIL_COMPARISON_HPP
#define CPPNAM_UTIL_COMPARISON_HPP

#include <cstdint>

namespace nam {
/**
 * The Comperator class can be used to assemble comperator operators, such as
 * equals, smaller or larger than.
 *
 * @tparam T is the type for which the comperator is being instantiated.
 */
template <typename T>
struct Comperator {
	/**
	 * Functor which can be indefinitely chained by the user to build a
	 * comperator.
	 */
	template <int Threshold, typename F>
	struct ComperatorFunctor {
		/**
		 * Reference to the underlying comperator function.
		 */
		const F &f;

		/**
		 * Result of the previous evaluation of the comperator.
		 */
		const int res;

		/**
		 * Constructor of the ComperatorFunctor.
		 *
		 * @param f is the comperator function underlying the functor.
		 * @param res is the result of the previous comperator evaluation.
		 */
		ComperatorFunctor(const F &f, int res = 0) : f(f), res(res) {}

		/**
		 * Compares the two given values t1 and t2 and returns a new functor.
		 *
		 * @param t1 is the first value that should be compared.
		 * @param t2 is the second value that should be compared.
		 * @return a new and updated ComperatorFunctor object.
		 */
		ComperatorFunctor<Threshold, F> operator()(const T &t1,
		                                           const T &t2) const
		{
			if (res == 0) {
				return make_comperator<Threshold, F>(f, f(t1, t2));
			}
			return make_comperator<Threshold, F>(f, res);
		}

		/**
		 * Returns the final result of the comparison.
		 */
		bool operator()() const { return res >= Threshold; }
	};

	template <int Threshold, typename F>
	static ComperatorFunctor<Threshold, F> make_comperator(const F &f,
	                                                       int res = 0)
	{
		return ComperatorFunctor<Threshold, F>(f, res);
	}

	static auto smaller(const T &t1, const T &t2)
	{
		return make_comperator<1>([](const T &t1, const T &t2) {
			return (t1 < t2) ? 1 : ((t1 == t2) ? 0 : -1);
		})(t1, t2);
	}

	static auto smaller_equals(const T &t1, const T &t2)
	{
		return make_comperator<0>([](const T &t1, const T &t2) {
			return (t1 < t2) ? 1 : ((t1 == t2) ? 0 : -1);
		})(t1, t2);
	}

	static auto larger(const T &t1, const T &t2)
	{
		return make_comperator<1>([](const T &t1, const T &t2) {
			return (t1 > t2) ? 1 : ((t1 == t2) ? 0 : -1);
		})(t1, t2);
	}

	static auto larger_equals(const T &t1, const T &t2)
	{
		return make_comperator<0>([](const T &t1, const T &t2) {
			return (t1 > t2) ? 1 : ((t1 == t2) ? 0 : -1);
		})(t1, t2);
	}

	static auto equals(const T &t1, const T &t2)
	{
		return make_comperator<0>([](const T &t1, const T &t2) {
			return (t1 == t2) ? 0 : -1;
		})(t1, t2);
	}

	static auto inequal(const T &t1, const T &t2)
	{
		return make_comperator<1>([](const T &t1, const T &t2) {
			return (t1 != t2) ? 1 : 0;
		})(t1, t2);
	}
};
}

#endif /* CPPNAM_UTIL_COMPARISON_HPP */