/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
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

#include <algorithm>  //std::sort
#include <iomanip>
#include <random>
#include <string>
#include <vector>

#include "neuron_parameters.hpp"

namespace cypress {
namespace {

std::vector<cypress::Real> read_neuron_parameters_from_json(
    const cypress::NeuronType &type, const cypress::Json &obj)
{
	std::map<std::string, cypress::Real> input =
	    json_to_map<Real>(obj);
	// special case if g_leak was given instead of tau_m,
	// But not on spikey, as there g_leak is the standard parameter name
	auto iter = input.find("g_leak");
	if (iter != input.end() && &type != &cypress::IfFacetsHardware1::inst()) {
		auto iter2 = input.find("cm");
		cypress::Real cm = 0;
		if (iter2 == input.end()) {
			for (size_t j = 0; j < type.parameter_names.size(); j++) {
				if (type.parameter_names[j] == "cm") {
					cm = type.parameter_defaults[j];
					break;
				}
			}
		}
		else {
			cm = input["cm"];
		}
		input["tau_m"] = cm / input["g_leak"];
		input.erase(iter);
	}

	// special case if tau_m was given instead of g_leak on spikey
	iter = input.find("tau_m");
	if (iter != input.end() && &type == &cypress::IfFacetsHardware1::inst()) {
		input["g_leak"] = 0.2 / input["tau_m"];
		input.erase(iter);
	}

	return read_check<cypress::Real>(input, type.parameter_names,
	                                 type.parameter_defaults);
}
}

NeuronParameter::NeuronParameter(const cypress::NeuronType &type,
                                   const cypress::Json &json)
    : m_parameter_names(type.parameter_names)
{
	m_params = read_neuron_parameters_from_json(type, json);
	std::stringstream msg;
	msg << " Neuron Parameters:\n";
	for (size_t i = 0; i < m_params.size(); i++) {
		msg << std::setw(15) << type.parameter_names[i] << ":\t "
		    << std::setw(10) << std::to_string(m_params[i]) << "\n";
	}
	global_logger().debug("Cypress", msg.str());
}
}
