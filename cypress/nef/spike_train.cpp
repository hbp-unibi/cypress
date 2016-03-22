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

#include <cypress/nef/spike_train.hpp>

namespace cypress {
namespace nef {

std::vector<float> SpikeTrain::decode(const std::vector<float> &spikes,
                                      const DiscreteWindow &window, float t0,
                                      float t1, float min_val, float max_val)
{
	// Calculate the number of samples in the result
	const float i_step = 1.0f / window.step();
	const size_t n_samples = std::ceil((t1 - t0) * i_step);
	const size_t size = window.size();
	const size_t center = (size - 1) / 2;

	// Iterate over the input spikes and accumulate the windows for each
	// spike
	std::vector<float> res(n_samples);
	for (float spike : spikes) {
		const ptrdiff_t offs = (spike - t0) * i_step - center;
		const size_t i0 = std::max<ptrdiff_t>(0, offs);
		const size_t i1 = std::max<ptrdiff_t>(
		    0, std::min<ptrdiff_t>(res.size(), offs + size));
		const size_t j0 = std::max<ptrdiff_t>(0, i0 - offs);
		for (size_t i = i0, j = j0; i < i1; i++, j++) {
			res[i] += window[j];
		}
	}

	// Rescale the output -- assume the result values are scaled between zero
	// and one
	const float scale = max_val - min_val;
	for (size_t i = 0; i < res.size(); i++) {
		res[i] = res[i] * scale + min_val;
	}
	return res;
}
}
}
