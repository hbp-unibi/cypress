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
 * @file neurons_base.hpp
 *
 * File declaring the basic types used for the description of the individual
 * neuron types.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_NEURONS_BASE_HPP
#define CYPRESS_CORE_NEURONS_BASE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <cypress/core/data.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/matrix.hpp>
#include <cypress/util/optional.hpp>

namespace cypress {

class NullNeuron;
class NullNeuronParameters;
class NullNeuronSignals;

/**
 * The NeuronType class contains data describing an individual neuron
 * type and its parameters.
 */
class NeuronType {
protected:
	/**
	 * Constructor of the NeuronTypeDescriptor structure.
	 */
	NeuronType(const std::string &name,
	           const std::vector<std::string> &parameter_names,
	           const std::vector<std::string> &parameter_units,
	           const std::vector<float> &parameter_defaults,
	           const std::vector<std::string> &signal_names,
	           const std::vector<std::string> &signal_units,
	           bool conductance_based, bool spike_source)
	    : name(name),
	      parameter_names(parameter_names),
	      parameter_units(parameter_units),
	      parameter_defaults(parameter_defaults),
	      signal_names(signal_names),
	      signal_units(signal_units),
	      conductance_based(conductance_based),
	      spike_source(spike_source)
	{
	}

public:
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
	const std::vector<float> parameter_defaults;

	/**
	 * Names of the signals that can be recorded from this neuron.
	 */
	const std::vector<std::string> signal_names;

	/**
	 * Units of the signals listed in the signal_names vector.
	 */
	const std::vector<std::string> signal_units;

	/**
	 * True if this is a conductance based neuron.
	 */
	const bool conductance_based;

	/**
	 * True if this neuron type represents a spike source.
	 */
	const bool spike_source;

	/**
	 * Resolves the given parameter name into a parameter index.
	 */
	Optional<size_t> parameter_index(const std::string &name) const;

	/**
	 * Resolves the given signal name into a signel index.
	 */
	Optional<size_t> signal_index(const std::string &name) const;
};

/**
 * Templated class allowing to conveniently build a new neuron type.
 */
template <typename Parameters_, typename Signals_>
class NeuronTypeBase : public NeuronType {
public:
	using Parameters = Parameters_;
	using Signals = Signals_;

	using NeuronType::NeuronType;
};

/**
 * Base class for the storage of neuron parameters. Basically this class just
 * contains a vector of floats, where each float in the vector corresponds to
 * a single parameter.
 */
class NeuronParameters : public PopulationDataView {
protected:
	/**
	 * Constructor allowing to set the neuron parameters to the given values.
	 */
	NeuronParameters(std::initializer_list<float> parameters)
	{
		for (auto &p : write_parameters()) {
			p = std::vector<float>(parameters);
		}
	}

	/**
	 * Constructor allowing to set the neuron parameters to the given values.
	 */
	NeuronParameters(const std::vector<float> &parameters)
	{
		for (auto &p : write_parameters()) {
			p = parameters;
		}
	}

public:
	/**
	 * Constructor allowing the NeuronParameters class to act as a view onto
	 * an existing set of parameters.
	 */
	NeuronParameters(std::shared_ptr<PopulationData> data, NeuronIndex nid0,
	                 NeuronIndex nid1)
	    : PopulationDataView(std::move(data), nid0, nid1, true, false, false)
	{
	}

	/**
	 * Returns a read-only reference at the parameter vector.
	 */
	const std::vector<float> &parameters() const { return read_parameters(); }

	/**
	 * Sets the parameter with the given index to the specified value.
	 *
	 * @param idx is the parameter that should be updated.
	 * @param value is the value the parameter should be set to.
	 */
	void set(size_t idx, float value)
	{
		for (auto &p : write_parameters()) {
			p[idx] = value;
		}
	}

	/**
	 * Returns a read-only reference at the i-th element in the parameter
	 * vector.
	 */
	float operator[](size_t i) const { return read_parameters()[i]; }

	/**
	 * Returns an iterator allowing to iterate over the parameter vector.
	 */
	const float *begin() const { return &read_parameters().front(); }

	/**
	 * Returns an iterator allowing to iterate over the parameter vector.
	 */
	const float *end() const { return &read_parameters().back() + 1; }

	/**
	 * Returns the size of the parameter vector. Aborts if the population is
	 * non-uniform.
	 */
	size_t size() const { return get_parameters_size(); }
};

/**
 * Base class from which the NeuronParameter classes are derived, providing
 * the basic constructors.
 */
template <typename Impl, typename Type_>
class NeuronParametersBase : public NeuronParameters {
public:
	/**
	 * The neuron type associated with this NeuronSignals class.
	 */
	using Type = Type_;

	using NeuronParameters::NeuronParameters;

	/**
	 * Default constructor. Initialises the parameters with the default values.
	 */
	NeuronParametersBase() : NeuronParameters(Type::inst().parameter_defaults)
	{
	}

	/**
	 * Constructor allowing to build a SpikeSourceArrayParameters instance
	 * containing the parameters for multiple neurons.
	 *
	 * @param list is a list of other SpikeSourceArrayParameters instances which
	 * should be incorporated into the new SpikeSourceArrayParameters instance.
	 */
	NeuronParametersBase(std::initializer_list<Impl> list)
	    : NeuronParameters(PopulationDataView::from_sequence(list))
	{
	}
};

/**
 * Template class for building a parameter vector which has a pre-defined size.
 */
template <typename Impl, typename Type, size_t Size>
class ConstantSizeNeuronParametersBase
    : public NeuronParametersBase<Impl, Type> {
public:
	using NeuronParametersBase<Impl, Type>::NeuronParametersBase;

	/**
	 * Default constructor.
	 */
	ConstantSizeNeuronParametersBase() = default;

	/**
	 * Constructor allowing to set the neuron parameters to the given values.
	 */
	ConstantSizeNeuronParametersBase(const std::vector<float> &parameters)
	    : NeuronParametersBase<Impl, Type>(parameters)
	{
		if (parameters.size() != Size) {
			throw InvalidParameterArraySize(
			    "Number of parameters in the parameter vector does not match "
			    "the number of parameters.");
		}
	}

	/**
	 * Returns the constant size of the parameter vector.
	 */
	static constexpr size_t size() { return Size; }
};

/**
 * The NeuronSignals class is both responsible for flagging which signals
 * should be recorded in a simulation and for actually storing the data traces
 * which are recorded during a simulation.
 */
class NeuronSignals : public PopulationDataView {
protected:
	/**
	 * Constructor accepting the number of signals in the signal vector.
	 *
	 * @param signal_count number of signals available in the neuron.
	 */
	NeuronSignals(size_t signal_count)
	{
		for (auto &p : write_record()) {
			p.resize(signal_count);
		}
		for (auto &p : write_data()) {
			p.resize(signal_count);
		}
	}

public:
	/**
	 * Constructor allowing the NeuronParameters class to act as a view onto
	 * an existing set of parameters.
	 */
	NeuronSignals(std::shared_ptr<PopulationData> data, NeuronIndex nid0,
	              NeuronIndex nid1)
	    : PopulationDataView(std::move(data), nid0, nid1, false, true, true)
	{
	}

	/**
	 * Setter for the record flag of the i-th signal.
	 *
	 * @param i is the index of the signal for which the record flag should
	 * be set.
	 * @param record specifies whether the signal should be recorded (true) or
	 * not recorded (false).
	 */
	void record(size_t i, bool record = true)
	{
		for (auto &p : write_record()) {
			p[i] = record;
		}
	}

	/**
	 * Getter for the record flag of the i-th signal.
	 */
	bool is_recording(size_t i) const { return read_record()[i]; }

	/**
	 * Allows to set the data for the i-th data entry.
	 */
	void data(size_t i, std::shared_ptr<Matrix<float>> matrix)
	{
		for (auto &p : write_data()) {
			p[i] = matrix;
		}
	}

	/**
	 * Returns a reference at the matrix containing the data for the i-th
	 * signal. Throws an SignalNotRecordedException if the signal with the
	 * given index is currently not being recorded.
	 *
	 * @param i is the signal index for which the data should be returned.
	 * @return a reference at the data matrix.
	 */
	std::shared_ptr<Matrix<float>> data_ptr(size_t i) const
	{
		static std::shared_ptr<Matrix<float>> empty =
		    std::make_shared<Matrix<float>>();
		auto res = const_cast<NeuronSignals *>(this)->read_data()[i];
		if (!res) {
			if (!is_recording(i)) {
				throw SignalNotRecordedException();
			}
			return empty;
		}
		return res;
	}

	/**
	 * Returns a reference at the matrix containing the data for the i-th
	 * signal. Throws an SignalNotRecordedException if the signal with the
	 * given index is currently not being recorded.
	 *
	 * @param i is the signal index for which the data should be returned.
	 * @return a reference at the data matrix.
	 */
	const Matrix<float> &data(size_t i) const { return *data_ptr(i); }

	/**
	 * Returns the number of signals.
	 */
	size_t size() const { return get_record_size(); }
};

/**
 * Templated base class for all neuron signal classes -- provides all needed
 * constructors.
 */
template <class Impl, class Type_, size_t Size>
class NeuronSignalsBase : public NeuronSignals {
public:
	/**
	 * The neuron type associated with this NeuronSignals class.
	 */
	using Type = Type_;

	using NeuronSignals::NeuronSignals;

	/**
	 * Default constructor -- creates neuron signal descriptor with the size
	 * specified as template argument.
	 */
	NeuronSignalsBase() : NeuronSignals(Size) {}

	/**
	 * Creates an instance of NeuronSignalsBase which records the listed
	 * signals. Throws an exception if one of the signals does not exist.
	 */
	NeuronSignalsBase(std::initializer_list<std::string> record_signal_names)
	    : NeuronSignalsBase()
	{
		for (auto &signal : record_signal_names) {
			auto idx = Type::inst().signal_index(signal);
			if (!idx.valid()) {
				throw InvalidSignal(std::string("Signal with name \"") +
				                    signal + "\" does not exist.");
			}
			record(idx.value());
		}
	}

	/**
	 * Returns the number of signals.
	 */
	static constexpr size_t size() { return Size; }
};

/*
 * NullNeuron
 */
class NullNeuronParameters;
class NullNeuronSignals;

/**
 * Internally used neuron type representing no neuron type.
 */
class NullNeuron final
    : public NeuronTypeBase<NullNeuronParameters, NullNeuronSignals> {
private:
	NullNeuron();

public:
	static const NullNeuron &inst();
};

/**
 * Internally used parameter type with no parameters.
 */
class NullNeuronParameters final
    : public ConstantSizeNeuronParametersBase<NullNeuronParameters, NullNeuron,
                                              0> {
public:
	using ConstantSizeNeuronParametersBase<NullNeuronParameters, NullNeuron,
	                                       0>::ConstantSizeNeuronParametersBase;
};

class NullNeuronSignals final
    : public NeuronSignalsBase<NullNeuronSignals, NullNeuron, 0> {
public:
	using NeuronSignalsBase<NullNeuronSignals, NullNeuron,
	                        0>::NeuronSignalsBase;
};
}

#endif /* CYPRESS_CORE_NEURONS_BASE_HPP */
