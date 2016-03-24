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

#include <cypress/core/neurons_base.hpp>

/**
 * Macro used for defining the getters and setters associated with a neuron
 * parameter value.
 */
#define NAMED_PARAMETER(NAME, IDX)            \
	static constexpr size_t idx_##NAME = IDX; \
	auto &NAME(float x)                       \
	{                                         \
		for (auto &p : write_parameters()) {  \
			p[IDX] = x;                       \
		}                                     \
		return *this;                         \
	}                                         \
	float NAME() const { return read_parameters()[IDX]; }

/**
 * Macro used for defining a named signal associated with a named signal.
 */
#define NAMED_SIGNAL(NAME, IDX)                              \
	static constexpr size_t idx_##NAME = IDX;                \
	auto &record_##NAME(bool x = true)                       \
	{                                                        \
		record(IDX, x);                                      \
		return *this;                                        \
	}                                                        \
	bool is_recording_##NAME() { return is_recording(IDX); } \
	auto get_##NAME() { return data(IDX); }

namespace cypress {
/*
 * Forward declarations
 */
class SpikeSourceArray;
class SpikeSourceArrayParameters;
class SpikeSourceArraySignals;
class IfCondExp;
class IfCondExpParameters;
class IfCondExpSignals;
class IfFacetsHardware1;
class IfFacetsHardware1Parameters;
class IfFacetsHardware1Signals;
class EifCondExpIsfaIsta;
class EifCondExpIsfaIstaParameters;
class EifCondExpIsfaIstaSignals;

/*
 * SpikeSourceArray
 */

/**
 * Neuron type representing a spike source array.
 */
class SpikeSourceArray final : public NeuronTypeBase<SpikeSourceArrayParameters,
                                                     SpikeSourceArraySignals> {
private:
	SpikeSourceArray();

public:
	static const SpikeSourceArray &inst();
};

/**
 * The SpikeSourceArray missuses the parameter storage as storage for the
 * individual spike times.
 */
class SpikeSourceArrayParameters final
    : public NeuronParametersBase<SpikeSourceArrayParameters,
                                  SpikeSourceArray> {
public:
	using NeuronParametersBase<SpikeSourceArrayParameters,
	                           SpikeSourceArray>::NeuronParametersBase;

	SpikeSourceArrayParameters() : NeuronParametersBase() {}

	/**
	 * Constructor allowing to specify an arbitrary number of spike times from
	 * a vector.
	 */
	SpikeSourceArrayParameters(const std::vector<float> &spike_times)
	    : NeuronParametersBase(spike_times)
	{
	}

	/**
	 * Constructor allowing to specify an arbitrary number of spike times.
	 */
	SpikeSourceArrayParameters(std::initializer_list<float> spike_times)
	    : NeuronParametersBase(spike_times)
	{
	}

	const std::vector<float> &spike_times() const { return read_parameters(); }

	SpikeSourceArrayParameters &spike_times(
	    const std::vector<float> &spike_times)
	{
		for (auto &p : write_parameters()) {
			p = spike_times;
		}
		return *this;
	}
};

class SpikeSourceArraySignals final
    : public NeuronSignalsBase<SpikeSourceArraySignals, SpikeSourceArray, 1> {
public:
	using NeuronSignalsBase<SpikeSourceArraySignals, SpikeSourceArray,
	                        1>::NeuronSignalsBase;

	NAMED_SIGNAL(spikes, 0);
};

/*
 * IfCondExp
 */

/**
 * Neuron type representing an integrate and fire neuron with conductance based
 * synapses with expontential decay.
 */
class IfCondExp final
    : public NeuronTypeBase<IfCondExpParameters, IfCondExpSignals> {
private:
	IfCondExp();

public:
	static const IfCondExp &inst();
};

class IfCondExpParameters final
    : public ConstantSizeNeuronParametersBase<IfCondExpParameters, IfCondExp,
                                              11> {
public:
	using ConstantSizeNeuronParametersBase<
	    IfCondExpParameters, IfCondExp, 11>::ConstantSizeNeuronParametersBase;

	NAMED_PARAMETER(cm, 0);
	NAMED_PARAMETER(tau_m, 1);
	NAMED_PARAMETER(tau_syn_E, 2);
	NAMED_PARAMETER(tau_syn_I, 3);
	NAMED_PARAMETER(tau_refrac, 4);
	NAMED_PARAMETER(v_rest, 5);
	NAMED_PARAMETER(v_thresh, 6);
	NAMED_PARAMETER(v_reset, 7);
	NAMED_PARAMETER(e_rev_E, 8);
	NAMED_PARAMETER(e_rev_I, 9);
	NAMED_PARAMETER(i_offset, 10);
};

class IfCondExpSignals final
    : public NeuronSignalsBase<IfCondExpSignals, IfCondExp, 4> {
public:
	using NeuronSignalsBase<IfCondExpSignals, IfCondExp, 4>::NeuronSignalsBase;

	IfCondExpSignals() : NeuronSignalsBase(4) {}

	IfCondExpSignals(std::initializer_list<IfCondExpSignals> list)
	    : NeuronSignalsBase(PopulationDataView::from_sequence(list))
	{
	}

	NAMED_SIGNAL(spikes, 0);
	NAMED_SIGNAL(v, 1);
	NAMED_SIGNAL(gsyn_exc, 2);
	NAMED_SIGNAL(gsyn_inh, 3);
};

/*
 * IfFacetsHardware1
 */

/**
 * Neuron type used on the Spikey hardware.
 */
class IfFacetsHardware1 final
    : public NeuronTypeBase<IfFacetsHardware1Parameters,
                            IfFacetsHardware1Signals> {
private:
	IfFacetsHardware1();

public:
	static const IfFacetsHardware1 &inst();
};

class IfFacetsHardware1Parameters final
    : public ConstantSizeNeuronParametersBase<IfFacetsHardware1Parameters,
                                              IfFacetsHardware1, 6> {
public:
	using ConstantSizeNeuronParametersBase<IfFacetsHardware1Parameters,
	                                       IfFacetsHardware1,
	                                       6>::ConstantSizeNeuronParametersBase;

	NAMED_PARAMETER(g_leak, 0);
	NAMED_PARAMETER(tau_refrac, 1);
	NAMED_PARAMETER(v_rest, 2);
	NAMED_PARAMETER(v_thresh, 3);
	NAMED_PARAMETER(v_reset, 4);
	NAMED_PARAMETER(e_rev_I, 5);
};

class IfFacetsHardware1Signals final
    : public NeuronSignalsBase<IfFacetsHardware1Signals, IfFacetsHardware1, 4> {
public:
	using NeuronSignalsBase<IfFacetsHardware1Signals, IfFacetsHardware1,
	                        4>::NeuronSignalsBase;

	IfFacetsHardware1Signals() : NeuronSignalsBase(4) {}

	IfFacetsHardware1Signals(std::initializer_list<IfFacetsHardware1Signals> list)
	    : NeuronSignalsBase(PopulationDataView::from_sequence(list))
	{
	}

	NAMED_SIGNAL(spikes, 0);
	NAMED_SIGNAL(v, 1);
};

/*
 * EifCondExpIsfaIsta
 */

/**
 * Exponential integrate and fire neuron model also known as AdEx.
 */
class EifCondExpIsfaIsta final
    : public NeuronTypeBase<EifCondExpIsfaIstaParameters,
                            EifCondExpIsfaIstaSignals> {
private:
	EifCondExpIsfaIsta();

public:
	static const EifCondExpIsfaIsta &inst();
};

class EifCondExpIsfaIstaParameters final
    : public ConstantSizeNeuronParametersBase<EifCondExpIsfaIstaParameters,
                                              EifCondExpIsfaIsta, 15> {
public:
	using ConstantSizeNeuronParametersBase<
	    EifCondExpIsfaIstaParameters, EifCondExpIsfaIsta,
	    15>::ConstantSizeNeuronParametersBase;

	NAMED_PARAMETER(cm, 0);
	NAMED_PARAMETER(tau_m, 1);
	NAMED_PARAMETER(tau_syn_E, 2);
	NAMED_PARAMETER(tau_syn_I, 3);
	NAMED_PARAMETER(tau_refrac, 4);
	NAMED_PARAMETER(tau_w, 5);
	NAMED_PARAMETER(v_rest, 6);
	NAMED_PARAMETER(v_thresh, 7);
	NAMED_PARAMETER(v_reset, 8);
	NAMED_PARAMETER(e_rev_E, 9);
	NAMED_PARAMETER(e_rev_I, 10);
	NAMED_PARAMETER(i_offset, 11);
	NAMED_PARAMETER(a, 12);
	NAMED_PARAMETER(b, 13);
	NAMED_PARAMETER(delta_T, 14);
};

class EifCondExpIsfaIstaSignals final
    : public NeuronSignalsBase<EifCondExpIsfaIstaSignals, EifCondExpIsfaIsta,
                               4> {
public:
	using NeuronSignalsBase<EifCondExpIsfaIstaSignals, EifCondExpIsfaIsta,
	                        4>::NeuronSignalsBase;

	NAMED_SIGNAL(spikes, 0);
	NAMED_SIGNAL(v, 1);
	NAMED_SIGNAL(gsyn_exc, 2);
	NAMED_SIGNAL(gsyn_inh, 3);
};
}

#undef NAMED_SIGNAL
#undef NAMED_PARAMETER

#endif /* CYPRESS_CORE_NEURONS_HPP */
