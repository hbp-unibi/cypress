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

#include <algorithm>
#include <vector>
#include <string>

#include <cypress/backend/binnf/binnf.hpp>
#include <cypress/backend/binnf/marshaller.hpp>
#include <cypress/core/network.hpp>
#include <cypress/util/matrix.hpp>

namespace cypress {
namespace binnf {

static const auto INT = NumberType::INT;
static const auto FLOAT = NumberType::FLOAT;

/**
 * Constructs and sends the matrix containing the populations to the simulator.
 *
 * @param populations is a vector containing the population descriptors.
 * @param os is the output stream to which the population descriptors should be
 * written.
 */
static void write_populations(const std::vector<PopulationBase> &populations,
                              std::ostream &os)
{
	static const Header HEADER = {{"count", "type", "record_spikes", "record_v",
	                               "record_gsyn_exc", "record_gsyn_inh"},
	                              {INT, INT, INT, INT, INT, INT}};

	Matrix<Number> mat(populations.size(), 6);
	for (size_t i = 0; i < populations.size(); i++) {
		mat(i, 0) = int32_t(populations[i].size());
		mat(i, 1) = int32_t(populations[i].type().type_id);
		mat(i, 2) = int32_t(false);
		mat(i, 3) = int32_t(false);
		mat(i, 4) = int32_t(false);
		mat(i, 5) = int32_t(false);
	}

	serialise(os, "populations", HEADER, mat);
}

/**
 * Constructs and sends the connection matrix to the simulator.
 */
static void write_connections(const std::vector<ConnectionDescriptor> &descrs,
                              std::ostream &os)
{
	static const Header HEADER = {
	    {"pid_src", "pid_tar", "nid_src", "nid_tar", "weight", "delay"},
	    {INT, INT, INT, INT, FLOAT, FLOAT}};

	// Vector containing all connection objects
	std::vector<Connection> connections = instantiate_connections(descrs);

	serialise(os, "connections", HEADER,
	          reinterpret_cast<Number *>(&connections[0]), connections.size());
}

void marshall_network(Network &net, std::ostream &os)
{
	// Write the populations
	const std::vector<PopulationBase> populations = net.populations();
	write_populations(populations, os);

	// Write the connections
	write_connections(net.connections(), os);

	// Write the population parameters
}

void marshall_response(Network &net, std::istream &os)
{
	// TODO
}
}
}

