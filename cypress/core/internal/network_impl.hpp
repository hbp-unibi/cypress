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
 * @file network_impl.hpp
 *
 * Contains the hidden implementation of the Network class. Note that the
 * interface defined in this header should neither be exposed to, nor be used
 * by client programs.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_NETWORK_IMPL_HPP
#define CYPRESS_CORE_NETWORK_IMPL_HPP

#include <cypress/core/connector.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/core/types.hpp>

namespace cypress {
namespace internal {
/**
 * Structure containing all the data stored in a population object.
 */
struct PopulationImpl {
	/**
	 * Size of the population.
	 */
	size_t size;

	/**
	 * Reference at the underlying neuron type.
	 */
	NeuronType const *type;

	/**
	 * Parameters of all neurons in the population. If zero elements are stored
	 * in the vector, this means that the default parameters are used. If one
	 * element is stored, this means that the parameters are shared
	 * homogeneously between all neurons of the population. If the size of the
	 * parameters vector corresponds to the size of the population, each neuron
	 * possesses an individual parameter set.
	 */
	std::vector<NeuronParametersBase> parameters;

	/**
	 * Name of the population.
	 */
	std::string name;

	/**
	 * Constructor of the PopulationImpl class.
	 *
	 * @param size is the size of the population.
	 * @param type is the underlying neuron type shared by all neurons in the
	 * population.
	 * @param parameters is a vector containing the population parameters.
	 * @param name is the (optional) name of the population.
	 */
	PopulationImpl(size_t size = 0,
	               const NeuronType &type = NullNeuronType::inst(),
	               const std::vector<NeuronParametersBase> &parameters =
	                   std::vector<NeuronParametersBase>(),
	               const std::string &name = std::string())
	    : size(size), type(&type), parameters(parameters), name(name)
	{
	}
};

/**
 * Spiking neural network data container. Stores all the data describing the
 * network.
 */
class NetworkImpl {
private:
	/**
	 * Vector containing the PopulationImpl instances.
	 */
	std::vector<PopulationImpl> m_populations;

	/**
	 * Vector containing all connections.
	 */
	mutable std::vector<ConnectionDescriptor> m_connections;

	/**
	 * Flag indicating whether the connections are currently sorted.
	 */
	mutable bool m_connections_sorted;

public:
	/**
	 * Default constructor of the NetworkImpl class.
	 */
	NetworkImpl();

	/**
	 * Returns a reference at the vector containing the population data.
	 */
	std::vector<PopulationImpl> &populations() { return m_populations; }

	/**
	 * Returns a const reference at the vector containing the population data.
	 */
	const std::vector<PopulationImpl> &populations() const
	{
		return m_populations;
	}

	/**
	 * Returns the indices of the populations fulfilling a the given filter
	 * criteria.
	 *
	 * @param name is the name the returned populations should be filtered by.
	 * If an empty string is given, no filtering is performed (default).
	 * @param type is the neuron type the list should be filtered for. If
	 * a pointer at the special NullNeuronType instance  is given, no filtering
	 * by type is performed (default).
	 * @return a list of population indices.
	 */
	std::vector<PopulationIndex> populations(const std::string &name,
	                                         const NeuronType &type);

	/**
	 * Adds the given connector to the connection list.
	 */
	void connect(const ConnectionDescriptor &descr);

	/**
	 * Returns the list of connections. Makes sure the returned connections are
	 * sorted.
	 */
	const std::vector<ConnectionDescriptor> &connections() const;
};
}
}

#endif /* CYPRESS_CORE_NETWORK_IMPL_HPP */
