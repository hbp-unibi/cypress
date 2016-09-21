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

#pragma once

#ifndef CYPRESS_CORE_TYPES_HPP
#define CYPRESS_CORE_TYPES_HPP

#include <cstdint>

#include <cypress/config.h>

namespace cypress {

/**
 * Double type to be used in suffix declarations.
 */
using sx_double_t = long double;

/**
 * Integer type to be used in suffix declarations.
 */
using sx_int_t = unsigned long long int;

/**
 * The Real type is a floating point type. Its width can be chosen by the user
 * by setting the CYPRESS_REAL_WIDTH macro.
 */
#if CYPRESS_REAL_WIDTH == 4
using Real = float;
#elif CYPRESS_REAL_WIDTH == 8
using Real = double;
#elif CYPRESS_REAL_WIDTH == 10
using Real = long double;
#elif CYPRESS_REAL_WIDTH == 16
extern "C" {
#include <quadmath.h>
}
using Real = __float128;
#else
using Real = double;
#error Invalid value for CYPRESS_REAL_WIDTH supplied!
#endif

/**
 * Suffix declaration converting a double to the cinder Real type. Use this
 * suffix whenever you have a floating point constant in the code for which you
 * want to use the user-defined floating-point number width.
 */
static constexpr Real operator"" _R(sx_double_t x) { return Real(x); }

/**
 * Suffix declaration converting an integer to the cinder Real type. Use this
 * suffix whenever you have an integer constant in the code which you want to
 * convert to the user-defined floating-point number width.
 */
static constexpr Real operator"" _R(sx_int_t x) { return Real(x); }

using NeuronIndex = int32_t;
using PopulationIndex = int32_t;

}

#endif /* CYPRESS_CORE_TYPES_HPP */
