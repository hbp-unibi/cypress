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
 * @file backend.hpp
 *
 * Contains the basic definition of the Backend interface, which is used to
 * actually run the network on a simulator.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_BACKEND_HPP
#define CYPRESS_BACKEND_HPP

#include <string>
#include <unordered_set>

#include <cypress/core/types.hpp>

namespace cypress {

/*
 * Forward declarations
 */
class NetworkBase;
class NeuronType;

/**
 * The Backend class is an abstract base class which provides the facilities
 * for network execution.
 */
class Backend {
protected:
	/**
	 * Abstract method which has to be implemented by the backend.
	 */
	virtual void do_run(NetworkBase &network, Real duration) const = 0;

public:
	/**
	 * Simulates the given spiking neural network for the given duration.
	 *
	 * @param network is the network that should be simulated. The simulation
	 * result will be written to the network instance.
	 * @param duration is the duration for which the network should be
	 * simulated.
	 */
	void run(NetworkBase &network, Real duration = 0.0) const;

	/**
	 * Returns a set of neuron types which are supported by this backend. Trying
	 * to execute a network with other neurons than the ones specified in the
	 * result of this function will result in an exception.
	 *
	 * @return a set of neuron types supported by this particular backend
	 * instance.
	 */
	virtual std::unordered_set<const NeuronType *> supported_neuron_types()
	    const = 0;

	/**
	 * Returns the canonical name of the backend.
	 */
	virtual std::string name() const = 0;

	/**
	 * Destructor of the backend class.
	 */
	virtual ~Backend();
};
}

#endif /* CYPRESS_BACKEND_HPP */
