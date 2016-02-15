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

namespace cypress {

/*
 * Forward declarations
 */
class Network;

/**
 * The Backend class is an abstract base class which provides the facilities
 * for network execution.
 */
class Backend {
protected:
	/**
	 * Abstract method which has to be implemented by the backend.
	 */
	virtual void do_run(Network &network, float duration) const = 0;

public:
	/**
	 * Simulates the given spiking neural network for the given duration.
	 */
	void run(Network &network, float duration = 0.0) const;

	/**
	 * Destructor of the backend class.
	 */
	virtual ~Backend();
};
}
