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
 * @file transformations.hpp
 *
 * Fixes incompatibilities between the parameter sets of various PyNN backends
 * by adapting network parameters.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_BACKEND_PYNN_TRANSFORMATION_HPP
#define CYPRESS_BACKEND_PYNN_TRANSFORMATION_HPP

#include <cstddef>

namespace cypress {

/*
 * Forward declarations
 */
class Connection;

namespace pynn {

/**
 * The transformation class is responsible for adapting the network parameters
 * in such a way that they are compatibly with all simulators. Transformations
 * take place in two directions: once when the network description is sent to
 * the simulator, and once when the data is read back.
 */
class Transformation {
private:
	float m_timestep;

public:
	/**
	 * Constructor of the Transformation class.
	 *
	 * @param timestep is the simulator timestep. Needed for various
	 * transformations.
	 */
	Transformation(float timestep = 0.1): m_timestep(timestep) {}

	/**
	 * Transforms the instantiated connection list. Makes sure all delays are
	 * larger than the simulator timestep.
	 *
	 * @param connections is a pointer at the array containing the connections.
	 * @param size is the length of the connection list.
	 * @return the new length of the connection list.
	 */
	size_t transform_connections(Connection connections[], size_t size);
};

}
}

#endif /* CYPRESS_BACKEND_PYNN_TRANSFORMATION_HPP */
