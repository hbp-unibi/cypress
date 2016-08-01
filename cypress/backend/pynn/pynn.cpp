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
#include <future>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <unistd.h>

#include <cypress/backend/binnf/marshaller.hpp>
#include <cypress/backend/pynn/pynn.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/process.hpp>
#include <cypress/util/filesystem.hpp>

// Enable to get a textual dump of the BiNNF instead of running the simulation
//#define CYPRESS_DEBUG_BINNF

namespace cypress {
namespace {
/**
 * Struct assining certain properties to the various hardware platforms.
 */
struct SystemProperties {
private:
	bool m_analogue;
	bool m_hardware;
	bool m_emulator;

public:
	SystemProperties(bool analogue, bool hardware, bool emulator)
	    : m_analogue(analogue), m_hardware(hardware), m_emulator(emulator)
	{
	}

	bool analogue() { return m_analogue; }
	bool hardware() { return m_hardware; }
	bool emulator() { return m_emulator; }
	bool software() { return m_emulator || !m_hardware; }
};

/**
 * List containing all supported simulators. Other simulators may also be
 * supported if they follow the PyNN specification, however, these simulators
 * were tested and will be returned by the "backends" method. The simulator
 * names are the normalized simulator names.
 */
static const std::vector<std::string> SUPPORTED_SIMULATORS = {
    "nest", "ess", "nmpm1", "nmmc1", "spikey"};

/**
 * Used to map certain simulator names to a more canonical form. This
 * canonical form does not correspond to the name of the actual module
 * includes but the names that "feel more correct".
 */
static const std::unordered_map<std::string, std::string>
    NORMALISED_SIMULATOR_NAMES = {
        {"pyhmf", "ess"},     {"spinnaker", "nmmc1"},
        {"pyhmf", "nmpm1"},   {"hardware.spikey", "spikey"},
        {"spikey", "spikey"}, {"nest", "nest"},
        {"nm-mc1", "nmmc1"},  {"nm-pm1", "nmpm1"},
        {"ess", "ess"}};

/**
 * Maps certain simulator names to the correct PyNN module names. If multiple
 * module names are given, the first found module is used.
 */
static const std::unordered_map<std::string, std::string> SIMULATOR_IMPORT_MAP =
    {{"nest", "pyNN.nest"},
     {"ess", "pyhmf"},
     {"nmmc1", "pyNN.spiNNaker"},
     {"nmpm1", "pyhmf"},
     {"spikey", "pyNN.hardware.spikey"}};

/**
 * Map between the canonical simulator name and the NMPI platform name.
 */
static const std::unordered_map<std::string, std::string> SIMULATOR_NMPI_MAP = {
    {"nmmc1", "SpiNNaker"},
    {"nmpm1", "BrainScaleS"},
    {"ess", "BrainScaleS-ESS"},
    {"spikey", "Spikey"},
};

/**
 * Map between the canonical system names and the
 */
static const std::unordered_map<std::string, SystemProperties>
    SIMULATOR_PROPERTIES = {{"nest", {false, false, false}},
                            {"ess", {true, true, true}},
                            {"nmmc1", {false, true, false}},
                            {"nmpm1", {true, true, false}},
                            {"spikey", {true, true, false}}};

/**
 * Map containing some default setup for the simulators.
 */
static const std::unordered_map<std::string, Json> DEFAULT_SETUPS = {
    {"nest", Json::object()},
    {"ess", {{"neuron_size", 4}}},
    {"nmmc1", {{"timestep", 1.0}}},
    {"nmpm1", {{"neuron_size", 4}}},
    {"spikey", Json::object()}};

/**
 * Map containing the supported neuron types per simulator.
 */
static const std::unordered_map<std::string,
                                std::unordered_set<const NeuronType *>>
    SUPPORTED_NEURON_TYPE_MAP = {
        {"nmmc1", {&SpikeSourceArray::inst(), &IfCondExp::inst()}},
        {"nmpm1",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(),
          &EifCondExpIsfaIsta::inst()}},
        {"ess",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(),
          &EifCondExpIsfaIsta::inst()}},
        {"spikey", {&SpikeSourceArray::inst(), &IfFacetsHardware1::inst()}},
        {"__default__",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(),
          &EifCondExpIsfaIsta::inst()}}};

/**
 * Static class used to lookup information about the PyNN simulations.
 */
class PyNNUtil {
private:
	std::mutex util_mutex;
	std::unordered_map<std::string, std::unique_ptr<std::shared_future<bool>>>
	    m_import_map;

	std::shared_future<bool> check_python_command(const std::string &cmd)
	{
		return std::async(std::launch::async, [cmd]() {
			return std::get<0>(Process::exec("python", {"-c", cmd})) == 0;
		});
	}

	auto &init_future(std::unique_ptr<std::shared_future<bool>> &tar,
	                  const std::string &cmd)
	{
		if (!tar) {
			tar = std::make_unique<std::shared_future<bool>>(
			    check_python_command(cmd));
		}
		return tar;
	}

	static std::string lower(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

public:
	/**
	 * Checks whether the given Python imports are valid. Performs the checks
	 * for multiple imports in parallel to save some time. Furthermore, the
	 * results of this method are cached.
	 *
	 * @param imports is a list of Python package names that should be imported.
	 * @return a vector of bool corresponding to each element in the "imports"
	 * list, specifying whether the import exists or not.
	 */
	std::vector<bool> has_imports(const std::vector<std::string> &imports)
	{
		std::lock_guard<std::mutex> lock(util_mutex);
		std::vector<bool> res;
		for (const std::string &import : imports) {
			auto res = m_import_map.emplace(import, nullptr);
			if (res.second) {
				init_future(res.first->second, std::string("import ") + import);
			}
		}
		for (const std::string &import : imports) {
			res.push_back(m_import_map.find(import)->second->get());
		}
		return res;
	}

	/**
	 * Like has_import, but only checks for a single import, which is less
	 * efficient.
	 *
	 * @param import is the Python package name that should be checked.
	 * @return true if the package is found, false otherwise.
	 */
	bool has_import(const std::string &import)
	{
		return has_imports({import})[0];
	}

	/**
	 * Looks up a canonical simulator name for the given simulator/PyNN package
	 * name and a list of possible imports.
	 *
	 * @param simulator is the simulator name as provided by the user.
	 * @return a pair consisting of the canonical simulator name and a list of
	 * possible imports.
	 */
	static std::pair<std::string, std::vector<std::string>> lookup_simulator(
	    std::string simulator)
	{
		// Strip the "pyNN."
		if (lower(simulator.substr(0, 5)) == "pynn.") {
			simulator = simulator.substr(5, simulator.size());
		}

		// Create the import list -- always try to load "pyNN.<simulator>" first
		std::vector<std::string> imports;
		imports.emplace_back(std::string("pyNN.") + simulator);

		// Check whether the simulator actually referred to a known import -- if
		// yes, translate it back to a canonical simulator name
		{
			auto it = NORMALISED_SIMULATOR_NAMES.find(lower(simulator));
			if (it != NORMALISED_SIMULATOR_NAMES.end()) {
				simulator = it->second;
			}
		}

		// Check whether the given simulator is present in the
		// SIMULATOR_IMPORT_MAP -- if yes, add the corresponding import to the
		// import list
		{
			auto it = SIMULATOR_IMPORT_MAP.find(simulator);
			if (it != SIMULATOR_IMPORT_MAP.end() &&
			    std::find(imports.begin(), imports.end(), it->second) ==
			        imports.end()) {
				imports.emplace_back(it->second);
			}
		}

		// Return the simulator name and the corresponding imports
		return std::make_pair(simulator, imports);
	}

	/**
	 * Returns properties of the chosen simulator.
	 */
	static SystemProperties properties(const std::string &normalised_simulator)
	{
		auto it = SIMULATOR_PROPERTIES.find(normalised_simulator);
		if (it != SIMULATOR_PROPERTIES.end()) {
			return it->second;
		}
		return SystemProperties(false, false, false);
	}
};

/**
 * Static instance of PyNNUtil which caches lookups for available Python
 * imports.
 */
static PyNNUtil PYNN_UTIL;
}

PyNN::PyNN(const std::string &simulator, const Json &setup)
    : m_simulator(simulator), m_setup(setup)
{
	// Lookup the canonical simulator name and the
	auto res = PYNN_UTIL.lookup_simulator(simulator);
	m_normalised_simulator = res.first;
	m_imports = res.second;

	// Merge the default setup with the user-provided setup
	auto it = DEFAULT_SETUPS.find(m_normalised_simulator);
	if (it != DEFAULT_SETUPS.end()) {
		m_setup = it->second;
	}
	join(m_setup, setup);  // Ensures m_setup is not null

	// Read the keep_log flag from the setup. Do not pass it to the PyNN
	// backend.
	m_keep_log = false;
	if (m_setup.count("keep_log") > 0 && m_setup["keep_log"]) {
		m_keep_log = true;
		m_setup.erase("keep_log");
	}
}

PyNN::~PyNN() = default;

float PyNN::timestep()
{
	SystemProperties props = PyNNUtil::properties(m_normalised_simulator);
	if (props.analogue()) {
		return 0.0;  // No minimum timestep on analogue neuromorphic hardware
	}
	return m_setup.value("timestep", 0.1);  // Default is 0.1ms
}

std::unordered_set<const NeuronType *> PyNN::supported_neuron_types() const
{
	auto it = SUPPORTED_NEURON_TYPE_MAP.find(m_normalised_simulator);
	if (it != SUPPORTED_NEURON_TYPE_MAP.end()) {
		return it->second;
	}
	return SUPPORTED_NEURON_TYPE_MAP.find("__default__")->second;
}

void PyNN::do_run(NetworkBase &source, float duration) const
{
	// Find the import that should be used
	std::vector<bool> available = PYNN_UTIL.has_imports(m_imports);
	std::string import;
	for (size_t i = 0; i < available.size(); i++) {
		if (available[i]) {
			import = m_imports[i];
			break;
		}
	}
	if (import.empty()) {
		std::stringstream ss;
		ss << "The simulator \"" << m_simulator << "\" with canonical name \""
		   << m_normalised_simulator << "\" was not found on this system or an "
		                                "error occured during the import. The "
		                                "following imports were tried: ";
		for (size_t i = 0; i < m_imports.size(); i++) {
			if (i > 0) {
				ss << ", ";
			}
			ss << m_imports[i];
		}
		throw PyNNSimulatorNotFound(ss.str());
	}

	// Generate the error log filename
	std::string log_path = ".cypress_err_" + m_normalised_simulator + "_XXXXXX";

	// Run the PyNN python backend
	{
#ifndef CYPRESS_DEBUG_BINNF
		std::vector<std::string> params(
		    {Resources::PYNN_INTERFACE.open(), "run", "--simulator",
		     m_normalised_simulator, "--library", import, "--setup",
		     m_setup.dump(), "--duration", std::to_string(duration)});
#else
		std::vector<std::string> params(
		    {Resources::PYNN_INTERFACE.open(), "dump"});
#endif
		Process proc("python", params);

// Attach the error log
#ifndef CYPRESS_DEBUG_BINNF
		std::ofstream log_stream(filesystem::tmpfile(log_path));
		std::thread log_thread(Process::generic_pipe,
		                       std::ref(proc.child_stderr()),
		                       std::ref(log_stream));
#else
		std::thread log_thread(Process::generic_pipe,
		                       std::ref(proc.child_stdout()),
		                       std::ref(std::cout));
#endif

		// Send the network description to the simulator, inject the connection
		// transformation to rewrite the connections
		binnf::marshall_network(source, proc.child_stdin());
		proc.close_child_stdin();

// Wait for the process to be done
#ifndef CYPRESS_DEBUG_BINNF
		if ((!binnf::marshall_response(source, proc.child_stdout())) |
		    (proc.wait() != 0)) {
			log_thread.join();  // Make sure the logging thread has finished

			std::ifstream log_stream_in(log_path);
			Process::generic_pipe(log_stream_in, std::cerr);
			throw ExecutionError(
			    std::string("Error while executing the simulator, see ") +
			    log_path + " for the above information.");
		}
#endif

		// Make sure the logging thread has finished
		log_thread.join();
	}

	// Remove the log file
	if (!m_keep_log) {
		unlink(log_path.c_str());
	}
}

std::string PyNN::nmpi_platform() const
{
	auto it = SIMULATOR_NMPI_MAP.find(m_normalised_simulator);
	if (it != SIMULATOR_NMPI_MAP.end()) {
		return it->second;
	}
	return std::string();
}

std::vector<std::string> PyNN::simulators()
{
	// Split the simulator import map into the simulators and the corresponding
	// imports
	std::vector<std::string> simulators;
	std::vector<std::string> imports;
	for (auto &e : SIMULATOR_IMPORT_MAP) {
		simulators.emplace_back(e.first);
		imports.emplace_back(e.second);
	}

	// Check which imports are available
	std::vector<bool> available = PYNN_UTIL.has_imports(imports);
	std::vector<std::string> res;
	for (size_t i = 0; i < available.size(); i++) {
		if (available[i]) {
			res.push_back(simulators[i]);
		}
	}
	return res;
}
}
