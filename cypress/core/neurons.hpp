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
 * @file neurons.hpp
 *
 * Contains the declarations of the individual neuron types and their parameter
 * sets. The basic pattern used here is that the individual neuron types inherit
 * from the NeuronType class which describes each neuron type in a generic way.
 * Each neuron type declares a singleton type which fills out the descriptor and
 * provides a reference at a parameter type. The parameter type merely is a
 * vector of floats. The individual neuron type parameters may be set using
 * convenient getter/setter functions. Neither the individual neuron types nor
 * the neuron parameter types add any new non-function members to their base.
 * This allows to generically use the base classes throughout the rest of the
 * code without having to deal with templates.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_NEURONS_HPP
#define CYPRESS_CORE_NEURONS_HPP

#include <string>
#include <vector>

#include <cypress/core/neurons.hpp>

namespace cypress {
/**
 * Base class for the storage of neuron parameters. Basically this class just
 * contains a vector of floats, where each float in the vector corresponds to
 * a single parameter.
 */
class NeuronParametersBase {
private:
	/**
	 * Vector containing the parameters.
	 */
	std::vector<float> m_parameters;

protected:
	/**
	 * Getter function which allows direct access at the underlying parameter
	 * vector.
	 */
	std::vector<float> &parameters() { return m_parameters; }
	const std::vector<float> &parameters() const { return m_parameters; }

public:
	/**
	 * Default constructor.
	 */
	NeuronParametersBase() {}

	/**
	 * Constructor allowing to set the neuron parameters to the given values.
	 */
	NeuronParametersBase(std::initializer_list<float> parameters)
	    : m_parameters(parameters)
	{
	}

	/**
	 * Constructor allowing to set the neuron parameters to the given values.
	 */
	NeuronParametersBase(const std::vector<float> &parameters)
	    : m_parameters(parameters)
	{
	}

	/**
	 * Operator allowing to access the i-th parameter.
	 */
	float &operator[](size_t i) { return m_parameters[i]; }

	/**
	 * Operator allowing to access the i-th parameter.
	 */
	float operator[](size_t i) const { return m_parameters[i]; }

	/**
	 * Method returning a read-only pointer at the first parameter.
	 */
	const float *begin() const { return &m_parameters[0]; }

	/**
	 * Method returning a read-only pointer at the last parameter.
	 */
	const float *end() const { return &m_parameters[0] + size(); }

	/**
	 * Number of parameters.
	 */
	size_t size() const { return m_parameters.size(); }
};

/**
 * The NeuronType class contains data describing an individual neuron
 * type and its parameters.
 */
class NeuronType {
protected:
	/**
	 * Constructor of the NeuronTypeDescriptor structure.
	 */
	NeuronType(int type_id, const std::string &name,
	           const std::vector<std::string> &parameter_names,
	           const std::vector<std::string> &parameter_units,
	           const std::vector<float> &parameter_defaults,
	           bool conductance_based, bool spike_source)
	    : type_id(type_id),
	      name(name),
	      parameter_names(parameter_names),
	      parameter_units(parameter_units),
	      parameter_defaults(parameter_defaults),
	      conductance_based(conductance_based),
	      spike_source(spike_source)
	{
	}

public:
	/**
	 * Type id as understood by the Python part of Cypress.
	 */
	const int type_id;

	/**
	 * Name of the neuron type.
	 */
	const std::string name;

	/**
	 * Name of all neuron parameters.
	 */
	const std::vector<std::string> parameter_names;

	/**
	 * Units of all neuron parameters.
	 */
	const std::vector<std::string> parameter_units;

	/**
	 * Contains default values for the neuron parameters.
	 */
	const NeuronParametersBase parameter_defaults;

	/**
	 * True if this is a conductance based neuron.
	 */
	const bool conductance_based;

	/**
	 * True if this neuron type represents a spike source.
	 */
	const bool spike_source;
};

/**
 * Internally used parameter type with no parameters.
 */
class NullNeuronParameters final : public NeuronParametersBase {
public:
	NullNeuronParameters() : NeuronParametersBase(std::vector<float>{}) {}
};

/**
 * The SpikeSourceArray missuses the parameter storage as storage for the
 * individual spike times.
 */
class SpikeSourceArrayParameters final : public NeuronParametersBase {
public:
	/**
	 * Default constructor of SpikeSourceArrayParameters.
	 */
	SpikeSourceArrayParameters() {}

	/**
	 * Constructor allowing to specify an arbitrary number of spike times.
	 */
	SpikeSourceArrayParameters(std::initializer_list<float> spike_times)
	    : NeuronParametersBase(spike_times)
	{
	}

	std::vector<float> &spike_times() { return parameters(); }
	const std::vector<float> &spike_times() const { return parameters(); }
};

#define NAMED_VECTOR_ELEMENT(NAME, IDX)       \
	static constexpr size_t idx_##NAME = IDX; \
	auto &NAME(float x)                       \
	{                                         \
		(*this)[IDX] = x;                     \
		return *this;                         \
	}                                         \
	float &NAME() { return (*this)[IDX]; }    \
	float NAME() const { return (*this)[IDX]; }

class IfCondExpParameters final : public NeuronParametersBase {
public:
	IfCondExpParameters();

	NAMED_VECTOR_ELEMENT(cm, 0);
	NAMED_VECTOR_ELEMENT(tau_m, 1);
	NAMED_VECTOR_ELEMENT(tau_syn_E, 2);
	NAMED_VECTOR_ELEMENT(tau_syn_I, 3);
	NAMED_VECTOR_ELEMENT(tau_refrac, 4);
	NAMED_VECTOR_ELEMENT(v_rest, 5)
	NAMED_VECTOR_ELEMENT(v_thresh, 6)
	NAMED_VECTOR_ELEMENT(v_reset, 7)
	NAMED_VECTOR_ELEMENT(e_rev_E, 8)
	NAMED_VECTOR_ELEMENT(e_rev_I, 9)
	NAMED_VECTOR_ELEMENT(i_offset, 10)
};

class EifCondExpIsfaIstaParameters final : public NeuronParametersBase {
public:
	EifCondExpIsfaIstaParameters();

	NAMED_VECTOR_ELEMENT(cm, 0);
	NAMED_VECTOR_ELEMENT(tau_m, 1);
	NAMED_VECTOR_ELEMENT(tau_syn_E, 2);
	NAMED_VECTOR_ELEMENT(tau_syn_I, 3);
	NAMED_VECTOR_ELEMENT(tau_refrac, 4);
	NAMED_VECTOR_ELEMENT(tau_w, 5)
	NAMED_VECTOR_ELEMENT(v_rest, 6)
	NAMED_VECTOR_ELEMENT(v_thresh, 7)
	NAMED_VECTOR_ELEMENT(v_reset, 8)
	NAMED_VECTOR_ELEMENT(e_rev_E, 9)
	NAMED_VECTOR_ELEMENT(e_rev_I, 10)
	NAMED_VECTOR_ELEMENT(i_offset, 11)
	NAMED_VECTOR_ELEMENT(a, 12)
	NAMED_VECTOR_ELEMENT(b, 13)
	NAMED_VECTOR_ELEMENT(delta_T, 14)
};

#undef NAMED_VECTOR_ELEMENT

/**
 * Internally used neuron type representing no neuron type.
 */
class NullNeuronType final : public NeuronType {
private:
	NullNeuronType();

public:
	using Parameters = NullNeuronParameters;

	static const NullNeuronType &inst();
};

class SpikeSourceArray final : public NeuronType {
private:
	SpikeSourceArray();

public:
	using Parameters = SpikeSourceArrayParameters;

	static const SpikeSourceArray &inst();
};

class IfCondExp final : public NeuronType {
private:
	IfCondExp();

public:
	using Parameters = IfCondExpParameters;

	static const IfCondExp &inst();
};

class EifCondExpIsfaIsta final : public NeuronType {
private:
	EifCondExpIsfaIsta();

public:
	using Parameters = EifCondExpIsfaIstaParameters;

	static const EifCondExpIsfaIsta &inst();
};
}

#endif /* CYPRESS_CORE_NEURONS_HPP */
