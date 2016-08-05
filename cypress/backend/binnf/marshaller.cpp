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
#include <unordered_set>

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

static const Header TARGET_HEADER = {{"pid", "nid"}, {INT, INT}};
static const Header SPIKE_TIMES_HEADER = {{"times"}, {FLOAT}};
static const Header CONNECTIONS_HEADER = {
    {"pid_src", "pid_tar", "nid_src", "nid_tar", "weight", "delay"},
    {INT, INT, INT, INT, FLOAT, FLOAT}};
static const int32_t ALL_NEURONS = std::numeric_limits<int32_t>::max();

static const std::map<const NeuronType *, int32_t> NEURON_TYPE_ID_MAP{
    {{&SpikeSourceArray::inst(), 0},
     {&IfCondExp::inst(), 1},
     {&EifCondExpIsfaIsta::inst(), 2},
     {&IfFacetsHardware1::inst(), 3}}};

static int32_t binnf_type_id(const NeuronType &type)
{
	auto it = NEURON_TYPE_ID_MAP.find(&type);
	if (it != NEURON_TYPE_ID_MAP.end()) {
		return it->second;
	}
	throw NotSupportedException(std::string("Neuron type \"") + type.name +
	                            "\" not supported!");
}

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
	Header header = {{"count", "type"}, {INT, INT}};

	// Fetch all signals available in the populations
	std::unordered_set<std::string> signals;
	for (auto const &population : populations) {
		for (auto const &signal_name : population.type().signal_names) {
			signals.emplace(signal_name);
		}
	}

	// Add the signals to the header
	for (auto const &signal : signals) {
		header.names.emplace_back(std::string("record_") + signal);
		header.types.emplace_back(INT);
	}

	// Fill the populations matrix
	Matrix<Number> mat(populations.size(), header.size());
	for (size_t i = 0; i < populations.size(); i++) {
		mat(i, 0) = int32_t(populations[i].size());
		mat(i, 1) = int32_t(binnf_type_id(populations[i].type()));
		size_t j = 2;
		for (auto const &signal : signals) {
			// Lookup the signal index for this population
			auto idx = populations[i].type().signal_index(signal);
			bool record = false;  // If this signal does not exist in this
			                      // population, do not try to record it
			if (idx.valid()) {
				if (populations[i].homogeneous_record()) {
					// All neurons in this population use the same record flag
					record = populations[i].signals().is_recording(idx.value());
				}
				else {
					// Check whether any neuron in this population wants to
					// record this signal
					for (size_t k = 0; k < populations[i].size(); k++) {
						record = record ||
						         populations[i][k].signals().is_recording(
						             idx.value());
					}
				}
			}
			mat(i, j++) = record;
		}
	}

	serialise_matrix(os, "populations", header, mat);
}

/**
 * Constructs and sends the connection matrix to the simulator.
 */
static void write_connections(const std::vector<ConnectionDescriptor> &descrs,
                              std::ostream &os)
{
	std::vector<Connection> connections = instantiate_connections(descrs);
	serialise_matrix(os, "connections", CONNECTIONS_HEADER,
	                 reinterpret_cast<Number *>(&connections[0]),
	                 connections.size());
}

static void write_spike_source_array(const PopulationBase &population,
                                     std::ostream &os)
{
	for (size_t i = 0; i < population.size(); i++) {
		const auto &params = population[i].parameters();
		Matrix<Number> mat(1, 2);
		mat(0, 0) = int32_t(population.pid());
		mat(0, 1) = int32_t(i);

		serialise_matrix(os, "target", TARGET_HEADER, mat);
		serialise_matrix(os, "spike_times", SPIKE_TIMES_HEADER,
		                 reinterpret_cast<const Number *>(params.begin()),
		                 params.size());
	}
}

static void write_uniform_parameters(const PopulationBase &population,
                                     std::ostream &os)
{
	// Assemble the parameter header
	Header header = {{"pid", "nid"}, {INT, INT}};
	for (const auto &name : population.type().parameter_names) {
		header.names.emplace_back(name);
		header.types.emplace_back(FLOAT);
	}

	// In case the population is homogeneous, just send one entry in
	// the parameters matrix -- otherwise send an entry for each neuron in each
	// population
	const bool homogeneous = population.homogeneous_parameters();
	const size_t n_rows = homogeneous ? 1 : population.size();
	Matrix<Number> mat(n_rows, header.size());
	for (size_t i = 0; i < n_rows; i++) {
		mat(i, 0) = int32_t(population.pid());
		mat(i, 1) = int32_t(homogeneous ? ALL_NEURONS : i);

		const auto &params = population[i].parameters();
		std::copy(params.begin(), params.end(), mat.begin(i) + 2);
	}

	serialise_matrix(os, "parameters", header, mat);
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

bool marshall_response(NetworkBase &net, std::istream &is)
{
	Block block;
	bool had_block = false;
	bool has_target = false;
	PopulationIndex tar_pid = 0;
	NeuronIndex tar_nid = 0;
	while (is.good()) {
		// Deserialise the incomming data, continue until the end of the input
		// stream is reached, skip faulty blocks
		try {
			block = deserialise(is);
			had_block = true;
		}
		catch (BinnfDecodeException ex) {
			// TODO: Log
			continue;
		}

		if (block.type == BlockType::LOG) {
			net.logger().log(block.severity, block.time, block.module,
			                 block.msg);
			continue;
		}
		else if (block.type != BlockType::MATRIX) {
			throw BinnfDecodeException("Unsupported block type!");
		}

		// Handle the block, depending on its name
		if (block.name == "target") {
			const size_t pid_col = block.colidx("pid");
			const size_t nid_col = block.colidx("nid");
			if (block.matrix.rows() != 1) {
				throw BinnfDecodeException("Invalid target block row count");
			}
			tar_pid = block.matrix(0, pid_col);
			tar_nid = block.matrix(0, nid_col);
			if (tar_pid >= ssize_t(net.population_count()) ||
			    tar_nid >= ssize_t(net.population(tar_pid).size())) {
				throw BinnfDecodeException("Invalid target neuron");
			}
			has_target = true;
		}
		else if (block.name == "spike_times") {
			if (!has_target) {
				throw BinnfDecodeException("No target neuron set");
			}
			if (block.matrix.cols() != 1 || block.colidx("times") != 0) {
				throw BinnfDecodeException("Invalid spike_times column count");
			}
			auto neuron = net[tar_pid][tar_nid];
			auto idx = neuron.type().signal_index("spikes");
			if (idx.valid() && neuron.signals().is_recording(idx.value())) {
				neuron.signals().data(
				    idx.value(),
				    std::make_shared<Matrix<float>>(
				        block.matrix.rows(), 1,
				        reinterpret_cast<float *>(block.matrix.begin())));
			}
			has_target = false;
		}
		else if (block.name.size() > 6 && block.name.substr(0, 6) == "trace_") {
			if (!has_target) {
				throw BinnfDecodeException("No target neuron set");
			}
			if (block.matrix.cols() != 2 || block.colidx("times") != 0 ||
			    block.colidx("values") != 1) {
				throw BinnfDecodeException("Invalid trace data layout!");
			}
			const std::string signal =
			    block.name.substr(6, block.name.size() - 6);
			auto neuron = net[tar_pid][tar_nid];
			auto idx = neuron.type().signal_index(signal);
			if (idx.valid() && neuron.signals().is_recording(idx.value())) {
				neuron.signals().data(
				    idx.value(),
				    std::make_shared<Matrix<float>>(
				        block.matrix.rows(), 2,
				        reinterpret_cast<float *>(block.matrix.begin())));
			}
		}
		else if (block.name == "runtimes") {
			const size_t total_col = block.colidx("total");
			const size_t sim_col = block.colidx("sim");
			const size_t initialize_col = block.colidx("initialize");
			const size_t finalize_col = block.colidx("finalize");

			net.runtime({block.matrix(0, total_col).f,
			             block.matrix(0, sim_col).f,
			             block.matrix(0, initialize_col).f,
			             block.matrix(0, finalize_col).f});

			has_target = false;
		}
	}
	return had_block;
}
}
}

