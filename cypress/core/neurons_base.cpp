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

#include <cypress/core/neurons_base.hpp>

namespace cypress {
/*
 * Class NeuronType
 */

Optional<size_t> NeuronType::parameter_index(const std::string &name) const
{
	auto it = std::find(parameter_names.begin(), parameter_names.end(), name);
	if (it != parameter_names.end()) {
		return Optional<size_t>(it - parameter_names.begin());
	}
	return Optional<size_t>();
}

Optional<size_t> NeuronType::signal_index(const std::string &name) const
{
	auto it = std::find(signal_names.begin(), signal_names.end(), name);
	if (it != signal_names.end()) {
		return Optional<size_t>(it - signal_names.begin());
	}
	return Optional<size_t>();
}

/*
 * Class NullNeuronType
 */

NullNeuron::NullNeuron()
    : NeuronTypeBase(0, "", {}, {}, {}, {}, {}, false, false)
{
}

const NullNeuron &NullNeuron::inst()
{
	static const NullNeuron inst;
	return inst;
}
}
