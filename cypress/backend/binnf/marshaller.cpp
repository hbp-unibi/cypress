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
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
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

static void write_spike_source_array(const PopulationBase &population,
                                     std::ostream &os)
{
	static const Header TARGET_HEADER = {{"pid", "nid"}, {INT, INT}};
	static const Header SPIKE_TIMES_HEADER = {{"times"}, {FLOAT}};

	for (size_t i = 0; i < population.size(); i++) {
		Matrix<Number> mat(1, 2);
		mat(0, 0) = int32_t(population.pid());
		mat(0, 1) = int32_t(i);

		serialise(os, "target", TARGET_HEADER, mat);
		serialise(
		    os, "spike_times", SPIKE_TIMES_HEADER,
		    reinterpret_cast<const Number *>(population.parameters(i).begin()),
		    population.parameters(i).size());
	}
}

static void write_uniform_parameters(const PopulationBase &population,
                                     std::ostream &os)
{
	static const int32_t ALL_NEURONS = std::numeric_limits<int32_t>::max();

	// Assemble the parameter header
	Header header = {{"pid", "nid"}, {INT, INT}};
	for (const auto &name : population.type().parameter_names) {
		header.names.emplace_back(name);
		header.types.emplace_back(INT);
	}

	// In case the population is homogeneous, just send one entry in
	// the parameters matrix -- otherwise send an entry for each neuron in each
	// population
	const bool homogeneous = population.homogeneous();
	Matrix<Number> mat(homogeneous ? 1 : population.size(), header.size());
	for (size_t i = 0; i < population.size(); i++) {
		mat(0, 0) = int32_t(population.pid());
		mat(0, 1) = int32_t(homogeneous ? ALL_NEURONS : i);

		const auto &params = population.parameters(i);
		std::copy(params.begin(), params.end(), mat.begin(i) + 2);
	}

	serialise(os, "parameters", header, mat);
}

/**
 * Sends the parameters of an individual population to the simulator.
 */
static void write_parameters(const PopulationBase &population, std::ostream &os)
{
	// Do not write anything for zero-sized populations
	if (population.size() == 0) {
		return;
	}

	// Special treatment for spike source arrays
	if (&population.type() == &SpikeSourceArray::inst()) {
		write_spike_source_array(population, os);
	}
	else {
		write_uniform_parameters(population, os);
	}
}

void marshall_network(NetworkBase &net, std::ostream &os)
{
	// Write the populations
	const std::vector<PopulationBase> populations = net.populations();
	write_populations(populations, os);

	// Write the connections
	write_connections(net.connections(), os);

	// Write the population parameters
	for (const auto &population : populations) {
		write_parameters(population, os);
	}
}

void marshall_response(NetworkBase &net, std::istream &os)
{
	// TODO
}
}
}

