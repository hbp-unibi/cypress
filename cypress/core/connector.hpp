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
#include <string>
#include <vector>

#include <cypress/util/comperator.hpp>

namespace cypress {
/**
 * Data structure describing a connection between two neurons.
 */
#pragma pack push(1)
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
	Connection(uint32_t src = 0, uint32_t tar = 0, float weight = 0.0,
	           delay = 0.0)
	    : src(nsrc),

	      tar(ntar),
	      synapse(weight, delay)
	{
	}

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() { return synapse.valid(); }

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
	           uint32_t ntar = 0, float weight = 0.0, delay = 0.0)
	    : psrc(psrc), ptar(ptar), n(nsrc, nsrc, weight, delay)
	{
	}

	/**
	 * Checks whether the connection is valid.
	 */
	bool valid() { return n.synapse.valid(); }

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
		return Comperator<uint32_t>::equals(1 - uint32_t(valid()))(
		    psrc, s.psrc)(ptar, s.ptar)(n.src, s.n.src)(n.tar, s.n.tar)();
	}

	/**
	 * Used to sort the connections by their valid status, their population ids
	 * and their neuron ids. The precedence of these fields is: validity, psrc,
	 * ptar, nsrc, ntar. Invalid neurons are sorted to the end to be able to
	 * simply throw them away.
	 */
	bool operator<(const Connection &s) const
	{
		return Comperator<uint32_t>::smaller(1 - uint32_t(valid()))(
		    psrc, s.psrc)(ptar, s.ptar)(n.src, s.n.src)(n.tar, s.n.tar)();
	}
};
#pragma pop

/*
 * Forward declarations.
 */
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
	 * The ConnectionDescriptor class details
	 */
	struct ConnectionDescriptor {
		/**
		 * The source population id.
		 */
		uint32_t pid_src;

		/**
		 * The target population id.
		 */
		uint32_t pid_src;

		/**
		 * Index of the first neuron in the source population involved in the
		 * connection.
		 */
		uint32_t nid_src_begin;

		/**
		 * Index of the first neuron in the target population involved in the
		 * connection.
		 */
		uint32_t nid_src_end;

		/**
		 * Index of the first neuron in the target population involved in the
		 * connection.
		 */
		uint32_t nid_tar_begin;

		/**
		 * Index of the last-plus-one neuron in the target population involved
		 * in the connection.
		 */
		uint32_t nid_tar_end;
	};

	/**
	 * Tells the Connector to actually create the neuron-to-neuron connections
	 * between certain neurons.
	 *
	 * @param descr is the connection descriptor containing the data detailing
	 * the connection.
	 * @param tar_mem is the start address of the memory region to which the
	 * connections should be written.
	 * @return the actual number of connections written. May be smaller or equal
	 * to the number estimated by connection_count().
	 */
	virtual size_t connect(const ConnectionDescriptor &descr,
	                       Connection[] tar_mem) = 0;

	/**
	 * Predicts an upper-bound of the number of connections. The caller of the
	 * connect function ensures that memory for at least as many elements
	 *
	 * @param descr is the descriptor describing the connection for which the
	 * number of neuron-to-neuron connections should be calculated.
	 * @return an upper bound of the number of neuron-to-neuron connections
	 * that will be created by the connector.
	 */
	virtual size_t connection_count(const ConnectionDescriptor &descr) = 0;

	/**
	 * Returns true if the connector can create the connections for the given
	 * connection descriptor. While most connectors will always be able to
	 * create a connection, for example a one-to-one connector
	 */
	virtual bool connection_valid(const ConnectionDescriptor &descr) = 0;

	/**
	 * Function which should return a name identifying the connector type --
	 * this name can be used for printing error messages or for visualisation
	 * purposes.
	 *
	 * @return a reference at a string describing the name of the descriptor
	 * type.
	 */
	virtual const std::string &name() = 0;

	/**
	 * Virtual destructor of the connector.
	 */
	virtual ~Connector();

	/**
	 * Creates an all-to-all connector and returns a pointer at the connector.
	 *
	 * @param weight is the synaptic weight that should be used for all
	 * connections.
	 * @param delay is the synaptic delay that should be used for all
	 * connections.
	 */
	static std::unique_ptr<AllToAllConnector> all_to_all(float weight = 1.0,
	                                                     delay = 0.0);

	/**
	 * Createa a one-to-one connector and returns a pointer at the connector.
	 */
	static std::unique_ptr<OneToOneConnector> one_to_one(float weight = 1.0,
	                                                     delay = 0.0);

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


class AllToAllConnector: public Connector {
private:
	static const std::string m_name;

public:
	size_t connect(const ConnectionDescriptor &descr,
	                       Connection[] tar_mem) override;

	size_t connection_count(const ConnectionDescriptor &descr) override;

	bool connection_valid(const ConnectionDescriptor &descr) override;

	const std::string &name() override; {return m_name;}

	~AllToAllConnector() override;
};

}

#endif /* CYPRESS_CORE_CONNECTOR_HPP */
