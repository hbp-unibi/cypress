/*
 *  Cypress -- C++ Neural Associative Memory Simulator
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

namespace cypress {

/**
 * The NeuronType class contains data describing an individual neuron
 * type and its parameters.
 */
struct NeuronType {
	/**
	 * Type id as understood by the Python part of Cypress.
	 */
	int type_id;

	/**
	 * Name of the neuron type.
	 */
	std::string name;

	/**
	 * Name of all neuron parameters.
	 */
	std::vector<std::string> parameter_names;

	/**
	 * Contains default values for the neuron parameters.
	 */
	std::vector<float> parameter_defaults;

	/**
	 * Constructor of the NeuronTypeDescriptor structure.
	 */
	NeuronType(int type_id, const std::string &name,
	           const std::vector<std::string> &parameter_names,
	           const std::vector<float> &parameter_defaults)
	    : type_id(type_id),
	      name(name),
	      parameter_names(parameter_names),
	      parameter_defaults(parameter_defaults)
	{
	}
};

/**
 * Base class for the storage of neuron parameters.
 */
class NeuronParametersBase {
private:
	std::vector<float> m_parameters;

protected:
	std::vector<float> &parameters() { return m_parameters; }

public:
	NeuronParametersBase(const NeuronTypeDescriptor &descr)
	    : m_parameters(descr.parameter_defaults)
	{
	}

	float &operator[](size_t i) { return m_parameters[i]; }

	const float &operator[](size_t i) const { return m_parameters[i]; }

	size_t size() { return m_parameters; }
};

class SpikeSourceArrayParameters: public NeuronParametersBase {
public:
	SpikeSourceArrayParameters();

	std::vector<float> &spike_times() {return parameters();}
};

struct SpikeSourceArray: public NeuronType {
private:
	SpikeSourceArray();

public:
	using Parameters = SpikeSourceArrayParameters;

	static const SpikeSourceArray &inst();
};


}
