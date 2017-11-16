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
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include <cypress/backend/binnf/binnf.hpp>
#include <cypress/backend/binnf/marshaller.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/matrix.hpp>

namespace cypress {
namespace binnf {

// Use 32 bit integers as default integer width in Binnf.
static const NumberType INT = NumberType::INT32;
// Either use 32 or 64 bit floats depending on the CMake user setting
#if CYPRESS_REAL_WIDTH == 4
static const NumberType FLOAT = NumberType::FLOAT32;
#else
static const NumberType FLOAT = NumberType::FLOAT64;
#endif

static int32_t binnf_type_id(const NeuronType &type)
{
	static const std::map<const NeuronType *, int32_t> NEURON_TYPE_ID_MAP{
	    {{&SpikeSourceArray::inst(), 0},
	     {&IfCondExp::inst(), 1},
	     {&EifCondExpIsfaIsta::inst(), 2},
	     {&IfFacetsHardware1::inst(), 3}}};

	auto it = NEURON_TYPE_ID_MAP.find(&type);
	if (it != NEURON_TYPE_ID_MAP.end()) {
		return it->second;
	}
	throw NotSupportedException(std::string("Neuron type \"") + type.name +
	                            "\" not supported!");
}

static int32_t binnf_connector_id(const std::string &connector)
{
	static const std::map<const std::string, int32_t> CONNECTION_TYPE_ID_MAP{
	    {{"AllToAllConnector", 1},       // PyNN: AllToAllConnector
	     {"OneToOneConnector", 2},       // PyNN: OneToOneConnector
	     {"RandomConnector", 3},         // PyNN: FixedProbabilityConnector
	     {"FixedFanInConnector", 4},     // PyNN: FixedNumberPreConnector
	     {"FixedFanOutConnector", 5}}};  // PyNN: FixedNumberPostConnector

	auto it = CONNECTION_TYPE_ID_MAP.find(connector);
	if (it != CONNECTION_TYPE_ID_MAP.end()) {
		return it->second;
	}
	return 0;
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
	std::vector<std::string> names{"count", "type"};
	std::vector<NumberType> types{INT, INT};

	// Fill the populations matrix
	Block block("populations", {names, types}, populations.size());
	for (size_t i = 0; i < populations.size(); i++) {
		block.set(i, 0, populations[i].size());
		block.set(i, 1, binnf_type_id(populations[i].type()));
	}

	serialise(os, block);
}

static void write_inhomogeneous_record(const PopulationBase &population,
                                       std::ostream &os)
{
	std::vector<std::string> names;
	std::vector<NumberType> types;

	// Fetch all signals available in the populations
	std::vector<std::string> signals;
	for (auto const &signal_name : population.type().signal_names) {
		signals.emplace_back(signal_name);
	}

	// Add the signals to the header
	for (auto const &signal : signals) {
		names.emplace_back(std::string("record_") + signal);
		types.emplace_back(INT);
	}

	const bool homogeneous = population.homogeneous_record();
	const size_t n_rows = homogeneous ? 1 : population.size();

	// Fill the signals matrix
	Block block("signals", {names, types}, n_rows);
	if (homogeneous) {
		size_t j = 0;
		for (auto const &signal : signals) {
			auto idx = population.type().signal_index(signal);
			if (idx.valid()) {
				block.set(0, j++,
				          population.signals().is_recording(idx.value()));
			}
			else {
				block.set(0, j++, false);
			}
		}
	}
	else {
		for (size_t i = 0; i < n_rows; i++) {
			size_t j = 0;
			for (auto const &signal : signals) {
				// Lookup the signal index for this population
				auto idx = population.type().signal_index(signal);
				bool record = false;  // If this signal does not exist in this
				                      // population, do not try to record it
				if (idx.valid()) {
					// Check whether the neuron wants to record this signal
					record = population[i].signals().is_recording(idx.value());
				}
				block.set(i, j++, record);
			}
		}
	}
	serialise(os, block);
}

namespace {
#pragma pack(push, 1)
// Structure for writing data in BiNNF table
struct custom_conn_descr {
	uint32_t pid_src = 0;
	NeuronIndex nid_src_start = 0;
	NeuronIndex nid_src_end = 0;
	uint32_t pid_tar = 0;
	NeuronIndex nid_tar_start = 0;
	NeuronIndex nid_tar_end = 0;
	int32_t connector_id = 0;
	Real weight = 0.0;
	Real delay = 0.0;
	Real parameter = 0.0;
};

/*
 * Contains information about one instance of list connection
 */
struct list_con_header {
	uint32_t pid_src = 0;
	uint32_t pid_tar = 0;
	uint32_t inhibitory = 0;
	uint32_t file = 0;
	list_con_header(uint32_t pid_src, uint32_t pid_tar, uint32_t inhibitory,
	                uint32_t file)
	    : pid_src(pid_src), pid_tar(pid_tar), inhibitory(inhibitory), file(file)
	{
	}
};
#pragma pack(pop)

custom_conn_descr convert_non_list_connection(GroupConnction conn)
{
	custom_conn_descr res;
	res.pid_src = conn.psrc;
	res.nid_src_start = conn.src0;
	res.nid_src_end = conn.src1;
	res.pid_tar = conn.ptar;
	res.nid_tar_start = conn.tar0;
	res.nid_tar_end = conn.tar1;
	res.connector_id = binnf_connector_id(conn.connection_name);
	res.weight = conn.synapse.weight;
	res.delay = conn.synapse.delay;
	res.parameter = conn.additional_parameter;
	return res;
}

/**
* Writes a vector of Local Connection (without population IDs) to file, which is
* readable by PyNN's "FromFileConnector". This is probably slower, but will save
* some RAM on the host machine
*
* @param filename name of the file to write to
* @param conns List of local connections
*/
void write_local_connections_to_file(std::string filename,
                                     const std::vector<LocalConnection> &conns)
{
	std::ofstream file;
	file.open(filename);
	if (!file.good()) {
		throw std::runtime_error("Could not write connections!");
	}
	file << "#columns={\"weight\", \"delay\"}" << std::endl;
	// TODO: compatibility hack for NEST: Delay must be above timestep. Doing so
	// in Python is pain/not possible, so do it here. Change adaptivley to
	// current used value missing
	if (filename.find("nest") != std::string::npos) {
		for (auto i : conns) {
			file << i.src << " " << i.tar << " " << i.synapse.weight << " "
			     << std::max(i.synapse.delay, 0.1) << std::endl;
		}
	}
	else {
		for (auto i : conns) {
			file << i.src << " " << i.tar << " " << i.synapse.weight << " "
			     << i.synapse.delay << std::endl;
		}
	}
	file.close();
}

/**
 * Write list connections, split connections into excitatory and inhibitory
 * connections as a compatibility hack
 *
 * @param descrs list of Connection descriptions
 * @param os output stream
 * @param base_filename name of files to write to
 */
static void write_list_connections(
    const std::vector<ConnectionDescriptor> &descrs, std::ostream &os,
    const std::string base_filename)
{
	static const Header LOCAL_CONNECTION_HEADER = {
	    {"nid_src", "nid_tar", "weight", "delay"}, {INT, INT, FLOAT, FLOAT}};
	static const Header LIST_CONNECTIONS_HEADER = {
	    {"pid_src", "pid_tar", "inh", "file"}, {INT, INT, INT, INT}};

	std::vector<list_con_header> pids;

	for (auto des : descrs) {
		std::vector<LocalConnection> conns_exc, conns_inh;
		std::vector<Connection> conns_full;
		des.connect(conns_full);
		for (auto i : conns_full) {
			if (i.n.synapse.weight < 0) {
				conns_inh.emplace_back(i.n.absolute_connection());
			}
			else {
				conns_exc.emplace_back(i.n);
			}
		}

		if (conns_inh.size() > 0) {
            // Choose, whether connections should be writenn to file 
			if (conns_inh.size() < 1000000000 or base_filename == "") {
				serialise_matrix(os, "list_connection", LOCAL_CONNECTION_HEADER,
				                 reinterpret_cast<uint8_t *>(&conns_inh[0]),
				                 conns_inh.size());
				pids.emplace_back(
				    list_con_header(des.pid_src(), des.pid_tar(), 1, 0));
			}
			else {
				write_local_connections_to_file(
				    base_filename + "_" + std::to_string(pids.size()),
				    conns_inh);
				pids.emplace_back(
				    list_con_header(des.pid_src(), des.pid_tar(), 1, 1));
				std::vector<LocalConnection> empty;
				serialise_matrix(os, "list_connection", LOCAL_CONNECTION_HEADER,
				                 reinterpret_cast<uint8_t *>(&empty),
				                 empty.size());
			}
		}

		if (conns_exc.size() > 0) {
            // Choose, whether connections should be writenn to file 
			if (conns_exc.size() < 1000000000 or base_filename == "") {
				serialise_matrix(os, "list_connection", LOCAL_CONNECTION_HEADER,
				                 reinterpret_cast<uint8_t *>(&conns_exc[0]),
				                 conns_exc.size());
				pids.emplace_back(
				    list_con_header(des.pid_src(), des.pid_tar(), 0, 0));
			}
			else {
				write_local_connections_to_file(
				    base_filename + "_" + std::to_string(pids.size()),
				    conns_exc);
				pids.emplace_back(
				    list_con_header(des.pid_src(), des.pid_tar(), 0, 1));
				std::vector<LocalConnection> empty;
				serialise_matrix(os, "list_connection", LOCAL_CONNECTION_HEADER,
				                 reinterpret_cast<uint8_t *>(&empty),
				                 empty.size());
			}
		}
	}
	serialise_matrix(os, "list_connection_header", LIST_CONNECTIONS_HEADER,
	                 reinterpret_cast<uint8_t *>(&pids[0]), pids.size());
}
}

/**
 * Constructs and sends the connections to the simulator. There are two
 * different formats: Own connectors are expanded to a list, PyNN supported
 * Connectors are converted to a compact custom_conn_descr. The latter makes use
 * of PyNN included connectors.
 */
static void write_connections(const std::vector<ConnectionDescriptor> &descrs,
                              std::ostream &os,
                              const std::string &base_filename)
{
	static const Header LIST_CONNECTIONS_HEADER = {
	    {"pid_src", "pid_tar", "nid_src", "nid_tar", "weight", "delay"},
	    {INT, INT, INT, INT, FLOAT, FLOAT}};

	static const Header GROUP_CONNECTIONS_HEADER = {
	    {"pid_src", "nid_src_start", "nid_src_end", "pid_tar", "nid_tar_start",
	     "nid_tar_end", "connector_id", "weight", "delay", "parameter"},
	    {INT, INT, INT, INT, INT, INT, INT, FLOAT, FLOAT, FLOAT}};

	std::vector<ConnectionDescriptor> list_connections;
	std::vector<custom_conn_descr> group_connections;

	for (auto desc : descrs) {
		GroupConnction temp;
		// Check wether connection can be used as grouped connection
		if (desc.connector().group_connect(desc, temp)) {
			group_connections.emplace_back(convert_non_list_connection(temp));
		}
		else {
			list_connections.emplace_back(desc);
		}
	}

	write_list_connections(list_connections, os, base_filename);

	serialise_matrix(os, "group_connections", GROUP_CONNECTIONS_HEADER,
	                 reinterpret_cast<uint8_t *>(&group_connections[0]),
	                 group_connections.size());
}

static void write_target(PopulationIndex pid, NeuronIndex nid, std::ostream &os)
{
	static const Header TARGET_HEADER = {{"pid", "nid"}, {INT, INT}};

	Block block("target", TARGET_HEADER, 1);
	block.set(0, 0, pid);
	block.set(0, 1, nid);
	serialise(os, block);
}

static void write_spike_source_array(const PopulationBase &population,
                                     std::ostream &os)
{
	static const Header SPIKE_TIMES_HEADER = {{"times"}, {FLOAT}};

	for (size_t i = 0; i < population.size(); i++) {
		// Send the target header
		write_target(population.pid(), i, os);

		// Send the actual spike times
		const auto &params = population[i].parameters();
		Block block("spike_times", SPIKE_TIMES_HEADER, params.size());
		for (size_t j = 0; j < params.size(); j++) {
			block.set(j, 0, params[j]);
		}
		serialise(os, block);
	}
}

static void write_uniform_parameters(const PopulationBase &population,
                                     std::ostream &os)
{
	static const int32_t ALL_NEURONS = std::numeric_limits<int32_t>::max();

	// Assemble the parameter header
	std::vector<std::string> names{"pid", "nid"};
	std::vector<NumberType> types{INT, INT};
	for (const auto &name : population.type().parameter_names) {
		names.emplace_back(name);
		types.emplace_back(FLOAT);
	}

	// In case the population is homogeneous, just send one entry in
	// the parameters matrix -- otherwise send an entry for each neuron in each
	// population
	const bool homogeneous = population.homogeneous_parameters();
	const size_t n_rows = homogeneous ? 1 : population.size();

	// Assemble the block containing the parameters
	Block block("parameters", {names, types}, n_rows);
	for (size_t i = 0; i < n_rows; i++) {
		block.set(i, 0, population.pid());
		block.set(i, 1, homogeneous ? ALL_NEURONS : i);

		const auto &params = population[i].parameters();
		for (size_t j = 0; j < params.size(); j++) {
			block.set(i, j + 2, params[j]);
		}
	}
	serialise(os, block);
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

void marshall_network(NetworkBase &net, std::ostream &os,
                      std::string base_filename)
{
	// Write the populations
	const std::vector<PopulationBase> populations = net.populations();
	write_populations(populations, os);

	// Write the connections
	write_connections(net.connections(), os, base_filename);

	// Write the population parameters
	for (const auto &population : populations) {
		write_inhomogeneous_record(population, os);
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
			if (block.type == BlockType::INVALID) {
				return had_block;  // We're at the end of the stream
			}
			had_block = true;

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
				if (block.rows() != 1) {
					throw BinnfDecodeException(
					    "Invalid target block row count");
				}
				tar_pid = block.get_int(0, pid_col);
				tar_nid = block.get_int(0, nid_col);
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
				if (block.cols() != 1 || block.colidx("times") != 0) {
					throw BinnfDecodeException(
					    "Invalid spike_times column count");
				}
				auto neuron = net[tar_pid][tar_nid];
				auto idx = neuron.type().signal_index("spikes");
				if (idx.valid() && neuron.signals().is_recording(idx.value())) {
					auto data = std::make_shared<Matrix<Real>>(block.rows(), 1);
					for (size_t i = 0; i < block.rows(); i++) {
						(*data)(i, 0) = block.get_float(i, 0);
					}
					neuron.signals().data(idx.value(), std::move(data));
				}
				has_target = false;
			}
			else if (block.name.size() > 6 &&
			         block.name.substr(0, 6) == "trace_") {
				if (!has_target) {
					throw BinnfDecodeException("No target neuron set");
				}
				if (block.cols() != 2 || block.colidx("times") != 0 ||
				    block.colidx("values") != 1) {
					throw BinnfDecodeException("Invalid trace data layout!");
				}
				const std::string signal =
				    block.name.substr(6, block.name.size() - 6);
				auto neuron = net[tar_pid][tar_nid];
				auto idx = neuron.type().signal_index(signal);
				if (idx.valid() && neuron.signals().is_recording(idx.value())) {
					auto data = std::make_shared<Matrix<Real>>(block.rows(), 2);
					for (size_t i = 0; i < block.rows(); i++) {
						(*data)(i, 0) = block.get_float(i, 0);
						(*data)(i, 1) = block.get_float(i, 1);
					}
					neuron.signals().data(idx.value(), std::move(data));
				}
			}
			else if (block.name == "runtimes") {
				const size_t total_col = block.colidx("total");
				const size_t sim_col = block.colidx("sim");
				const size_t initialize_col = block.colidx("initialize");
				const size_t finalize_col = block.colidx("finalize");

				net.runtime({block.get_float(0, total_col),
				             block.get_float(0, sim_col),
				             block.get_float(0, initialize_col),
				             block.get_float(0, finalize_col)});

				has_target = false;
			}
		}
		catch (BinnfDecodeException &ex) {
			net.logger().error(
			    "cypress",
			    std::string("Error while parsing BiNNF: ") + ex.what());
			continue;
		}
	}
	return had_block;
}

bool marshall_log(Logger &logger, std::istream &is)
{
	Block block;
	bool had_block = false;
	while (is.good()) {
		// Deserialise the incomming data, continue until the end of the input
		// stream is reached, skip faulty blocks
		try {
			block = deserialise(is);
			if (block.type == BlockType::INVALID) {
				return had_block;  // We're at the end of the stream
			}
			had_block = true;

			if (block.type == BlockType::LOG) {
				logger.log(block.severity, block.time, block.module, block.msg);
				continue;
			}
			else if (block.type != BlockType::MATRIX) {
				throw BinnfDecodeException("Unsupported block type!");
			}
		}
		catch (BinnfDecodeException &ex) {
			logger.error(
			    "cypress",
			    std::string("Error while parsing BiNNF: ") + ex.what());
			continue;
		}
	}
	return had_block;
}
}
}
