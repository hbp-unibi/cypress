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

#pragma once

#ifndef CYPRESS_CORE_CONNECTOR_HPP
#define CYPRESS_CORE_CONNECTOR_HPP

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <cypress/core/synapses.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/comperator.hpp>

namespace cypress {
/**
 * Data structure describing a connection between two neurons.
 */
#pragma pack(push, 1)

/**
 * Structure describing the synaptic properties -- weight and delay.
 */
struct Synapse {
	/**
	 * Synaptic weight -- for conductance-based neurons the synaptic weight
	 * is usually measured in micro-siemens, current-based neurons in nano
	 * Ampere. Inhibitory connections are indicated by smaller-than-zero
	 * connection weights.
	 */
	Real weight;

	/**
	 * Synaptic delay in milliseconds.
	 */
	Real delay;

	/**
	 * Constructor of the Synapse structure. Per default initialises all member
	 * variables with zero.
	 *
	 * @param weight weight in micro-Siemens or nano-Ampere
	 * @param delay delay in milliseconds
	 */
	explicit Synapse(Real weight = 0.0, Real delay = 0.0)
	    : weight(weight), delay(delay)
	{
	}

	/**
	 * Returns true if the synapse is excitatory.
	 */
	bool excitatory() const { return weight > 0.0; }

	/**
	 * Returns true if the synapse is inhibitory.
	 */
	bool inhibitory() const { return weight < 0.0; }

	/**
	 * Checks whether the synapse is valid -- invalid synapses are those with
	 * zero weight negative delay.
	 */
	bool valid() const { return (weight != 0.0) && (delay >= 0.0); }
};

/**
 * The LocalConnection class is a stripped-down version of the Connection class
 * which only contains the neuron indices and not the source and target
 * population indices.
 */
struct LocalConnection {
	/**
	 * Source neuron index within the specified source population.
	 */
	NeuronIndex src;

	/**
	 * Target neuron index within the specified target population.
	 */
	NeuronIndex tar;

	/**
	 * Synaptic properties.
	 */
	std::vector<Real> SynapseParameters;

	/**
	 * Constructor of the Connection class. Per default all fields are
	 * initialised with zero.
	 */
	LocalConnection(uint32_t src = 0, uint32_t tar = 0, Real weight = 0.0,
	                Real delay = 0.0)
	    : src(src), tar(tar), SynapseParameters({weight, delay})
	{
	}
	LocalConnection(uint32_t src, uint32_t tar, SynapseBase &synapse)
	    : src(src), tar(tar), SynapseParameters(synapse.parameters())
	{
	}

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() const
	{
		return (SynapseParameters[0] != 0.0) && (SynapseParameters[1] >= 0.0);
	}

	/**
	 * Returns true if the synapse is excitatory.
	 */
	bool excitatory() const { return SynapseParameters[0] > 0; }

	/**
	 * Returns true if the synapse is inhibitory.
	 */
	bool inhibitory() const { return SynapseParameters[0] < 0; }

	/**
	 * Returns a LocalConnection with absolute value of the weight
	 */
	LocalConnection absolute_connection()
	{
		LocalConnection temp = *this;
		temp.SynapseParameters[0] = std::abs(SynapseParameters[0]);
		return temp;
	}
};

struct Connection {
	/**
	 * Source population index.
	 */
	uint32_t psrc;

	/**
	 * Target population index.
	 */
	uint32_t ptar;

	/**
	 * Neuron connection properties.
	 */
	LocalConnection n;

	/**
	 * Constructor of the Connection class. Per default all fields are
	 * initialised with zero.
	 */
	Connection(uint32_t psrc = 0, uint32_t ptar = 0, uint32_t nsrc = 0,
	           uint32_t ntar = 0, Real weight = 0.0, Real delay = 0.0)
	    : psrc(psrc), ptar(ptar), n(nsrc, ntar, weight, delay)
	{
	}
	Connection(uint32_t psrc, uint32_t ptar, uint32_t nsrc, uint32_t ntar,
	           SynapseBase synapse)
	    : psrc(psrc), ptar(ptar), n(nsrc, ntar, synapse)
	{
	}

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() const { return n.valid(); }

	/**
	 * Returns true if the synapse is excitatory.
	 */
	bool excitatory() const { return n.excitatory(); }

	/**
	 * Returns true if the synapse is inhibitory.
	 */
	bool inhibitory() const { return n.inhibitory(); }

	/**
	 * Checks whether two Connection instances are equal.
	 */
	bool operator==(const Connection &s) const
	{
		return Comperator<uint32_t>::equals(1 - uint32_t(valid()),
		                                    1 - uint32_t(s.valid()))(
		           psrc, s.psrc)(ptar, s.ptar)(n.src, s.n.src)(n.tar,
		                                                       s.n.tar)() &&
		       n.SynapseParameters == s.n.SynapseParameters;
	}

	/**
	 * Used to sort the connections by their valid status, their population ids
	 * and their neuron ids. The precedence of these fields is: validity, psrc,
	 * ptar, nsrc, ntar. Invalid neurons are sorted to the end to be able to
	 * simply throw them away.
	 */
	bool operator<(const Connection &s) const
	{
		return Comperator<uint32_t>::smaller(
		    1 - uint32_t(valid()), 1 - uint32_t(s.valid()))(psrc, s.psrc)(
		    ptar, s.ptar)(n.src, s.n.src)(n.tar, s.n.tar)();
	}
};

#pragma pack(pop)

/*
 * Forward declarations.
 */
class ConnectionDescriptor;
class AllToAllConnector;
class OneToOneConnector;
class FromListConnector;

template <typename Callback>
class FunctorConnector;

template <typename Callback>
class UniformFunctorConnector;

template <typename RandomEngine>
class FixedProbabilityConnector;

template <typename RandomEngine>
class RandomConnector;

template <typename RandomEngine>
class FixedFanInConnector;

template <typename RandomEngine>
class FixedFanOutConnector;

/**
 * The abstract Connector class defines the basic interface used to construct
 * connections between populations of neurons. Implementations of this class
 * implement these specific functions to provide such functionality as a
 * one-to-one or an all-to-all connector.
 *
 * This class furthermore provides some static methods which allow to quickly
 * create any desired connector.
 *
 * Implementors of this class must be careful to store all their data inside
 * this class, as the actual "connect" method which creates the connections is
 * deferred until the entire network is serialised.
 */
class Connector {
protected:
	/**
	 * Synapse model
	 */
	std::shared_ptr<SynapseBase> m_synapse;

	/**
	 * This where learned weights are stored (e.g. using STDP)
	 */
	std::vector<LocalConnection> m_weights;

	/**
	 * A additional parameter a connector might use (e.g. probability)
	 */
	Real m_additional_parameter = 0.0;

	/**
	 * Flag for allowing neuron self connections if source and target population
	 * are identical
	 */
	bool m_self_connections = true;

	/**
	 * If a random connector is created with seed, we need this flag to not use
	 * the high level connectors of a backend, but generating connections here
	 * and using a list-connector
	 */
	bool m_seed_given = false;

	/**
	 * Default constructor.
	 */
	explicit Connector(SynapseBase &synapse, bool self_connections)
	{
		m_synapse = SynapseBase::make_shared(synapse);
		m_self_connections = self_connections;
	}
	explicit Connector(std::shared_ptr<SynapseBase> synapse,
	                   bool self_connections)
	{
		m_synapse = std::move(synapse);
		m_self_connections = self_connections;
	}
	Connector(Real weight, Real delay, bool self_connections)
	{
		m_synapse =
		    std::make_shared<StaticSynapse>(StaticSynapse({weight, delay}));
		m_self_connections = self_connections;
	}

public:
	/**
	 * Virtual default destructor.
	 */
	virtual ~Connector() = default;

	/**
	 * Tells the Connector to actually create the neuron-to-neuron connections
	 * between certain neurons.
	 *
	 * @param descr is the connection descriptor containing the data detailing
	 * the connection.
	 * @param tar is the vector the connections should be appended to.
	 */
	virtual void connect(const ConnectionDescriptor &descr,
	                     std::vector<Connection> &tar) const = 0;

	/**
	 * Flag that allows to use the backend connection descriptor instead of a
	 * list connector. Returns true if backend connector should be used (if
	 * available)
	 *
	 * @param descr is the connection descriptor containing the data detailing
	 * the connection.
	 * @return true if connection is valid and makes sense
	 */
	virtual bool group_connect(const ConnectionDescriptor &descr) const = 0;

	/**
	 * Returns true if the connector can create the connections for the given
	 * connection descriptor. While most connectors will always be able to
	 * create a connection, for example a one-to-one connector
	 */
	virtual bool valid(const ConnectionDescriptor &descr) const = 0;

	/**
	 * Flag which either allows (true) or forbids self connections if the source
	 * and target population are the same
	 */
	bool allow_self_connections() const { return m_self_connections; }

	Real additional_parameter() const { return m_additional_parameter; }

	/**
	 * Function which should return a name identifying the connector type --
	 * this name can be used for printing error messages or for visualisation
	 * purposes.
	 *
	 * @return a reference at a string describing the name of the descriptor
	 * type.
	 */
	virtual std::string name() const = 0;

	/**
	 * Returns the number of connections created with this connector and given
	 * pop sizes
	 * @param size_src_pop Size of source population
	 * @param size_target_pop Size of target population
	 * @return number of connections
	 */
	virtual size_t size(size_t size_src_pop, size_t size_target_pop) const = 0;

	/**
	 * Returns a pointer to the underlying synapse object used in the connector
	 * @return Shared-pointer to synapse
	 */
	const std::shared_ptr<SynapseBase> synapse() const { return m_synapse; }

	/**
	 * Directly access the name of the synapse model
	 * @return string containing the name
	 */
	const std::string synapse_name() const { return m_synapse->name(); }

	/**
	 * Returns a vector of (source, target, weight, delay) for learning synapses
	 * The returned structure can be used for a new FromListConnector...
	 */
	const std::vector<LocalConnection> &learned_weights() const
	{
		if (!m_synapse->learning()) {
			throw CypressException(
			    "Requested learned weights, although Synapse is static");
		}
		return m_weights;
	}

	/**
	 * Used internally to store the learned weights
	 *
	 * @param weights matrix of new weights
	 */
	void _store_learned_weights(std::vector<LocalConnection> weights)
	{
		m_weights = std::move(weights);
	}

	/**
	 * Creates an all-to-all connector and returns a pointer at the connector.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<AllToAllConnector> all_to_all(
	    Real weight = 1.0, Real delay = 0.0,
	    bool allow_self_connections = true);
	/**
	 * Creates an all-to-all connector and returns a pointer at the connector.
	 *
	 * @param synapse Synapse object containing model and parameters
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<AllToAllConnector> all_to_all(
	    SynapseBase &synapse, bool allow_self_connections = true);

	/**
	 * Create a a one-to-one connector and returns a pointer at the connector.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<OneToOneConnector> one_to_one(Real weight = 1.0,
	                                                     Real delay = 0.0);

	/**
	 * Creates an one-to-one connector and returns a pointer at the connector.
	 *
	 * @param synapse Synapse object containing model and parameters
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<OneToOneConnector> one_to_one(SynapseBase &synapse);

	/**
	 * Create a list connector which creates connections according to the given
	 * list.
	 *
	 * @param connections is a list of connections that should be created.
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FromListConnector> from_list(
	    const std::vector<LocalConnection> &connections);

	/**
	 * Create a list connector which creates connections according to the given
	 * list.
	 *
	 * @param connections is a list of connections that should be created.
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FromListConnector> from_list(
	    std::initializer_list<LocalConnection> connections);

	/**
	 * Create a list connector which creates connections according to the given
	 * list.
	 *
	 * @param cback is a function which is called for each neuron-pair in the
	 * two populations and which is supposed to return a synapse description. If
	 * the returned synapse is invalid, no connection is created.
	 * @return Unique pointer to the connector
	 */
	template <typename F>
	static std::unique_ptr<FunctorConnector<F>> functor(const F &cback);

	/**
	 * Create a list connector which creates connections according to the given
	 * list.
	 *
	 * @param cback is a function which is called for each neuron-pair in the
	 * two populations and which is supposed to return a synapse description. If
	 * the returned synapse is invalid, no connection is created.
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 * @return Unique pointer to the connector
	 */
	template <typename F>
	static std::unique_ptr<UniformFunctorConnector<F>> functor(
	    const F &cback, Real weight, Real delay = 0.0);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param connector another connector that should be modified by this
	 * connector.
	 * @param p probability with which an connection is created.
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<
	    FixedProbabilityConnector<std::default_random_engine>>
	fixed_probability(std::unique_ptr<Connector> connector, Real p,
	                  bool allow_self_connections = true);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param connector another connector that should be modified by this
	 * connector.
	 * @param p probability with which an connection is created.
	 * @param seed is the seed the random number generator should be initialised
	 * with.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<
	    FixedProbabilityConnector<std::default_random_engine>>
	fixed_probability(std::unique_ptr<Connector> connector, Real p, size_t seed,
	                  bool allow_self_connections = true);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 * @param probability probability with which an connection is created.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<RandomConnector<std::default_random_engine>> random(
	    Real weight = 1, Real delay = 0, Real probability = 1,
	    bool allow_self_connections = true);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param synapse Synapse object containing model and parameters
	 * @param probability probability with which an connection is created.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<RandomConnector<std::default_random_engine>> random(
	    SynapseBase &synapse, Real probability = 1,
	    bool allow_self_connections = true);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 * @param probability probability with which an connection is created.
	 * @param seed is the seed the random number generator should be initialised
	 * with.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<RandomConnector<std::default_random_engine>> random(
	    Real weight, Real delay, Real probability, size_t seed,
	    bool allow_self_connections = true);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param synapse Synapse object containing model and parameters
	 * @param probability probability with which an connection is created.
	 * @param seed is the seed the random number generator should be initialised
	 * with.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<RandomConnector<std::default_random_engine>> random(
	    SynapseBase &synapse, Real probability, size_t seed,
	    bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the source population and
	 * ensures a fixed fan in (number of incomming connections) in each of the
	 * target neurons.
	 *
	 * @param n_fan_in is the fan in that should be ensured.
	 * @param weight is the connection weight.
	 * @param delay is the synaptic delay.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
	fixed_fan_in(size_t n_fan_in, Real weight, Real delay = 0.0,
	             bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the source population and
	 * ensures a fixed fan in (number of incomming connections) in each of the
	 * target neurons.
	 *
	 * @param n_fan_in is the fan in that should be ensured.
	 * @param synapse Synapse object containing model and parameters
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
	fixed_fan_in(size_t n_fan_in, SynapseBase &synapse,
	             bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the source population and
	 * ensures a fixed fan in (number of incomming connections) in each of the
	 * target neurons. Uses the given seed for the initialization of the random
	 * engine.
	 *
	 * @param n_fan_in is the fan in that should be ensured.
	 * @param weight is the connection weight.
	 * @param delay is the synaptic delay.
	 * @param seed is the seed that should be used in order to initialize the
	 * random engine.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
	fixed_fan_in(size_t n_fan_in, Real weight, Real delay, size_t seed,
	             bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the source population and
	 * ensures a fixed fan in (number of incomming connections) in each of the
	 * target neurons. Uses the given seed for the initialization of the random
	 * engine.
	 *
	 * @param n_fan_in is the fan in that should be ensured.
	 * @param synapse Synapse object containing model and parameters
	 * @param seed is the seed that should be used in order to initialize the
	 * random engine.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
	fixed_fan_in(size_t n_fan_in, SynapseBase &synapse, size_t seed,
	             bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the target population and
	 * ensures a fixed fan out (number of outgoing connections) in each of the
	 * source neurons.
	 *
	 * @param n_fan_out is the fan out that should be ensured.
	 * @param weight is the connection weight.
	 * @param delay is the synaptic delay.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
	fixed_fan_out(size_t n_fan_out, Real weight, Real delay = 0.0,
	              bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the target population and
	 * ensures a fixed fan out (number of outgoing connections) in each of the
	 * source neurons.
	 *
	 * @param n_fan_out is the fan out that should be ensured.
	 * @param synapse Synapse object containing model and parameters
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
	fixed_fan_out(size_t n_fan_out, SynapseBase &synapse,
	              bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the target population and
	 * ensures a fixed fan out (number of outgoing connections) in each of the
	 * source neurons. Uses the given seed for the initialization of the random
	 * engine.
	 *
	 * @param n_fan_out is the fan out that should be ensured.
	 * @param weight is the connection weight.
	 * @param delay is the synaptic delay.
	 * @param seed is the seed that should be used in order to initialize the
	 * random engine.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
	fixed_fan_out(size_t n_fan_out, Real weight, Real delay, size_t seed,
	              bool allow_self_connections = true);

	/**
	 * Connector which randomly selects neurons from the target population and
	 * ensures a fixed fan out (number of outgoing connections) in each of the
	 * source neurons. Uses the given seed for the initialization of the random
	 * engine.
	 *
	 * @param n_fan_out is the fan out that should be ensured.
	 * @param synapse Synapse object containing model and parameters
	 * @param seed is the seed that should be used in order to initialize the
	 * random engine.
	 * @param allow_self_connections Flag which either allows (true) or forbids
	 * self connections if the source and target population are the same
	 * @return Unique pointer to the connector
	 */
	static std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
	fixed_fan_out(size_t n_fan_out, SynapseBase &synapse, size_t seed,
	              bool allow_self_connections = true);
};

/**
 * The ConnectionDescriptor represents a user-defined connection between two
 * neuron populations.
 */
class ConnectionDescriptor {
private:
	PopulationIndex m_pid_src;
	NeuronIndex m_nid_src0;
	NeuronIndex m_nid_src1;
	PopulationIndex m_pid_tar;
	NeuronIndex m_nid_tar0;
	NeuronIndex m_nid_tar1;
	std::shared_ptr<Connector> m_connector;

public:
	/**
	 * Constructor of the ConnectionDescriptor class.
	 */
	ConnectionDescriptor(uint32_t pid_src, uint32_t nid_src0, uint32_t nid_src1,
	                     uint32_t pid_tar, uint32_t nid_tar0, uint32_t nid_tar1,
	                     std::shared_ptr<Connector> connector = nullptr)
	    : m_pid_src(pid_src),
	      m_nid_src0(nid_src0),
	      m_nid_src1(nid_src1),
	      m_pid_tar(pid_tar),
	      m_nid_tar0(nid_tar0),
	      m_nid_tar1(nid_tar1),
	      m_connector(std::move(connector))
	{
	}

	/**
	 * The source population id.
	 */
	PopulationIndex pid_src() const { return m_pid_src; }

	/**
	 * Index of the first neuron in the source population involved in the
	 * connection.
	 */
	NeuronIndex nid_src0() const { return m_nid_src0; }

	/**
	 * Index of the first neuron in the target population involved in the
	 * connection.
	 */
	NeuronIndex nid_src1() const { return m_nid_src1; }

	/**
	 * The target population id.
	 */
	PopulationIndex pid_tar() const { return m_pid_tar; }

	/**
	 * Index of the first neuron in the target population involved in the
	 * connection.
	 */
	NeuronIndex nid_tar0() const { return m_nid_tar0; }

	/**
	 * Index of the last-plus-one neuron in the target population involved
	 * in the connection.
	 */
	NeuronIndex nid_tar1() const { return m_nid_tar1; }

	/**
	 * Returns a const reference at the underlying connector instance -- the
	 * class which actually fills the connection table.
	 */
	Connector &connector() const { return *m_connector; }

	/**
	 * Tells the Connector to actually create the neuron-to-neuron connections
	 * between certain neurons.
	 *
	 * @param tar is the target vector to which the connections should be
	 * written.
	 */
	void connect(std::vector<Connection> &tar) const
	{
		connector().connect(*this, tar);
	}

	/**
	 * Returns true if the connector is valid.
	 */
	bool valid() const { return m_connector && connector().valid(*this); }

	/**
	 * Returns the number of neurons in the source population.
	 */
	NeuronIndex nsrc() const { return m_nid_src1 - m_nid_src0; }

	/**
	 * Returns the number of neurons in the target population.
	 */
	NeuronIndex ntar() const { return m_nid_tar1 - m_nid_tar0; }

	/**
	 * Checks whether two Connection instances are equal, apart from the
	 * connector.
	 */
	bool operator==(const ConnectionDescriptor &s) const
	{
		return Comperator<uint32_t>::equals(m_pid_src, s.m_pid_src)(
		    m_pid_tar, s.m_pid_tar)(m_nid_src0, s.m_nid_src0)(
		    m_nid_src1, s.m_nid_src1)(m_nid_tar0, s.m_nid_tar0)(m_nid_tar1,
		                                                        s.m_nid_tar1)();
	}

	/**
	 * Used to sort the connections by their source id.
	 */
	bool operator<(const ConnectionDescriptor &s) const
	{
		return Comperator<uint32_t>::smaller(m_pid_src, s.m_pid_src)(
		    m_pid_tar, s.m_pid_tar)(m_nid_src0, s.m_nid_src0)(
		    m_nid_src1, s.m_nid_src1)(m_nid_tar0, s.m_nid_tar0)(m_nid_tar1,
		                                                        s.m_nid_tar1)();
	}

	size_t size()
	{
		return connector().size(m_nid_src1 - m_nid_src0,
		                        m_nid_tar1 - m_nid_tar0);
	}
};

/**
 * Instantiates the connection descriptors into a flat list of actual
 * connections.
 *
 * @param descrs is a list of connection descriptors which should be turned into
 * a list of connections.
 * @return a flat vector containing the instantiated connections.
 */
std::vector<Connection> instantiate_connections(
    const std::vector<ConnectionDescriptor> &descrs);

bool instantiate_connections_to_file(
    std::string filename, const std::vector<ConnectionDescriptor> &descrs);

/**
 * Abstract base class for connectors with a weight and an delay.
 */
class UniformConnector : public Connector {
public:
	explicit UniformConnector(Real weight = 0.0, Real delay = 0.0,
	                          bool self_connections = true)
	    : Connector(weight, delay, self_connections)
	{
	}
	explicit UniformConnector(SynapseBase &synapse,
	                          bool self_connections = true)
	    : Connector(synapse, self_connections)
	{
	}
};

/**
 * The AllToAllConnector class is used to establish a connection from each
 * neuron in the specified source population to every neuron in the specified
 * target population.
 */
class AllToAllConnector : public UniformConnector {
public:
	using UniformConnector::UniformConnector;

	~AllToAllConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override;

	bool group_connect(const ConnectionDescriptor &) const override
	{
		return true;
	}

	bool valid(const ConnectionDescriptor &) const override { return true; }

	size_t size(size_t size_src_pop, size_t size_target_pop) const override
	{
		return size_src_pop * size_target_pop;
	}

	std::string name() const override { return "AllToAllConnector"; }
};

/**
 * The OneToOneConnector connects pairs of neurons between source and target
 * population. The number of neurons in both populations must be equal.
 */
class OneToOneConnector : public UniformConnector {
public:
	using UniformConnector::UniformConnector;

	~OneToOneConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override;

	bool group_connect(const ConnectionDescriptor &) const override
	{
		return true;
	}

	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc() == descr.ntar();
	}

	size_t size(size_t size_src_pop, size_t) const override
	{
		return size_src_pop;
	}

	std::string name() const override { return "OneToOneConnector"; }
};

/**
 * Connects neurons based on the given list. Note that invalid connections
 * (those lying outside the neuron ranges indicated by the two population views)
 * are silently discarded.
 */
class FromListConnector : public Connector {
private:
	std::vector<LocalConnection> m_connections;

public:
	explicit FromListConnector(std::vector<LocalConnection> connections)
	    : Connector(0.0, 0.0, true), m_connections(std::move(connections))
	{
	}

	FromListConnector(std::initializer_list<LocalConnection> connections)
	    : Connector(0.0, 0.0, true), m_connections(connections)
	{
	}

	~FromListConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override;

	bool group_connect(const ConnectionDescriptor &) const override;

	bool valid(const ConnectionDescriptor &) const override { return true; }

	size_t size(size_t, size_t) const override { return m_connections.size(); }

	std::string name() const override { return "FromListConnector"; }
};

class FunctorConnectorBase : public Connector {
protected:
	FunctorConnectorBase() : Connector(0, 0, true) {}

public:
	std::string name() const override { return "FunctorConnector"; }

	bool group_connect(const ConnectionDescriptor &) const override
	{
		return false;
	}
};

template <typename Callback>
class FunctorConnector : public FunctorConnectorBase {
private:
	Callback m_cback;

public:
	explicit FunctorConnector(const Callback &cback) : m_cback(cback) {}

	~FunctorConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				Synapse synapse = m_cback(n_src, n_tar);
				if (synapse.valid()) {
					tar.emplace_back(descr.pid_src(), descr.pid_tar(), n_src,
					                 n_tar, synapse.weight, synapse.delay);
				}
			}
		}
	}

	bool valid(const ConnectionDescriptor &) const override { return true; }

	size_t size(size_t size_src_pop, size_t size_target_pop) const override
	{
		return size_src_pop * size_target_pop;
	}
};

class UniformFunctorConnectorBase : public UniformConnector {
public:
	using UniformConnector::UniformConnector;

	std::string name() const override { return "UniformFunctorConnector"; }

	bool group_connect(const ConnectionDescriptor &) const override
	{
		return false;
	}

	size_t size(size_t size_src_pop, size_t size_target_pop) const override
	{
		return size_src_pop * size_target_pop;
	}
};

template <typename Callback>
class UniformFunctorConnector : public UniformFunctorConnectorBase {
private:
	Callback m_cback;

public:
	explicit UniformFunctorConnector(const Callback &cback, Real weight = 0.0,
	                                 Real delay = 0.0)
	    : UniformFunctorConnectorBase(weight, delay, true), m_cback(cback)
	{
	}
	UniformFunctorConnector(const Callback &cback, SynapseBase &synapse)
	    : UniformFunctorConnectorBase(synapse, true), m_cback(cback)
	{
	}

	~UniformFunctorConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				if (m_cback(n_src, n_tar)) {
					tar.emplace_back(descr.pid_src(), descr.pid_tar(), n_src,
					                 n_tar, *m_synapse);
				}
			}
		}
	}

	bool valid(const ConnectionDescriptor &) const override { return true; }
};

class FixedProbabilityConnectorBase : public Connector {
protected:
	std::string name_string = "FixedProbabilityConnector";
	FixedProbabilityConnectorBase(std::shared_ptr<SynapseBase> synapse,
	                              bool self_connections)
	    : Connector(std::move(synapse), self_connections)
	{
	}

public:
	std::string name() const override { return name_string; }

	/**
	 * Internally used to switch off backend connectors and use lists generated
	 * with the seed and random engine provided
	 */
	void seed_given() { m_seed_given = true; }
};

template <typename RandomEngine>
class FixedProbabilityConnector : public FixedProbabilityConnectorBase {
protected:
	std::unique_ptr<Connector> m_connector;
	std::shared_ptr<RandomEngine> m_engine;

public:
	FixedProbabilityConnector(std::unique_ptr<Connector> connector, Real p,
	                          std::shared_ptr<RandomEngine> engine,
	                          bool self_connections)
	    : FixedProbabilityConnectorBase(connector->synapse(), self_connections),
	      m_connector(std::move(connector)),
	      m_engine(std::move(engine))
	{
		m_additional_parameter = p;
	}

	~FixedProbabilityConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		const size_t first = tar.size();   // Old number of connections
		m_connector->connect(descr, tar);  // Instantiate the connections
		std::uniform_real_distribution<Real> distr(0.0, 1.0);
		for (size_t i = first; i < tar.size(); i++) {
			if (distr(*m_engine) >= m_additional_parameter) {
				tar[i].n.SynapseParameters[0] =
				    0.0;  // Invalidate the connection
			}
		}

		if ((!descr.connector().allow_self_connections()) &&
		    descr.pid_src() == descr.pid_tar()) {
			for (size_t i = first; i < tar.size(); i++) {
				if (tar[i].n.src == tar[i].n.tar) {
					tar[i].n.SynapseParameters[0] =
					    0.0;  // Invalidate the connection
				}
			}
		}
	}

	bool group_connect(const ConnectionDescriptor &) const override
	{
		return false;
	}

	bool valid(const ConnectionDescriptor &descr) const override
	{
		return m_connector->valid(descr);
	}

	size_t size(size_t size_src_pop, size_t size_target_pop) const override
	{
		return size_t(Real(size_src_pop * size_target_pop) *
		              m_additional_parameter);
	}
};

/**
 * Establishes a random connection between populations. A possible connection
 * between two neurons exists with given probability @probability.
 */
template <typename RandomEngine>
class RandomConnector : public FixedProbabilityConnector<RandomEngine> {
private:
	using Base = FixedProbabilityConnector<RandomEngine>;

public:
	RandomConnector(Real weight, Real delay, Real probability,
	                std::shared_ptr<RandomEngine> engine, bool self_connections)
	    : FixedProbabilityConnector<RandomEngine>(
	          Connector::all_to_all(weight, delay), probability, engine,
	          self_connections)
	{
		FixedProbabilityConnectorBase::name_string = "RandomConnector";
	}
	RandomConnector(SynapseBase &synapse, Real probability,
	                std::shared_ptr<RandomEngine> engine, bool self_connections)
	    : FixedProbabilityConnector<RandomEngine>(
	          Connector::all_to_all(synapse), probability, engine,
	          self_connections)
	{
		FixedProbabilityConnectorBase::name_string = "RandomConnector";
	}

	bool group_connect(const ConnectionDescriptor &) const override
	{
		if (Base::m_seed_given) {
			return false;
		}
		return true;
	}

	std::string name() const override { return "RandomConnector"; }
};

/**
 * Base class for both the FixedFanInConnector and FixedFanOutConnector classes.
 * Provides a protected member function for the generation of the random
 * permutations of input/output neuron indices.
 */
template <typename RandomEngine>
class FixedFanConnectorBase : public UniformConnector {
private:
	std::shared_ptr<RandomEngine> m_engine;

protected:
	template <typename F>
	void generate_connections(size_t offs, size_t len, size_t subset_len,
	                          size_t i0, size_t i1, const F &f, bool self) const
	{
		// Initialize a vector with the identity permutation
		std::vector<size_t> perm(len);
		for (size_t i = 0; i < len; i++) {
			perm[i] = offs + i;
		}

		// Iterate over each target neuron, create a new permutation containing
		// the source neuron indices and instantiate the connection
		std::uniform_int_distribution<size_t> distr(0, perm.size() - 1);
		for (size_t i = i0; i < i1; i++) {
			// Generate a new permutation for the first subset_len elements
			for (size_t j = 0; j < subset_len; j++) {
				std::swap(perm[j], perm[distr(*m_engine)]);
			}
			// Create the connections
			if (self) {
				for (size_t j = 0; j < subset_len; j++) {
					f(i, perm[j]);
				}
			}
			else {
				size_t sub_len = subset_len;
				for (size_t j = 0; j < sub_len; j++) {
					if (i != perm[j]) {
						f(i, perm[j]);
					}
					else {
						sub_len = std::min(sub_len + 1, perm.size());
					}
				}
			}
		}
	}

public:
	FixedFanConnectorBase(Real weight, Real delay,
	                      std::shared_ptr<RandomEngine> engine,
	                      bool self_connections)
	    : UniformConnector(weight, delay, self_connections),
	      m_engine(std::move(engine))
	{
	}
	FixedFanConnectorBase(SynapseBase &synapse,
	                      std::shared_ptr<RandomEngine> engine,
	                      bool self_connections)
	    : UniformConnector(synapse, self_connections),
	      m_engine(std::move(engine))
	{
	}

	/**
	 * Internally used to switch off backend connectors and use lists generated
	 * with the seed and random engine provided
	 */
	void seed_given() { m_seed_given = true; }
};

/**
 * Connects each neuron in the target population with a fixed number of neurons
 * in the source population.
 */
template <typename RandomEngine>
class FixedFanInConnector : public FixedFanConnectorBase<RandomEngine> {
private:
	using Base = FixedFanConnectorBase<RandomEngine>;

public:
	FixedFanInConnector(size_t n_fan_in, Real weight, Real delay,
	                    std::shared_ptr<RandomEngine> engine,
	                    bool self_connections)
	    : FixedFanConnectorBase<RandomEngine>(weight, delay, std::move(engine),
	                                          self_connections)
	{
		Base::m_additional_parameter = n_fan_in;
	}
	FixedFanInConnector(size_t n_fan_in, SynapseBase &synapse,
	                    std::shared_ptr<RandomEngine> engine,
	                    bool self_connections)
	    : FixedFanConnectorBase<RandomEngine>(synapse, std::move(engine),
	                                          self_connections)
	{
		Base::m_additional_parameter = n_fan_in;
	}

	~FixedFanInConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		if (descr.pid_src() == descr.pid_tar()) {
			Base::generate_connections(
			    descr.nid_src0(), descr.nsrc(), Base::m_additional_parameter,
			    descr.nid_tar0(), descr.nid_tar1(),
			    [&](size_t i, size_t r) {
				    tar.emplace_back(descr.pid_src(), descr.pid_tar(), r, i,
				                     *this->m_synapse);
			    },
			    descr.connector().allow_self_connections());
		}
		else {
			Base::generate_connections(
			    descr.nid_src0(), descr.nsrc(), Base::m_additional_parameter,
			    descr.nid_tar0(), descr.nid_tar1(),
			    [&](size_t i, size_t r) {
				    tar.emplace_back(descr.pid_src(), descr.pid_tar(), r, i,
				                     *this->m_synapse);
			    },
			    true);
		}
	}

	bool group_connect(const ConnectionDescriptor &) const override
	{
		if (Base::m_seed_given) {
			return false;
		}
		return true;
	};

	/**
	 * Checks the validity of the connection. For the FixedFanInConnector to be
	 * valid, the number of neurons in the source population must be at least
	 * equal to the fan in.
	 */
	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc() >= NeuronIndex(Base::m_additional_parameter);
	}

	std::string name() const override { return "FixedFanInConnector"; }

	size_t size(size_t, size_t size_target_pop) const override
	{
		return Base::m_additional_parameter * size_target_pop;
	}
};

/**
 * Connects each neuron in the source population to a fixed number of neurons
 * in the target population.
 */
template <typename RandomEngine>
class FixedFanOutConnector : public FixedFanConnectorBase<RandomEngine> {
private:
	using Base = FixedFanConnectorBase<RandomEngine>;

public:
	FixedFanOutConnector(size_t n_fan_out, Real weight, Real delay,
	                     std::shared_ptr<RandomEngine> engine,
	                     bool self_connections)
	    : Base(weight, delay, std::move(engine), self_connections)
	{
		Base::m_additional_parameter = n_fan_out;
	}

	FixedFanOutConnector(size_t n_fan_out, SynapseBase &synapse,
	                     std::shared_ptr<RandomEngine> engine,
	                     bool self_connections)
	    : Base(synapse, std::move(engine), self_connections)
	{
		Base::m_additional_parameter = n_fan_out;
	}

	~FixedFanOutConnector() override = default;

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		if (descr.pid_src() == descr.pid_tar()) {
			Base::generate_connections(
			    descr.nid_tar0(), descr.ntar(), Base::m_additional_parameter,
			    descr.nid_src0(), descr.nid_src1(),
			    [&](size_t i, size_t r) {
				    tar.emplace_back(descr.pid_src(), descr.pid_tar(), i, r,
				                     *this->m_synapse);
			    },
			    descr.connector().allow_self_connections());
		}
		else {
			Base::generate_connections(
			    descr.nid_tar0(), descr.ntar(), Base::m_additional_parameter,
			    descr.nid_src0(), descr.nid_src1(),
			    [&](size_t i, size_t r) {
				    tar.emplace_back(descr.pid_src(), descr.pid_tar(), i, r,
				                     *this->m_synapse);
			    },
			    true);
		}
	}

	bool group_connect(const ConnectionDescriptor &) const override
	{
		return false;
		/*if (Base::m_seed_given) {
			return false;
		}
		return true;*/
	};

	/**
	 * Checks the validity of the connection. For the FixedFanOutConnector,
	 * the number of neurons in the target population must be at least equal to
	 * the fan out.
	 */
	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.ntar() >= NeuronIndex(Base::m_additional_parameter);
	}

	std::string name() const override { return "FixedFanOutConnector"; }

	size_t size(size_t size_src_pop, size_t) const override
	{
		return size_src_pop * Base::m_additional_parameter;
	}
};

/**
 * Inline methods
 */

inline std::unique_ptr<AllToAllConnector> Connector::all_to_all(
    Real weight, Real delay, bool self_connections)
{
	return std::make_unique<AllToAllConnector>(weight, delay, self_connections);
}
inline std::unique_ptr<AllToAllConnector> Connector::all_to_all(
    SynapseBase &synapse, bool self_connections)
{
	return std::make_unique<AllToAllConnector>(synapse, self_connections);
}

inline std::unique_ptr<OneToOneConnector> Connector::one_to_one(Real weight,
                                                                Real delay)
{
	return std::make_unique<OneToOneConnector>(weight, delay);
}
inline std::unique_ptr<OneToOneConnector> Connector::one_to_one(
    SynapseBase &synapse)
{
	return std::make_unique<OneToOneConnector>(synapse);
}

inline std::unique_ptr<FromListConnector> Connector::from_list(
    const std::vector<LocalConnection> &connections)
{
	return std::make_unique<FromListConnector>(connections);
}

inline std::unique_ptr<FromListConnector> Connector::from_list(
    std::initializer_list<LocalConnection> connections)
{
	return std::make_unique<FromListConnector>(connections);
}

template <typename F>
inline std::unique_ptr<FunctorConnector<F>> Connector::functor(const F &cback)
{
	return std::make_unique<FunctorConnector<F>>(cback);
}

template <typename F>
inline std::unique_ptr<UniformFunctorConnector<F>> Connector::functor(
    const F &cback, Real weight, Real delay)
{
	return std::make_unique<UniformFunctorConnector<F>>(cback, weight, delay);
}
/*template <typename F>
inline std::unique_ptr<UniformFunctorConnector<F>> Connector::functor(
    const F &cback, SynapseBase& synapse)
{
    return std::make_unique<UniformFunctorConnector<F>>(cback, synapse);
}*/

inline std::unique_ptr<FixedProbabilityConnector<std::default_random_engine>>
Connector::fixed_probability(std::unique_ptr<Connector> connector, Real p,
                             bool self_connections)
{
	return fixed_probability(std::move(connector), p, std::random_device()(),
	                         self_connections);
}

inline std::unique_ptr<FixedProbabilityConnector<std::default_random_engine>>
Connector::fixed_probability(std::unique_ptr<Connector> connector, Real p,
                             size_t seed, bool self_connections)
{
	auto tmp =
	    std::make_unique<FixedProbabilityConnector<std::default_random_engine>>(
	        std::move(connector), p,
	        std::make_shared<std::default_random_engine>(seed),
	        self_connections);
	tmp->seed_given();
	return tmp;
}

inline std::unique_ptr<RandomConnector<std::default_random_engine>>
Connector::random(Real weight, Real delay, Real probability,
                  bool self_connections)
{
	return random(weight, delay, probability, std::random_device()(),
	              self_connections);
}

inline std::unique_ptr<RandomConnector<std::default_random_engine>>
Connector::random(SynapseBase &synapse, Real probability, bool self_connections)
{
	return random(synapse, probability, std::random_device()(),
	              self_connections);
}

inline std::unique_ptr<RandomConnector<std::default_random_engine>>
Connector::random(Real weight, Real delay, Real probability, size_t seed,
                  bool self_connections)
{
	auto tmp = std::make_unique<RandomConnector<std::default_random_engine>>(
	    weight, delay, probability,
	    std::make_shared<std::default_random_engine>(seed), self_connections);
	tmp->seed_given();
	return tmp;
}
inline std::unique_ptr<RandomConnector<std::default_random_engine>>
Connector::random(SynapseBase &synapse, Real probability, size_t seed,
                  bool self_connections)
{
	auto tmp = std::make_unique<RandomConnector<std::default_random_engine>>(
	    synapse, probability,
	    std::make_shared<std::default_random_engine>(seed), self_connections);
	tmp->seed_given();
	return tmp;
}

inline std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
Connector::fixed_fan_in(size_t n_fan_in, Real weight, Real delay,
                        bool self_connections)
{
	return fixed_fan_in(n_fan_in, weight, delay, std::random_device()(),
	                    self_connections);
}
inline std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
Connector::fixed_fan_in(size_t n_fan_in, SynapseBase &synapse,
                        bool self_connections)
{
	return fixed_fan_in(n_fan_in, synapse, std::random_device()(),
	                    self_connections);
}

inline std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
Connector::fixed_fan_in(size_t n_fan_in, Real weight, Real delay, size_t seed,
                        bool self_connections)
{
	auto tmp =
	    std::make_unique<FixedFanInConnector<std::default_random_engine>>(
	        n_fan_in, weight, delay,
	        std::make_shared<std::default_random_engine>(seed),
	        self_connections);
	tmp->seed_given();
	return tmp;
}
inline std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
Connector::fixed_fan_in(size_t n_fan_in, SynapseBase &synapse, size_t seed,
                        bool self_connections)
{
	auto tmp =
	    std::make_unique<FixedFanInConnector<std::default_random_engine>>(
	        n_fan_in, synapse,
	        std::make_shared<std::default_random_engine>(seed),
	        self_connections);
	tmp->seed_given();
	return tmp;
}

inline std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
Connector::fixed_fan_out(size_t n_fan_out, Real weight, Real delay,
                         bool self_connections)
{
	return fixed_fan_out(n_fan_out, weight, delay, std::random_device()(),
	                     self_connections);
}
inline std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
Connector::fixed_fan_out(size_t n_fan_out, SynapseBase &synapse,
                         bool self_connections)
{
	return fixed_fan_out(n_fan_out, synapse, std::random_device()(),
	                     self_connections);
}

inline std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
Connector::fixed_fan_out(size_t n_fan_out, Real weight, Real delay, size_t seed,
                         bool self_connections)
{
	auto tmp =
	    std::make_unique<FixedFanOutConnector<std::default_random_engine>>(
	        n_fan_out, weight, delay,
	        std::make_shared<std::default_random_engine>(seed),
	        self_connections);
	tmp->seed_given();
	return tmp;
}
inline std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
Connector::fixed_fan_out(size_t n_fan_out, SynapseBase &synapse, size_t seed,
                         bool self_connections)
{
	auto tmp =
	    std::make_unique<FixedFanOutConnector<std::default_random_engine>>(
	        n_fan_out, synapse,
	        std::make_shared<std::default_random_engine>(seed),
	        self_connections);
	tmp->seed_given();
	return tmp;
}
}  // namespace cypress

#endif /* CYPRESS_CORE_CONNECTOR_HPP */
