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

#include <memory>
#include <random>
#include <string>
#include <vector>

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
	 * is usually measured in micro-siemens. Inhibitory connections are
	 * indicated by smaller-than-zero connection weights.
	 */
	Real weight;

	/**
	 * Synaptic delay in milliseconds.
	 */
	Real delay;

	/**
	 * Constructor of the Synapse structure. Per default initialises all member
	 * variables with zero.
	 */
	Synapse(Real weight = 0.0, Real delay = 0.0) : weight(weight), delay(delay)
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
	Synapse synapse;

	/**
	 * Constructor of the Connection class. Per default all fields are
	 * initialised with zero.
	 */
	LocalConnection(uint32_t src = 0, uint32_t tar = 0, Real weight = 0.0,
	                Real delay = 0.0)
	    : src(src), tar(tar), synapse(weight, delay)
	{
	}

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() const { return synapse.valid(); }

	/**
	 * Returns true if the synapse is excitatory.
	 */
	bool excitatory() const { return synapse.excitatory(); }

	/**
	 * Returns true if the synapse is inhibitory.
	 */
	bool inhibitory() const { return synapse.inhibitory(); }
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

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() const { return n.synapse.valid(); }

	/**
	 * Returns true if the synapse is excitatory.
	 */
	bool excitatory() const { return n.synapse.excitatory(); }

	/**
	 * Returns true if the synapse is inhibitory.
	 */
	bool inhibitory() const { return n.synapse.inhibitory(); }

	/**
	 * Checks whether two Connection instances are equal.
	 */
	bool operator==(const Connection &s) const
	{
		return Comperator<uint32_t>::equals(
		    1 - uint32_t(valid()), 1 - uint32_t(s.valid()))(psrc, s.psrc)(
		    ptar, s.ptar)(n.synapse.weight < 0.0, s.n.synapse.weight < 0.0)(
		    n.src, s.n.src)(n.tar, s.n.tar)();
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
		    ptar, s.ptar)(n.synapse.weight < 0.0, s.n.synapse.weight < 0.0)(
		    n.src, s.n.src)(n.tar, s.n.tar)();
	}
};

/**
 * For some connectors and backends it may not be necessary to list all
 * connections, but to describe the connector
 */
struct GroupConnction {
	/**
	 * Source population index.
	 */
	uint32_t psrc;

	/**
	 * Target population index.
	 */
	uint32_t ptar;

	/**
	 * Index of the first neuron in the source population involved in the
	 * connection.
	 */
	NeuronIndex src0;

	/**
	 * Index of the last neuron in the source population involved in the
	 * connection.
	 */
	NeuronIndex src1;

	/**
	 * Index of the first neuron in the target population involved in the
	 * connection.
	 */
	NeuronIndex tar0;

	/**
	 * Index of the last neuron in the target population involved in the
	 * connection.
	 */
	NeuronIndex tar1;

	/**
	 * Synaptic properties.
	 */
	Synapse synapse;

	/**
	 * Addtional Parameter storing e.g. connection probability or number of
	 * neurons (only for certain Connectors)
	 */
	Real additional_parameter;

	/**
	 * Flag stating wether this is a valid group connection
	 */
	// bool valid_group = false;

	std::string connection_name = "invalid";

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() const
	{
		return synapse.valid() && connection_name != "invalid";
	}

	/**
	 * Returns true if the synapse is excitatory.
	 */
	bool excitatory() const { return synapse.excitatory(); }

	/**
	 * Returns true if the synapse is inhibitory.
	 */
	bool inhibitory() const { return synapse.inhibitory(); }
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
public:
	/**
	 * Default constructor.
	 */
	Connector() {}

	/**
	 * Virtual default destructor.
	 */
	virtual ~Connector() {}

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
	 * Tells the Connector to actually create the neuron-to-neuron connections
	 * between certain neurons in a compact group representation
	 *
	 * @param descr is the connection descriptor containing the data detailing
	 * the connection.
	 * @param tar is the group connection.
	 * @return true if connection is valid and makes sense
	 */
	virtual bool group_connect(const ConnectionDescriptor &descr,
	                           GroupConnction &tar) const = 0;

	/**
	 * Returns true if the connector can create the connections for the given
	 * connection descriptor. While most connectors will always be able to
	 * create a connection, for example a one-to-one connector
	 */
	virtual bool valid(const ConnectionDescriptor &descr) const = 0;

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
	 * Creates an all-to-all connector and returns a pointer at the connector.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 */
	static std::unique_ptr<AllToAllConnector> all_to_all(Real weight = 1.0,
	                                                     Real delay = 0.0);

	/**
	 * Createa a one-to-one connector and returns a pointer at the connector.
	 */
	static std::unique_ptr<OneToOneConnector> one_to_one(Real weight = 1.0,
	                                                     Real delay = 0.0);

	/**
	 * Create a list connector which creates connections according to the given
	 * list.
	 *
	 * @param connections is a list of connections that should be created.
	 */
	static std::unique_ptr<FromListConnector> from_list(
	    const std::vector<LocalConnection> &connections);

	/**
	 * Create a list connector which creates connections according to the given
	 * list.
	 *
	 * @param connections is a list of connections that should be created.
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
	 */
	static std::unique_ptr<
	    FixedProbabilityConnector<std::default_random_engine>>
	fixed_probability(std::unique_ptr<Connector> connector, Real p = 1.0);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param connector another connector that should be modified by this
	 * connector.
	 * @param p probability with which an connection is created.
	 * @param seed is the seed the random number generator should be initialised
	 * with.
	 */
	static std::unique_ptr<
	    FixedProbabilityConnector<std::default_random_engine>>
	fixed_probability(std::unique_ptr<Connector> connector, Real p,
	                  size_t seed);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 * @param probability probability with which an connection is created.
	 */
	static std::unique_ptr<RandomConnector<std::default_random_engine>> random(
	    Real weight = 1, Real delay = 0, Real probability = 1);

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
	 */
	static std::unique_ptr<RandomConnector<std::default_random_engine>> random(
	    Real weight, Real delay, Real probability, size_t seed);

	/**
	 * Connector which randomly selects neurons from the source population and
	 * ensures a fixed fan in (number of incomming connections) in each of the
	 * target neurons.
	 *
	 * @param n_fan_in is the fan in that should be ensured.
	 * @param weight is the connection weight.
	 * @param delay is the synaptic delay.
	 */
	static std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
	fixed_fan_in(size_t n_fan_in, Real weight, Real delay = 0.0);

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
	 */
	static std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
	fixed_fan_in(size_t n_fan_in, Real weight, Real delay, size_t seed);

	/**
	 * Connector which randomly selects neurons from the target population and
	 * ensures a fixed fan out (number of outgoing connections) in each of the
	 * source neurons.
	 *
	 * @param n_fan_in is the fan in that should be ensured.
	 * @param weight is the connection weight.
	 * @param delay is the synaptic delay.
	 */
	static std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
	fixed_fan_out(size_t n_fan_out, Real weight, Real delay = 0.0);

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
	 */
	static std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
	fixed_fan_out(size_t n_fan_out, Real weight, Real delay, size_t seed);
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
	      m_connector(connector)
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
	const Connector &connector() const { return *m_connector; }

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
	size_t nsrc() const { return m_nid_src1 - m_nid_src0; }

	/**
	 * Returns the number of neurons in the target population.
	 */
	size_t ntar() const { return m_nid_tar1 - m_nid_tar0; }

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

/**
 * Abstract base class for connectors with a weight and an delay.
 */
class UniformConnector : public Connector {
protected:
	Real m_weight;
	Real m_delay;

public:
	UniformConnector(Real weight = 0.0, Real delay = 0.0)
	    : m_weight(weight), m_delay(delay)
	{
	}

	Real weight() const { return m_weight; }
	Real delay() const { return m_delay; }
};

/**
 * The AllToAllConnector class is used to establish a connection from each
 * neuron in the specified source population to every neuron in the specified
 * target population.
 */
class AllToAllConnector : public UniformConnector {
public:
	using UniformConnector::UniformConnector;

	~AllToAllConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override;

	bool group_connect(const ConnectionDescriptor &descr,
	                   GroupConnction &tar) const override;

	bool valid(const ConnectionDescriptor &) const override { return true; }

	std::string name() const override { return "AllToAllConnector"; }
};

/**
 * The OneToOneConnector connects pairs of neurons between source and target
 * population. The number of neurons in both populations must be equal.
 */
class OneToOneConnector : public UniformConnector {
public:
	using UniformConnector::UniformConnector;

	~OneToOneConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override;

	bool group_connect(const ConnectionDescriptor &descr,
	                   GroupConnction &tar) const override;

	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc() == descr.ntar();
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
	FromListConnector(const std::vector<LocalConnection> &connections)
	    : m_connections(connections)
	{
	}

	FromListConnector(std::initializer_list<LocalConnection> connections)
	    : m_connections(connections)
	{
	}

	~FromListConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override;

	bool group_connect(const ConnectionDescriptor &,
	                   GroupConnction &tar) const override;

	bool valid(const ConnectionDescriptor &) const override { return true; }

	std::string name() const override { return "FromListConnector"; }
};

class FunctorConnectorBase : public Connector {
public:
	std::string name() const override { return "FunctorConnector"; }

	bool group_connect(const ConnectionDescriptor &,
	                   GroupConnction &) const override
	{
		return false;
	}
};

template <typename Callback>
class FunctorConnector : public FunctorConnectorBase {
private:
	Callback m_cback;

public:
	FunctorConnector(const Callback &cback) : m_cback(cback) {}

	~FunctorConnector() override {}

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
};

class UniformFunctorConnectorBase : public UniformConnector {
public:
	using UniformConnector::UniformConnector;

	std::string name() const override { return "UniformFunctorConnector"; }

	bool group_connect(const ConnectionDescriptor &,
	                   GroupConnction &) const override
	{
		return false;
	}
};

template <typename Callback>
class UniformFunctorConnector : public UniformFunctorConnectorBase {
private:
	Callback m_cback;

public:
	UniformFunctorConnector(const Callback &cback, Real weight = 0.0,
	                        Real delay = 0.0)
	    : UniformFunctorConnectorBase(weight, delay), m_cback(cback)
	{
	}

	~UniformFunctorConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				if (m_cback(n_src, n_tar)) {
					tar.emplace_back(descr.pid_src(), descr.pid_tar(), n_src,
					                 n_tar, weight(), delay());
				}
			}
		}
	}

	bool valid(const ConnectionDescriptor &) const override { return true; }
};

class FixedProbabilityConnectorBase : public Connector {
protected:
	std::string name_string = "FixedProbabilityConnector";

public:
	std::string name() const override { return name_string; }
};

template <typename RandomEngine>
class FixedProbabilityConnector : public FixedProbabilityConnectorBase {
protected:
	std::unique_ptr<Connector> m_connector;
	std::shared_ptr<RandomEngine> m_engine;
	Real m_p = 1.0;

public:
	FixedProbabilityConnector(std::unique_ptr<Connector> connector, Real p,
	                          std::shared_ptr<RandomEngine> engine)
	    : m_connector(std::move(connector)), m_engine(std::move(engine)), m_p(p)
	{
	}

	~FixedProbabilityConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		const size_t first = tar.size();   // Old number of connections
		m_connector->connect(descr, tar);  // Instantiate the connections
		std::uniform_real_distribution<Real> distr(0.0, 1.0);
		for (size_t i = first; i < tar.size(); i++) {
			if (distr(*m_engine) >= m_p) {
				tar[i].n.synapse.weight = 0.0;  // Invalidate the connection
			}
		}
	}

	bool group_connect(const ConnectionDescriptor &,
	                   GroupConnction &) const override
	{
		return false;
	}

	bool valid(const ConnectionDescriptor &descr) const override
	{
		return m_connector->valid(descr);
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
	Real m_weight;
	Real m_delay;

public:
	RandomConnector(Real weight, Real delay, Real probability,
	                std::shared_ptr<RandomEngine> engine)
	    : FixedProbabilityConnector<RandomEngine>(
	          Connector::all_to_all(weight, delay), probability, engine),
	      m_weight(weight),
	      m_delay(delay)
	{
		FixedProbabilityConnectorBase::name_string = "RandomConnector";
	}

	bool group_connect(const ConnectionDescriptor &descr,
	                   GroupConnction &tar) const override
	{
		tar.psrc = descr.pid_src();
		tar.src0 = descr.nid_src0();
		tar.src1 = descr.nid_src1();
		tar.ptar = descr.pid_tar();
		tar.tar0 = descr.nid_tar0();
		tar.tar1 = descr.nid_tar1();
		tar.synapse = Synapse(m_weight, m_delay);
		tar.additional_parameter = Base::m_p;
		tar.connection_name = "RandomConnector";
		return true;
	}
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
	                          size_t i0, size_t i1, const F &f) const
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
			for (size_t j = 0; j < subset_len; j++) {
				f(i, perm[j]);
			}
		}
	}

public:
	FixedFanConnectorBase(Real weight, Real delay,
	                      std::shared_ptr<RandomEngine> engine)
	    : UniformConnector(weight, delay), m_engine(std::move(engine))
	{
	}
};

/**
 * Connects each neuron in the target population with a fixed number of neurons
 * in the source population.
 */
template <typename RandomEngine>
class FixedFanInConnector : public FixedFanConnectorBase<RandomEngine> {
private:
	using Base = FixedFanConnectorBase<RandomEngine>;

	size_t m_n_fan_in;

public:
	FixedFanInConnector(size_t n_fan_in, Real weight, Real delay,
	                    std::shared_ptr<RandomEngine> engine)
	    : Base(weight, delay, std::move(engine)), m_n_fan_in(n_fan_in)
	{
	}

	~FixedFanInConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		Base::generate_connections(
		    descr.nid_src0(), descr.nsrc(), m_n_fan_in, descr.nid_tar0(),
		    descr.nid_tar1(), [&](size_t i, size_t r) {
			    tar.emplace_back(descr.pid_src(), descr.pid_tar(), r, i,
			                     Base::weight(), Base::delay());
			});
	}

	bool group_connect(const ConnectionDescriptor &descr,
	                   GroupConnction &tar) const override
	{
		tar.psrc = descr.pid_src();
		tar.src0 = descr.nid_src0();
		tar.src1 = descr.nid_src1();
		tar.ptar = descr.pid_tar();
		tar.tar0 = descr.nid_tar0();
		tar.tar1 = descr.nid_tar1();
		tar.synapse = Synapse(Base::m_weight, Base::m_delay);
		tar.additional_parameter = m_n_fan_in;
		tar.connection_name = "FixedFanInConnector";
		return true;
	};

	/**
	 * Checks the validity of the connection. For the FixedFanInConnector to be
	 * valid, the number of neurons in the source population must be at least
	 * equal to the fan in.
	 */
	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc() >= m_n_fan_in;
	}

	std::string name() const override { return "FixedFanInConnector"; }
};

/**
 * Connects each neuron in the source population to a fixed number of neurons
 * in the target population.
 */
template <typename RandomEngine>
class FixedFanOutConnector : public FixedFanConnectorBase<RandomEngine> {
private:
	using Base = FixedFanConnectorBase<RandomEngine>;

	size_t m_n_fan_out;

public:
	FixedFanOutConnector(size_t n_fan_out, Real weight, Real delay,
	                     std::shared_ptr<RandomEngine> engine)
	    : Base(weight, delay, std::move(engine)), m_n_fan_out(n_fan_out)
	{
	}

	~FixedFanOutConnector() override {}

	void connect(const ConnectionDescriptor &descr,
	             std::vector<Connection> &tar) const override
	{
		Base::generate_connections(
		    descr.nid_tar0(), descr.ntar(), m_n_fan_out, descr.nid_src0(),
		    descr.nid_src1(), [&](size_t i, size_t r) {
			    tar.emplace_back(descr.pid_src(), descr.pid_tar(), i, r,
			                     Base::weight(), Base::delay());
			});
	}

	bool group_connect(const ConnectionDescriptor &descr,
	                   GroupConnction &tar) const override
	{
        return false;
		tar.psrc = descr.pid_src();
		tar.src0 = descr.nid_src0();
		tar.src1 = descr.nid_src1();
		tar.ptar = descr.pid_tar();
		tar.tar0 = descr.nid_tar0();
		tar.tar1 = descr.nid_tar1();
		tar.synapse = Synapse(Base::m_weight, Base::m_delay);
		tar.additional_parameter = m_n_fan_out;
		tar.connection_name = "FixedFanOutConnector";
		return true;
	};

	/**
	 * Checks the validity of the connection. For the FixedFanOutConnector,
	 * the number of neurons in the target population must be at least equal to
	 * the fan out.
	 */
	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.ntar() >= m_n_fan_out;
	}

	std::string name() const override { return "FixedFanOutConnector"; }
};

/**
 * Inline methods
 */

inline std::unique_ptr<AllToAllConnector> Connector::all_to_all(Real weight,
                                                                Real delay)
{
	return std::make_unique<AllToAllConnector>(weight, delay);
}

inline std::unique_ptr<OneToOneConnector> Connector::one_to_one(Real weight,
                                                                Real delay)
{
	return std::make_unique<OneToOneConnector>(weight, delay);
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

inline std::unique_ptr<FixedProbabilityConnector<std::default_random_engine>>
Connector::fixed_probability(std::unique_ptr<Connector> connector, Real p)
{
	return fixed_probability(std::move(connector), p, std::random_device()());
}

inline std::unique_ptr<FixedProbabilityConnector<std::default_random_engine>>
Connector::fixed_probability(std::unique_ptr<Connector> connector, Real p,
                             size_t seed)
{
	return std::make_unique<
	    FixedProbabilityConnector<std::default_random_engine>>(
	    std::move(connector), p,
	    std::make_shared<std::default_random_engine>(seed));
}

inline std::unique_ptr<RandomConnector<std::default_random_engine>>
Connector::random(Real weight, Real delay, Real p)
{
	return random(weight, delay, p, std::random_device()());
}

inline std::unique_ptr<RandomConnector<std::default_random_engine>>
Connector::random(Real weight, Real delay, Real p, size_t seed)
{
	return std::make_unique<RandomConnector<std::default_random_engine>>(
	    weight, delay, p, std::make_shared<std::default_random_engine>(seed));
}

inline std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
Connector::fixed_fan_in(size_t n_fan_in, Real weight, Real delay)
{
	return fixed_fan_in(n_fan_in, weight, delay, std::random_device()());
}

inline std::unique_ptr<FixedFanInConnector<std::default_random_engine>>
Connector::fixed_fan_in(size_t n_fan_in, Real weight, Real delay, size_t seed)
{
	return std::make_unique<FixedFanInConnector<std::default_random_engine>>(
	    n_fan_in, weight, delay,
	    std::make_shared<std::default_random_engine>(seed));
}

inline std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
Connector::fixed_fan_out(size_t n_fan_out, Real weight, Real delay)
{
	return fixed_fan_out(n_fan_out, weight, delay, std::random_device()());
}

inline std::unique_ptr<FixedFanOutConnector<std::default_random_engine>>
Connector::fixed_fan_out(size_t n_fan_out, Real weight, Real delay, size_t seed)
{
	return std::make_unique<FixedFanOutConnector<std::default_random_engine>>(
	    n_fan_out, weight, delay,
	    std::make_shared<std::default_random_engine>(seed));
}
}

#endif /* CYPRESS_CORE_CONNECTOR_HPP */
