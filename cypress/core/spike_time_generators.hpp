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
 * @file spike_time_generators.hpp
 *
 * Contains some methods for the generation of spike times.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_CORE_SPIKE_TIME_GENERATORS
#define CYPRESS_CORE_SPIKE_TIME_GENERATORS

#include <cstddef>
#include <cypress/core/types.hpp>
#include <vector>

namespace cypress {
namespace spikes {

std::vector<Real> poisson(Real t_start, Real t_end, Real rate,
                          std::size_t seed = 0.0);

std::vector<Real> constant_interval(Real t_start, Real t_end, Real interval,
                                    Real sigma = 0.0, std::size_t seed = 0);

std::vector<Real> constant_frequency(Real t_start, Real t_end, Real frequency,
                                     Real sigma = 0.0, std::size_t seed = 0);
}  // namespace spikes
}  // namespace cypress

#endif /* CYPRESS_CORE_SPIKE_TIME_GENERATORS */
