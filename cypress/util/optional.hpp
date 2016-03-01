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

#ifndef CYPRESS_UTIL_OPTIONAL
#define CYPRESS_UTIL_OPTIONAL

#include <limits>

namespace cypress {

template<typename T>
struct NumericPolicy {
	static constexpr T empty() {return std::numeric_limits<T>::max();}
	static constexpr bool is_empty(const T &v) {return v == empty();}
};

template<typename T, typename Policy = NumericPolicy<T>>
class Optional {
private:
	T m_value;

public:
	constexpr Optional() : m_value(Policy::empty()) {}
	constexpr Optional(T v) : m_value(v) {}

	constexpr bool valid() const {return !Policy::is_empty(m_value); }
	constexpr T value() const {return m_value; }
};

}

#endif /* CYPRESS_UTIL_OPTIONAL */
