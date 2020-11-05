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

#include <algorithm>
#include <cypress/core/spike_time_generators.hpp>
#include <cypress/util/rng.hpp>
#include <random>

namespace cypress {
namespace spikes {

std::vector<Real> poisson(Real t_start, Real t_end, Real rate)
{
    auto& re = RNG::instance().get();
	std::vector<Real> result;
	if (rate > 0.0) {
		std::exponential_distribution<Real> dist(rate / 1000.0);
		Real t = t_start;
		while (true) {
			t += dist(re);
			if (t < t_end) {
				result.emplace_back(t);
			}
			else {
				break;
			}
		}
	}
	return result;
}

std::vector<Real> constant_interval(Real t_start, Real t_end, Real interval,
                                    Real sigma)
{
    auto& re = RNG::instance().get();
	std::normal_distribution<Real> distribution(0.0, sigma);

	const size_t n_samples = (t_end - t_start) / interval;
	std::vector<Real> result(n_samples);
	for (size_t i = 0; i < n_samples; i++) {
		result[i] = t_start + interval * (i + 1) + distribution(re);
	}
	std::sort(result.begin(), result.end());
	return result;
}

std::vector<Real> constant_frequency(Real t_start, Real t_end, Real frequency,
                                     Real sigma)
{
	return constant_interval(t_start, t_end, 1000.0 / frequency, sigma);
}
}  // namespace spikes
}  // namespace cypress
