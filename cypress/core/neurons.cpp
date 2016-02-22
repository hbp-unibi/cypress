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

#include <cypress/core/exceptions.hpp>
#include <cypress/core/neurons.hpp>

namespace cypress {

/*
 * Class NeuronType
 */

size_t NeuronType::parameter_index(const std::string &name) const
{
	auto it = std::find(parameter_names.begin(), parameter_names.end(), name);
	if (it == parameter_names.end()) {
		throw InvalidParameter(std::string("Signal with name \"") + name +
		                       "\" does not exist.");
	}
	return it - parameter_names.begin();
}

size_t NeuronType::signal_index(const std::string &name) const
{
	auto it = std::find(signal_names.begin(), signal_names.end(), name);
	if (it == parameter_names.end()) {
		throw InvalidSignal(std::string("Signal with name \"") + name +
		                    "\" does not exist.");
	}
	return it - signal_names.begin();
}

/*
 * Class NullNeuronType
 */

NullNeuronType::NullNeuronType()
    : NeuronType(0, "", {}, {}, {}, {}, {}, false, false)
{
}

const NullNeuronType &NullNeuronType::inst()
{
	static const NullNeuronType inst;
	return inst;
}

/*
 * Class SpikeSourceArray
 */

SpikeSourceArray::SpikeSourceArray()
    : NeuronType(0, "SpikeSourceArray", {"spike_times"}, {"ms"}, {}, {"spikes"},
                 {"ms"}, false, true)
{
}

const SpikeSourceArray &SpikeSourceArray::inst()
{
	static SpikeSourceArray inst;
	return inst;
}

/*
 * Class IfCondExp
 */

IfCondExp::IfCondExp()
    : NeuronType(
          1, "IfCondExp",
          {"cm", "tau_m", "tau_syn_E", "tau_syn_I", "tau_refrac", "v_rest",
           "v_thresh", "v_reset", "e_rev_E", "e_rev_I", "i_offset"},
          {"nF", "ms", "ms", "ms", "ms", "mV", "mV", "mV", "mV", "mV", "nA"},
          {1.0, 20.0, 5.0, 5.0, 0.1, -65.0, -50.0, -65.0, 0.0, -70.0, 0.0},
          {"spikes", "v", "gsyn_exc", "gsyn_inh"}, {"ms", "mV", "uS", "uS"},
          true, false)
{
}

const IfCondExp &IfCondExp::inst()
{
	static IfCondExp inst;
	return inst;
}

/*
 * Class EifCondExpIsfaIsta
 */

EifCondExpIsfaIsta::EifCondExpIsfaIsta()
    : NeuronType(2, "EifCondExpIsfaIsta",
                 {"cm", "tau_m", "tau_syn_E", "tau_syn_I", "tau_refrac",
                  "tau_w", "v_rest", "v_thresh", "v_reset", "e_rev_E",
                  "e_rev_I", "i_offset", "a", "b", "delta_T"},
                 {"nF", "ms", "ms", "ms", "ms", "ms", "mV", "mV", "mV", "mV",
                  "mV", "nA", "nS", "nA", "mV"},
                 {1.0, 20.0, 5.0, 5.0, 0.1, 144.0, -70.6, -50.4, -70.6, 0.0,
                  -80.0, 0.0, 4.0, 0.0805, 2.0},
                 {"spikes", "v", "gsyn_exc", "gsyn_inh"},
                 {"ms", "mV", "uS", "uS"}, true, false)
{
}

const EifCondExpIsfaIsta &EifCondExpIsfaIsta::inst()
{
	static EifCondExpIsfaIsta inst;
	return inst;
}

/*
 * Class IfCondExpParameters
 */

IfCondExpParameters::IfCondExpParameters()
    : NeuronParametersBase(IfCondExp::inst().parameter_defaults)
{
}

/*
 * Class EifCondExpIsfaIstaParameters
 */

EifCondExpIsfaIstaParameters::EifCondExpIsfaIstaParameters()
    : NeuronParametersBase(EifCondExpIsfaIsta::inst().parameter_defaults)
{
}
}
