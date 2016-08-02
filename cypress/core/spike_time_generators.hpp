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

#include <vector>

namespace cypress {
namespace spikes {

// TODO: Add support for seeds!

std::vector<float> poisson(float t_start, float t_end, float rate);

std::vector<float> constant_interval(float t_start, float t_end, float interval,
                                     float sigma = 0.0);

std::vector<float> constant_frequency(float t_start, float t_end,
                                      float frequency, float sigma = 0.0);
}
}

#endif /* CYPRESS_CORE_SPIKE_TIME_GENERATORS */
