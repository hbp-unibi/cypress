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
 * @file network_base.hpp
 *
 * Header containing the declarations of the basic network, neuron and
 * population classes. The classes used in this header are generic in a sense
 * that they can represent neurons and populations of any type. If possible do
 * not use the types in this header directly, but use the templated versions
 * in network.hpp.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_NETWORK_BASE_HPP
#define CYPRESS_CORE_NETWORK_BASE_HPP

#include <memory>
#include <string>
#include <vector>

#include <cypress/core/connector.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/core/types.hpp>

namespace cypress {

/*
 * Forward declarations
 */
namespace internal {
class NetworkImpl;
}

class Backend;
class NetworkBase;
class PopulationBase;
class PopulationViewBase;
class NeuronBase;

template <typename Impl, typename Accessor, typename Params, typename Signals>
class PopulationMixin;

template <typename Impl, typename Accessor, typename Params>
class NeuronMixin;

template <typename Impl, typename Accessor, typename Params>
class ParametersMixin;

template <typename Impl, typename Accessor>
class ConnectableMixin;

/**
 * The NetworkBase class represents an entire spiking neural network. Note that
 * this class only represents a lightweight handle at the actual network,
 * copying this handle does not create a new network instance. Use the clone()
 * method to create a new NetworkBase instance.
 */
class NetworkBase {
private:
	/**
	 * Reference at the object providing the actual network implementation.
	 */
	std::shared_ptr<internal::NetworkImpl> m_impl;

protected:
	template <typename Impl, typename Accessor, typename Params,
	          typename Signals>
	friend class PopulationMixin;

	template <typename Impl, typename Accessor, typename Params>
	friend class NeuronMixin;

	template <typename Impl, typename Accessor, typename Params>
	friend class ParametersMixin;

	template <typename Impl, typename Accessor>
	friend class ConnectableMixin;

	/**
	 * Returns the size of the given population.
	 */
	size_t population_size(PopulationIndex pid) const;

	/**
	 * Returns true if all neurons in the given population share the same
	 * parameters.
	 */
	const NeuronType &population_type(PopulationIndex pid) const;

	/**
	 * Returns the name of the given population.
	 */
	const std::string &population_name(PopulationIndex pid) const;

	/**
	 * Allows to set the name of the population with the given index.
	 */
	void population_name(PopulationIndex pid, const std::string &name);

	/**
	 * Returns a reference at the signals instance of the given population.
	 */
	NeuronSignalsBase &signals(PopulationIndex pid);

	/**
	 * Returns a const reference at the signals instance of the given
	 * population.
	 */
	const NeuronSignalsBase &signals(PopulationIndex pid) const;

	/**
	 * Enables or disables recording of the given signal for the population with
	 * the given id.
	 */
	void record(PopulationIndex pid, size_t signal_idx, bool record);

	/**
	 * Returns true if all neurons in the given population share the same
	 * parameters.
	 */
	bool homogeneous(PopulationIndex pid);

	/**
	 * Returns the parameters of the nid-th neuron in the given population.
	 */
	const NeuronParametersBase &parameters(PopulationIndex pid,
	                                       NeuronIndex nid) const;

	/**
	 * Sets the neuron parameters for the neurons nid0 to nid1 (exclusive)
	 * in the populatid pid to the parameters specified in the given parameter
	 * list. If the size of the specified range and the number of elements
	 * in the parameter vector do not match, an exception is thrown.
	 *
	 * @param pid is the index of the population that should be modified.
	 * @param nid0 is the index of the first neuron that should be modified.
	 * @param nid1 is the index of the last-plus-one neuron that should be
	 * modified.
	 * @param params is a vector containing parameters for each of the neurons
	 * that should be updated.
	 */
	void parameters(PopulationIndex pid, NeuronIndex nid0, NeuronIndex nid1,
	                const std::vector<NeuronParametersBase> &params);

	/**
	 * Sets the neuron parameters for the neurons nid0 to nid1 (exclusive) in
	 * the populatid pid to the parameters specified in the given parameter
	 * list.
	 *
	 * @param pid is the index of the population that should be modified.
	 * @param nid0 is the index of the first neuron that should be modified.
	 * @param nid1 is the index of the last-plus-one neuron that should be
	 * modified.
	 * @param params describes the parameters all neurons in the specified range
	 * should be set to.
	 */
	void parameters(PopulationIndex pid, NeuronIndex nid0, NeuronIndex nid1,
	                const NeuronParametersBase &params);

	/**
	 * Sets the neuron parameters for the neurons nid0 to nid1 (exclusive) in
	 * the populatid pid to the parameters specified in the given parameter
	 * list. If the size of the specified range and the number of elements
	 * in the parameter vector do not match, an exception is thrown.
	 *
	 * @param pid is the index of the population that should be modified.
	 * @param nid0 is the index of the first neuron that should be modified.
	 * @param nid1 is the index of the last-plus-one neuron that should be
	 * modified.
	 * @param index is the index of the parameter that should be updated. If the
	 * index is out of bounds, an exception will be thrown.
	 * @param values is a vector containing value for each of the neurons
	 * that should be updated.
	 */
	void parameter(PopulationIndex pid, NeuronIndex nid0, NeuronIndex nid1,
	               size_t index, const std::vector<float> &values);

	/**
	 * Sets a single neuron parameter for the neurons nid0 to nid1 (exclusive)
	 * in the populatid pid to the parameters specified in the given parameter
	 * list.
	 *
	 * @param pid is the index of the population that should be modified.
	 * @param nid0 is the index of the first neuron that should be modified.
	 * @param nid1 is the index of the last-plus-one neuron that should be
	 * modified.
	 * @param index is the index of the parameter that should be updated. If the
	 * index is out of bounds, an exception will be thrown.
	 * @param value is the value the parameter with the given index should be
	 * set to.
	 */
	void parameter(PopulationIndex pid, NeuronIndex nid0, NeuronIndex nid1,
	               size_t index, float value);

	/**
	 * Base connection method. Connects a range of neurons in the source
	 * population to a range of neurons in the target population. All connection
	 * methods in ConnectableMixin are relayed to this method.
	 *
	 * @param pid_src is the index of the source population.
	 * @param nid_src0 is the index of the first neuron in this population that
	 * is going to be connected.
	 * @param nid_src1 is the index of the last-plus-one neuron in this
	 * population being connected.
	 * @param pid_tar is the index of the target population.
	 * @param nid_tar0 is the index of the first neuron in the target population
	 * that is going to be connected.
	 * @param nid_tar1 is the index of the last-plus-one neuron in the target
	 * population that is going to be connected.
	 * @param connector is a connector which is used to construct the actual
	 * neuron-to-neuron connections on demand.
	 * @return a reference at this object for simple method chaining.
	 */
	void connect(PopulationIndex pid_src, NeuronIndex nid_src0,
	             NeuronIndex nid_src1, PopulationIndex pid_tar,
	             NeuronIndex nid_tar0, NeuronIndex nid_tar1,
	             std::unique_ptr<Connector> connector);

	/**
	 * Internally used to add a new population. Use the templated public
	 * create_population method in the Network class instead.
	 *
	 * @param size is the number of neurons in the population.
	 * @param type is the type of the neurons in the population.
	 * @param params contains the initial neuron parameters. A vector with no
	 * entries corresponds to the default parameters, a vector whith one entry
	 * corresponds to homogeneous parameters accross the entire population,
	 * a vector with as many elements as the size of the population corresponds
	 * to individual parameters for each neuron in the population.
	 * @param name is the name of the population.
	 */
	PopulationIndex create_population_index(
	    size_t size, const NeuronType &type,
	    const std::vector<NeuronParametersBase> &params,
	    const NeuronSignalsBase &signals,
	    const std::string &name);

public:
	/**
	 * Default constructor. Creates a new network.
	 */
	NetworkBase();

	/**
	 * Creates a network instance with a reference at an already existing
	 * network. Used internally.
	 */
	NetworkBase(std::shared_ptr<internal::NetworkImpl> impl) : m_impl(impl) {}

	/**
	 * Destructor. Destroys this network instance and the handle pointing at
	 * underlying network.
	 */
	~NetworkBase();

	/**
	 * Compares whether two NetworkBase instances point at the same underlying
	 * network.
	 */
	bool operator==(const NetworkBase &o) const { return m_impl == o.m_impl; }

	/**
	 * Compares whether two NetworkBase instances point at different underlying
	 * network instances.
	 */
	bool operator!=(const NetworkBase &o) const { return m_impl != o.m_impl; }

	/**
	 * Creates and returns a copy of the network.
	 */
	NetworkBase clone() const;

	/**
	 * Returns the number of created populations.
	 */
	size_t population_count() const;

	/**
	 * Returns a PopulationBase instance pointing at the population with the
	 * given index.
	 */
	PopulationBase population(PopulationIndex pid);

	/**
	 * Returns a list of populations available in the network, filtered by the
	 * given name and type.
	 *
	 * @param name is the name the returned populations should be filtered by.
	 * If an empty string is given, no filtering is performed (default).
	 * @param type is the neuron type the list should be filtered for. If
	 * a pointer at the special NullNeuronType instance  is given, no filtering
	 * by type is performed (default).
	 * @return a list of PopulationBase instances, pointing at the populations
	 * matching the given requirements.
	 */
	const std::vector<PopulationBase> populations(
	    const std::string &name = std::string(),
	    const NeuronType &type = NullNeuronType::inst()) const;

	/**
	 * Returns a list of populations filtered by the given neuron type.
	 *
	 * @param type is the neuron type the list should be filtered for. If
	 * a pointer at the special NullNeuronType instance  is given, no filtering
	 * by type is performed (default).
	 * @return a list of PopulationBase instances, pointing at the populations
	 * matching the given requirements.
	 */
	const std::vector<PopulationBase> populations(const NeuronType &type) const;

	/**
	 * Returns a Population object pointing at the last created population with
	 * the given name. If no such population exists, an exception is thrown.
	 *
	 * @param name is the name of the population that should be looked up. If
	 * empty, the last created population is returned.
	 * @return a population handle pointing at the requested population.
	 */
	PopulationBase population(const std::string &name = std::string());

	/**
	 * Returns a a reference at a list of ConnectionDescriptor instances,
	 * describing the connections between the individual populations.
	 *
	 * @return a reference at the internal list of connections stored in the
	 * network instance.
	 */
	const std::vector<ConnectionDescriptor> &connections() const;

	/**
	 * Executes the network on the given backend and stores the results in the
	 * population objects. This function is simply a wrapper for Backend.run().
	 * If there is an error during execution, the run function will throw a
	 * exception.
	 *
	 * @param backend is a reference at the backend instance the network should
	 * be executed on.
	 * @param duration is the simulation-time. If a value smaller or equal to
	 * zero is given, the simulation time is automatically chosen.
	 * @return a reference at this Network instance for simple method chaining.
	 */
	void run(const Backend &backend, float duration = 0.0);

	/**
	 * Returns the duration of the network. The duration is defined as the
	 * last spike stored in a SpikeSourceArray.
	 *
	 * @return the duration of the network in milliseconds.
	 */
	float duration() const;
};
}

#endif /* CYPRESS_CORE_NETWORK_BASE_HPP */
