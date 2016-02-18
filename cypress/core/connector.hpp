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

#include <functional>
#include <memory>
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
	float weight;

	/**
	 * Synaptic delay in milliseconds.
	 */
	float delay;

	/**
	 * Constructor of the Synapse structure. Per default initialises all member
	 * variables with zero.
	 */
	Synapse(float weight = 0.0, float delay = 0.0)
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
	uint32_t src;

	/**
	 * Target neuron index within the specified target population.
	 */
	uint32_t tar;

	/**
	 * Synaptic properties.
	 */
	Synapse synapse;

	/**
	 * Constructor of the Connection class. Per default all fields are
	 * initialised with zero.
	 */
	LocalConnection(uint32_t src = 0, uint32_t tar = 0, float weight = 0.0,
	                float delay = 0.0)
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
	           uint32_t ntar = 0, float weight = 0.0, float delay = 0.0)
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
		    ptar, s.ptar)(n.src, s.n.src)(n.tar, s.n.tar)();
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
class FromFunctorConnector;
class FixedProbabilityConnector;
class GaussNoiseConnector;

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
	 * @param tar_mem is the start address of the memory region to which the
	 * connections should be written.
	 * @return the actual number of connections written. May be smaller or equal
	 * to the number estimated by connection_size().
	 */
	virtual size_t connect(const ConnectionDescriptor &descr,
	                       Connection tar_mem[]) const = 0;

	/**
	 * Predicts an upper-bound of the number of connections. The caller of the
	 * connect function ensures that memory for at least as many elements
	 *
	 * @param descr is the descriptor describing the connection for which the
	 * number of neuron-to-neuron connections should be calculated.
	 * @return an upper bound of the number of neuron-to-neuron connections
	 * that will be created by the connector.
	 */
	virtual size_t size(const ConnectionDescriptor &descr) const = 0;

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
	virtual const std::string &name() const = 0;

	/**
	 * Virtual destructor of the connector.
	 */
	/**
	 * Creates an all-to-all connector and returns a pointer at the connector.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 */
	static std::unique_ptr<AllToAllConnector> all_to_all(float weight = 1.0,
	                                                     float delay = 0.0);

	/**
	 * Createa a one-to-one connector and returns a pointer at the connector.
	 */
	static std::unique_ptr<OneToOneConnector> one_to_one(float weight = 1.0,
	                                                     float delay = 0.0);

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
	 * @param f is a function which is called for each neuron-pair in the two
	 * populations and which is supposed to return a synapse description. If the
	 * returned synapse is invalid, no connection is created.
	 */
	static std::unique_ptr<FromListConnector> from_functor(
	    std::function<Synapse(uint32_t nsrc, uint32_t ntar)> f);

	/**
	 * Connector adapter which ensures connections are only produced with a
	 * certain probability.
	 *
	 * @param connector another connector that should be modified by this
	 * connector.
	 * @param p probability with which an connection is created.
	 */
	static std::unique_ptr<FixedProbabilityConnector> fixed_probability(
	    std::unique_ptr<Connector> connector, float p = 1.0);

	/**
	 * Connector adapter which adds Gaussian noise to the synaptic parameters.
	 *
	 * @param connector another connector that should be modified by this
	 * connector.
	 * @param stddev_weight is the weight standard deviation. Note that the
	 * sign of the synaptic weight will not be modified (an excitatory synapse
	 * will not suddenly become inhibitory), the weights are clamped to zero.
	 * @param stddev_delay is the delay standard deviation. The result is
	 * clamped to zero.
	 * @param resurrect if true, also adds noise to zero-weight synapses,
	 * effectively generating connections where none were before.
	 */
	static std::unique_ptr<GaussNoiseConnector> gauss_noise(
	    std::unique_ptr<Connector> connector, float stddev_weight = 0.0,
	    float stddev_delay = 0.0, bool resurrect = false);
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
	 * @param descr is the connection descriptor containing the data detailing
	 * the connection.
	 * @param tar_mem is the start address of the memory region to which the
	 * connections should be written.
	 * @return the actual number of connections written. May be smaller or equal
	 * to the number estimated by connection_size().
	 */
	size_t connect(Connection tar_mem[]) const
	{
		return connector().connect(*this, tar_mem);
	}

	/**
	 * Returns an upper bound of the number of neuron-to-neuron connections
	 * that will be created by the connector.
	 */
	size_t size() const { return connector().size(*this); }

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
 * Abstract base class for connectors with a weight and an delay.
 */
class UniformConnector : public Connector {
private:
	float m_weight;
	float m_delay;

public:
	UniformConnector(float weight = 0.0, float delay = 0.0)
	    : m_weight(weight), m_delay(delay)
	{
	}

	float weight() const { return m_weight; }
	float delay() const { return m_delay; }
};

/**
 * The AllToAllConnector class is used to establish a connection from each
 * neuron in the specified source population to every neuron in the specified
 * target population.
 */
class AllToAllConnector : public UniformConnector {
private:
	static const std::string m_name;

public:
	using UniformConnector::UniformConnector;

	~AllToAllConnector() override {}

	size_t connect(const ConnectionDescriptor &descr,
	               Connection tar_mem[]) const override;

	size_t size(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc() * descr.ntar();
	}

	bool valid(const ConnectionDescriptor &) const override { return true; }

	const std::string &name() const override { return m_name; }
};

/**
 * The OneToOneConnector connects pairs of neurons between source and target
 * population. The number of neurons in both populations must be equal.
 */
class OneToOneConnector : public UniformConnector {
private:
	static const std::string m_name;

public:
	using UniformConnector::UniformConnector;

	~OneToOneConnector() override {}

	size_t connect(const ConnectionDescriptor &descr,
	               Connection tar_mem[]) const override;

	size_t size(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc();
	}

	bool valid(const ConnectionDescriptor &descr) const override
	{
		return descr.nsrc() == descr.ntar();
	}

	const std::string &name() const override { return m_name; }
};
}

#endif /* CYPRESS_CORE_CONNECTOR_HPP */
