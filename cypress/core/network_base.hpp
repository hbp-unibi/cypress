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
#include <unordered_set>
#include <vector>

#include <cypress/core/connector.hpp>
#include <cypress/core/data.hpp>
#include <cypress/core/neurons_base.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/json.hpp>

namespace cypress {

/*
 * Forward declarations
 */
namespace internal {
class NetworkData;
}

class Backend;
class NetworkBase;
class PopulationBase;
class PopulationViewBase;
class NeuronBase;
class Logger;

template <typename Impl, typename Accessor>
class PopulationMixin;

template <typename Impl, typename Accessor, typename Params, typename Signals>
class DataMixin;

template <typename Impl, typename Accessor>
class ConnectableMixin;

/**
 * Structure storing information about the network runtime.
 */
struct NetworkRuntime {
	/**
	 * Total runtime -- sum of the three other times.
	 */
	Real total;

	/**
	 * Raw simulation time. This may include setup and finalization time
	 * required by the hardware timer.
	 */
	Real sim;

	/**
	 * Initialization time. Time required to construct the network.
	 */
	Real initialize;

	/**
	 * Finalization time. Time required to retrieve the results from the
	 * driver.
	 */
	Real finalize;
};

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
	std::shared_ptr<internal::NetworkData> m_impl;

protected:
	template <typename Impl, typename Accessor>
	friend class PopulationMixin;

	template <typename Impl, typename Accessor, typename Params,
	          typename Signals>
	friend class DataMixin;

	template <typename Impl, typename Accessor>
	friend class ConnectableMixin;

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
	 * @param params contains the initial neuron parameters. The parameters
	 * may be a list of neuron parameters, in which case the size of the list
	 * must be equal to the size of the population.
	 * @param signals contains the initial flags describing which signals should
	 * be recorded. The signals object may contain a list of signal instances,
	 * in which case the size of the list must be equal to the size of the
	 * population.
	 * @param name is the name of the population.
	 */
	PopulationIndex create_population_index(size_t size, const NeuronType &type,
	                                        const NeuronParameters &params,
	                                        const NeuronSignals &signals,
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
	NetworkBase(std::shared_ptr<internal::NetworkData> impl) : m_impl(impl) {}

	/**
	 * Destructor. Destroys this network instance and the handle pointing at
	 * underlying network.
	 */
	~NetworkBase();

	/**
	 * Returns the logger that should be used to log messages related to this
	 * network. The "global_logger()" is used per default.
	 */
	Logger &logger() const;

	/**
	 * Sets a new logger that should be used to log messages related to this
	 * network.
	 */
	void logger(Logger &logger);

	/**
	 * Returns the raw population data associated with the given population
	 * index. Try to avoid using this function directly. Proper use cases are
	 * network transformations.
	 *
	 * @param pid is the index of the population for which a reference at the
	 * internal data should be returned.
	 */
	std::shared_ptr<PopulationData> population_data(PopulationIndex pid);

	/**
	 * Returns the raw population data associated with the given population
	 * index. Try to avoid using this function directly. Proper use cases are
	 * network transformations.
	 *
	 * @param pid is the index of the population for which a reference at the
	 * internal data should be returned.
	 */
	std::shared_ptr<const PopulationData> population_data(
	    PopulationIndex pid) const;

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
	 * Returns the number of populations of a certain neuron type.
	 */
	size_t population_count(const NeuronType &type) const;

	/**
	 * Returns the number of populations of a certain neuron type.
	 */
	template <typename T>
	size_t population_count() const
	{
		return population_count(T::inst());
	}

	/**
	 * Returns the total number of neurons in the network.
	 */
	size_t neuron_count() const;

	/**
	 * Returns the number of neurons of a given type.
	 */
	size_t neuron_count(const NeuronType &type) const;

	/**
	 * Returns the number of neurons of a certain neuron type.
	 */
	template <typename T>
	size_t neuron_count() const
	{
		return neuron_count(T::inst());
	}

	/**
	 * Returns a PopulationBase instance pointing at the population with the
	 * given index.
	 */
	PopulationBase population(PopulationIndex pid);

	const PopulationBase population(PopulationIndex pid) const;

	PopulationBase operator[](PopulationIndex pid);

	const PopulationBase operator[](PopulationIndex pid) const;

	/**
	 * Returns a list of populations available in the network, filtered by the
	 * given name and type.
	 *
	 * @param name is the name the returned populations should be filtered by.
	 * If an empty string is given, no filtering is performed (default).
	 * @param type is the neuron type the list should be filtered for. If
	 * a pointer at the special NullNeuron instance is given, no filtering
	 * by type is performed (default).
	 * @return a list of PopulationBase instances, pointing at the populations
	 * matching the given requirements.
	 */
	const std::vector<PopulationBase> populations(
	    const std::string &name = std::string(),
	    const NeuronType &type = NullNeuron::inst()) const;

	/**
	 * Returns a list of populations filtered by the given neuron type.
	 *
	 * @param type is the neuron type the list should be filtered for. If
	 * a pointer at the special NullNeuron instance is given, no filtering
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
	 * Returns the backend instance for the given backend string. The backend
	 * string may be one of the following:
	 *
	 * - nest: executes the network using the native nest backend
	 * - pynn.*: executes the network using the given PyNN backend
	 * - nmpi.*: executes the network using NMPI and the given PyNN backend.
	 * - nmpi.pynn.*: same as above.
	 *
	 * @param backend_id is the id of the backend that should be created.
	 * @param argc is the number of command line arguments. Needed when the NMPI
	 * backend should be used.
	 * @param argv is the array containing the command line arguments. Needed
	 * when the NMPI backend should be used.
	 * @param setup is an optional Json object which contains backend
	 * configuration.
	 * @return a pointer at a new backend instance.
	 */
	static std::unique_ptr<Backend> make_backend(std::string backend_id,
	                                             int argc = 0,
	                                             const char *argv[] = nullptr,
	                                             Json setup = Json());

	/**
	 * Returns the state of the "use_lossy_trafos" flag. If true, applies
	 * possibly lossy transformations to the network in order to run it on the
	 * target platform. The default value is "true".
	 */
	bool use_lossy_trafos() const;

	/**
	 * Sets the "use_lossy_trafos" flag. If true, allows to apply lossy
	 * transformations to allow running a network on the target platform. The
	 * defautl value is "true".
	 */
	void use_lossy_trafos(bool use_lossy) const;

	/**
	 * Returns the list of currently disabled transformation ids.
	 */
	const std::unordered_set<std::string> &disabled_trafo_ids() const;

	/**
	 * Returns the list of currently disabled transformation ids.
	 */
	std::unordered_set<std::string> &disabled_trafo_ids();

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
	 */
	void run(const Backend &backend, Real duration = 0.0);

	/**
	 * Executes the network on the given backend and stores the results in the
	 * population objects. This function is simply a wrapper for Backend.run().
	 * If there is an error during execution, the run function will throw a
	 * exception.
	 *
	 * @param backend_id is a string describing the backend instance. See
	 * make_backend() for more information on available backend strings.
	 * @param argc is the number of command line arguments. Needed when the NMPI
	 * backend should be used.
	 * @param argv is the array containing the command line arguments. Needed
	 * when the NMPI backend should be used.
	 * @param duration is the simulation-time. If a value smaller or equal to
	 * zero is given, the simulation time is automatically chosen.
	 */
	void run(const std::string &backend_id, Real duration = 0.0, int argc = 0,
	         const char *argv[] = nullptr);

	/**
	 * Returns the duration of the network. The duration is defined as the
	 * last spike stored in a SpikeSourceArray.
	 *
	 * @return the duration of the network in milliseconds.
	 */
	Real duration() const;

	/**
	 * Returns information about the network runtime. Behaviour is undefined if
	 * the network has not yet been executed.
	 */
	NetworkRuntime runtime() const;

	/**
	 * Allows to set the runtime information to the given values.
	 */
	void runtime(const NetworkRuntime &runtime);
};
}

#endif /* CYPRESS_CORE_NETWORK_BASE_HPP */
