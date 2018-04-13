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
#include <cerrno>
#include <fstream>
#include <future>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pybind11/embed.h"
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"  //Type conversions

#include <cypress/backend/pynn/pynn2.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>

// Enable to get a textual dump of the BiNNF instead of running the simulation
//#define CYPRESS_DEBUG_BINNF

namespace cypress {
namespace py = pybind11;
using namespace py::literals;
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
 * Map containing connections
 */
static const std::unordered_map<std::string, std::string>
    SUPPORTED_CONNECTIONS = {
        {"AllToAllConnector", "AllToAllConnector"},
        {"OneToOneConnector", "OneToOneConnector"},
        {"FixedFanInConnector", "FixedNumberPreConnector"},
        {"FixedFanOutConnector", "FixedNumberPostConnector"},
        {"RandomConnector", "FixedProbabilityConnector"}};

/**
 * Static class used to lookup information about the PyNN simulations.
 */
class PyNNUtil {
private:
	std::mutex util_mutex;

	bool check_python_import(const std::string &cmd)
	{
		bool ret = false;
		try {
			if (cmd == "pyNN.nest") {
				py::module sys = py::module::import("sys");
				auto a = py::list();
				a.append("pynest");
				sys.attr("argv") = a;
				py::module::import(cmd.c_str());
				a[0] = "";
				sys.attr("argv") = a;
			}
			else {
				py::module::import(cmd.c_str());
			}
			ret = true;
		}
		catch (...) {
		}
		return ret;
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
		for (auto i : imports) {
			bool temp = check_python_import(i);
			res.push_back(temp);
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
}  // namespace

PyNN_::PyNN_(const std::string &simulator, const Json &setup)
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

	// Delete config option for SLURM
	m_setup.erase("slurm_mode");
	m_setup.erase("slurm_filename");
	m_setup.erase("station");
}

PyNN_::~PyNN_() = default;

Real PyNN_::timestep()
{
	SystemProperties props = PyNNUtil::properties(m_normalised_simulator);
	if (props.analogue()) {
		return 0.0;  // No minimum timestep on analogue neuromorphic hardware
	}
	return m_setup.value("timestep", 0.1);  // Default is 0.1ms
}

std::unordered_set<const NeuronType *> PyNN_::supported_neuron_types() const
{
	auto it = SUPPORTED_NEURON_TYPE_MAP.find(m_normalised_simulator);
	if (it != SUPPORTED_NEURON_TYPE_MAP.end()) {
		return it->second;
	}
	return SUPPORTED_NEURON_TYPE_MAP.find("__default__")->second;
}

std::string PyNN_::get_import(const std::vector<std::string> &imports,
                              const std::string &simulator)
{
	// Find the import that should be used
	std::vector<bool> available = PYNN_UTIL.has_imports(imports);
	std::string import;

	for (size_t i = 0; i < available.size(); i++) {
		if (available[i]) {
			import = imports[i];
			break;
		}
	}

	auto normalised_simulator = PYNN_UTIL.lookup_simulator(simulator).first;
	if (import.empty()) {
		std::stringstream ss;
		ss << "The simulator \"" << simulator << "\" with canonical name \""
		   << normalised_simulator
		   << "\" was not found on this system or an "
		      "error occured during the import. The "
		      "following imports were tried: ";
		for (size_t i = 0; i < imports.size(); i++) {
			if (i > 0) {
				ss << ", ";
			}
			ss << imports[i];
		}
		throw PyNNSimulatorNotFound(ss.str());
	}
	return import;
}

/**
 * Converting a Json object to a py::dict. Make sure that the python interpreter
 * is already started before calling this function
 *
 * @param json The json object which will be converted
 * @return python dictionary
 */
py::dict json_to_dict(Json json)
{
	py::dict dict;
	for (Json::iterator i = json.begin(); i != json.end(); ++i) {
		auto key = py::arg(i.key().c_str());
		Json value = i.value();
		if (value.is_object()) {
			dict = py::dict(**json_to_dict(value), **dict);
		}
		else if (value.is_string()) {
			std::string temp = value;
			dict = py::dict(key = temp, **dict);
		}
		else if (value.is_array()) {  // structured
			py::list list;
			for (auto i : value) {
				if (value.is_number_float()) {
					list.append(float(value));
				}
				else if (value.is_boolean()) {
					list.append(bool(value));
				}
				else if (value.is_number_integer() or
				         value.is_number_unsigned()) {
					list.append(int(value));
				}
				else if (value.is_string()) {
					std::string temp = value;
					list.append(temp);
				}
			}
			dict = py::dict(key = list, **dict);
		}
		else if (value.is_boolean()) {
			bool temp = value;
			dict = py::dict(key = temp, **dict);
		}
		else if (value.is_number_float()) {
			float temp = value;
			dict = py::dict(key = temp, **dict);
		}
		else if (value.is_number_integer() or value.is_number_unsigned()) {
			int temp = value;
			dict = py::dict(key = temp, **dict);
		}
		else {
			global_logger().info(
			    "cypress",
			    "Ignoring key " + i.key() + "in json to py:dict conversion");
		}
	}
	return dict;
}

/**
 * Given a NeuronType, this function returns the name of the PyNN neuron_type
 *
 * @param neuron_type Reference to the NeuronType of the population
 * @return PyNN class name of the neuron type
 */
std::string get_neuron_class(const NeuronType &neuron_type)
{
	if (&neuron_type == &IfCondExp::inst()) {
		return "IF_cond_exp";
	}
	else if (&neuron_type == &IfFacetsHardware1::inst()) {
		return "IF_facets_hardware1";
	}
	else if (&neuron_type == &EifCondExpIsfaIsta::inst()) {
		return "EIF_cond_exp_isfa_ista";
	}
	else if (&neuron_type == &SpikeSourceArray::inst()) {
		return "SpikeSourceArray";
	}
	else {
		throw NotSupportedException("This neuron type is not supported yet!");
	}
}

py::object get_pop_view(py::module pynn, py::object py_pop,
                        PopulationBase c_pop, size_t start, size_t end)
{
	if (start == 0 && end == c_pop.size()) {
		return py_pop;
	}
	else {
		return pynn.attr("PopulationView")(py_pop, py::slice(start, end, 1));
	}
}

/**
 * Creates a PyNN 0.9 Connector
 *
 * @param connector_name name of the pyNN connector
 * @param pynn Reference to the pynn module
 * @param additional_parameter Additional parameter needed by the connector
 * @return py::object reference to the connector
 */
py::object get_connector(const std::string &connector_name,
                         const py::module &pynn,
                         const Real &additional_parameter)
{
	/* OLD pyNN connector
	        if (it->second == "AllToAllConnector") {
	            pyconnector = pynn.attr(it->second.c_str())(
	                "weights"_a = temp.synapse.weight,
	                "delays"_a = temp.synapse.delay,
	                "allow_self_connections"_a = py::cast(false));
	        }
	        else if (it->second == "OneToOneConnector") {
	            pyconnector = pynn.attr(it->second.c_str())(
	                "weights"_a = temp.synapse.weight,
	                "delays"_a = temp.synapse.delay);
	        }
	        else if (it->second == "FixedProbabilityConnector") {
	            pyconnector = pynn.attr(it->second.c_str())(
	                "weights"_a = temp.synapse.weight,
	                "delays"_a = temp.synapse.delay,
	                "p_connect"_a = temp.additional_parameter,
	                "allow_self_connections"_a = py::cast(false));
	        }
	        else if (it->second == "FixedNumberPreConnector") {
	            pyconnector = pynn.attr(it->second.c_str())(
	                "weights"_a = temp.synapse.weight,
	                "delays"_a = temp.synapse.delay,
	                "n"_a = int(temp.additional_parameter),
	                "allow_self_connections"_a = py::cast(false));
	        }
	        else if (it->second == "FixedNumberPostConnector") {
	            pyconnector = pynn.attr(it->second.c_str())(
	                "weights"_a = temp.synapse.weight,
	                "delays"_a = temp.synapse.delay,
	                "n"_a = int(temp.additional_parameter),
	                "allow_self_connections"_a = py::cast(false));
	        }*/

	if (connector_name == "AllToAllConnector") {
		return pynn.attr(connector_name.c_str())("allow_self_connections"_a =
		                                             py::cast(false));
	}
	else if (connector_name == "OneToOneConnector") {
		return pynn.attr(connector_name.c_str())();
	}
	else if (connector_name == "FixedProbabilityConnector") {
		return pynn.attr(connector_name.c_str())(
		    "p_connect"_a = additional_parameter,
		    "allow_self_connections"_a = py::cast(false));
	}
	else if (connector_name == "FixedNumberPreConnector") {
		return pynn.attr(connector_name.c_str())(
		    "n"_a = int(additional_parameter),
		    "allow_self_connections"_a = py::cast(false));
	}
	else if (connector_name == "FixedNumberPostConnector") {
		return pynn.attr(connector_name.c_str())(
		    "n"_a = int(additional_parameter),
		    "allow_self_connections"_a = py::cast(false));
	}
}

void PyNN_::do_run(NetworkBase &source, Real duration) const
{
	// TODO : LOGGer
	py::scoped_interpreter guard;
	std::string import = get_import(m_imports, m_simulator);

	auto start = std::chrono::system_clock::now();
	// Bug when importing NEST
	if (import == "pyNN.nest") {
		py::module sys = py::module::import("sys");
		auto a = py::list();
		a.append("pynest");
		sys.attr("argv") = a;
	}

	// Setup simulator
	py::module pynn = py::module::import(import.c_str());
	auto dict = json_to_dict(m_setup);
	pynn.attr("setup")(**dict);

	// Create populations
	auto setup = std::chrono::system_clock::now();
	const std::vector<PopulationBase> &populations = source.populations();
	std::vector<py::object> pypopulations;
	// py::print("Population");
	for (size_t i = 0; i < populations.size(); i++) {
		if (populations.size() == 0) {
			continue;
		}

		if (&populations[i].type() == &SpikeSourceArray::inst()) {
			// This covers all spike sources!

			// Create Spike Source
			py::dict neuron_params;
			py::list spikes;
			for (auto neuron : populations[i]) {
				const std::vector<Real> &temp =
				    neuron.parameters().parameters();
				spikes.append(py::array_t<Real>({temp.size()}, {sizeof(Real)},
				                                temp.data()));
			}

			auto neuron_type =
			    pynn.attr("SpikeSourceArray")("spike_times"_a = spikes);

			pypopulations.push_back(pynn.attr("Population")(
			    "size"_a = populations[i].size(), "cellclass"_a = neuron_type));
		}
		else {
			// TODO INITIAL VALUES ?
			bool homogeneous = populations[i].homogeneous_parameters();

			py::dict neuron_params;
			const auto &params = populations[i][0].parameters();
			const auto &param_names = populations[i].type().parameter_names;
			for (size_t j = 0; j < param_names.size(); j++) {
				neuron_params =
				    py::dict(py::arg(param_names[j].c_str()) = params[j],
				             **neuron_params);
			}
			pypopulations.push_back(pynn.attr("Population")(
			    "size"_a = populations[i].size(),
			    "cellclass"_a =
			        pynn.attr(get_neuron_class(populations[i].type()).c_str())(
			            **neuron_params)));
			if (!homogeneous) {
				for (size_t id = 0; id < params.size(); id++) {
					std::vector<Real> new_params;
					for (auto neuron : populations[i]) {
						new_params.push_back(neuron.parameters()[i]);
					}
					pypopulations.back().attr("set")(
					    py::arg(param_names[id].c_str()) = py::array_t<Real>(
					        {new_params.size()}, {sizeof(Real)},
					        new_params.data()));
				}
			}
			const bool homogeneous_rec = populations[i].homogeneous_record();
			if (homogeneous_rec) {
				std::vector<std::string> signals =
				    populations[i].type().signal_names;
				for (size_t j = 0; j < signals.size(); j++) {
					if (populations[i].signals().is_recording(j)) {
						pypopulations.back().attr("record")(signals[j].c_str());
					}
				}
			}
			else {
				std::vector<std::string> signals =
				    populations[i].type().signal_names;
				for (size_t j = 0; j < signals.size(); j++) {
					std::vector<size_t> neuron_ids;
					for (size_t k = 0; k < populations[i].size(); k++) {
						if (populations[i][k].signals().is_recording(j)) {
							neuron_ids.push_back(k);
						}
					}
					py::object popview = pynn.attr("PopulationView")(
					    pypopulations.back(),
					    py::array_t<size_t>({neuron_ids.size()},
					                        {sizeof(size_t)},
					                        neuron_ids.data()));
					popview.attr("record")(signals[j].c_str());
				}
			}
		}
	}

	auto buildpop = std::chrono::system_clock::now();
	for (auto conn : source.connections()) {
		auto it = SUPPORTED_CONNECTIONS.find(conn.connector().name());
		GroupConnction group_conn;
		if (it != SUPPORTED_CONNECTIONS.end() &&
		    conn.connector().group_connect(conn, group_conn)) {
			// Group connections
			py::object source = get_pop_view(
			    pynn, pypopulations[conn.pid_src()],
			    populations[conn.pid_src()], group_conn.src0, group_conn.src1);

			py::object target = get_pop_view(
			    pynn, pypopulations[conn.pid_tar()],
			    populations[conn.pid_tar()], group_conn.tar0, group_conn.tar1);

			std::string receptor = "excitatory";
			if (group_conn.synapse.weight < 0) {
				receptor = "inhibitory";
			}
			auto proj = pynn.attr("Projection")(
			    source, target,
			    get_connector(it->second, pynn,
			                  group_conn.additional_parameter),
			    "synapse_type"_a = pynn.attr("StaticSynapse")(
			        "weight"_a = abs(group_conn.synapse.weight),
			        "delay"_a = group_conn.synapse.delay),
			    "receptor_type"_a = receptor);
		}
		else {
			std::vector<Connection> conns_full;
			size_t num_inh = 0;
			conn.connect(conns_full);
			for (auto i : conns_full) {
				if (i.n.synapse.weight < 0) {
					num_inh++;
				}
			}

			Matrix<Real> conns_exc(conns_full.size() - num_inh, 4),
			    conns_inh(num_inh, 4);
			size_t counter_ex = 0, counter_in = 0;
			Real timestep = 0;
			if (m_simulator == "nest") {
				global_logger().info(
				    "cypress",
				    "Delays are rounded to multiples of the timestep");
				timestep = py::cast<Real>(pynn.attr("get_time_step")());
			}
			for (auto i : conns_full) {
				if (i.n.synapse.weight > 0) {
					conns_exc(counter_ex, 0) = i.n.src;
					conns_exc(counter_ex, 1) = i.n.tar;
					conns_exc(counter_ex, 2) = i.n.synapse.weight;
					if (timestep != 0) {
						Real delay =
						    std::max(round(i.n.synapse.delay / timestep), 1.0) *
						    timestep;
						conns_exc(counter_ex, 3) = delay;
					}
					else {
						conns_exc(counter_ex, 3) = i.n.synapse.delay;
					}
					counter_ex++;
				}
				else {
					conns_inh(counter_in, 0) = i.n.src;
					conns_inh(counter_in, 1) = i.n.tar;
					conns_inh(counter_in, 2) = -i.n.synapse.weight;
					if (timestep != 0) {
						;
						Real delay =
						    std::max(round(i.n.synapse.delay / timestep), 1.0) *
						    timestep;
						conns_inh(counter_in, 3) = delay;
					}
					else {
						conns_inh(counter_in, 3) = i.n.synapse.delay;
					}
					counter_in++;
				}
			}

			if (conns_full.size() - num_inh > 0) {
				py::detail::any_container<ssize_t> shape(
				    {static_cast<long>(conns_exc.rows()), 4});
				py::array_t<Real> temp(shape, {4 * sizeof(Real), sizeof(Real)},
				                       conns_exc.data());
				py::object connector = pynn.attr("FromListConnector")(temp);
				pynn.attr("Projection")(
				    pypopulations[conn.pid_src()],
				    pypopulations[conn.pid_tar()], connector,
				    "synapse_type"_a = pynn.attr("StaticSynapse")(
				        "weight"_a = 0, "delay"_a = 1),
				    "receptor_type"_a = "excitatory");
			}
			if (num_inh > 0) {
				py::detail::any_container<ssize_t> shape(
				    {static_cast<long>(conns_inh.rows()), 4});
				py::array_t<Real> temp(shape, {4 * sizeof(Real), sizeof(Real)},
				                       conns_inh.data());
				py::object connector = pynn.attr("FromListConnector")(temp);
				pynn.attr("Projection")(
				    pypopulations[conn.pid_src()],
				    pypopulations[conn.pid_tar()], connector,
				    "synapse_type"_a = pynn.attr("StaticSynapse")(
				        "weight"_a = 0, "delay"_a = 1),
				    "receptor_type"_a = "inhibitory");
			}
		}
	}

	auto buildconn = std::chrono::system_clock::now();
	pynn.attr("run")(duration);

	auto execrun = std::chrono::system_clock::now();

	// fetch spikes
	auto get_data = std::chrono::system_clock::now();
	for (size_t i = 0; i < populations.size(); i++) {
		std::vector<std::string> signals = populations[i].type().signal_names;
		if (m_simulator == "nest") {
			for (size_t j = 0; j < signals.size(); j++) {
				if (populations[i].signals().is_recording(j)) {
					if (signals[j] == "spikes") {
						py::module nest = py::module::import("nest");
						py::dict nest_data = py::tuple(
						    nest.attr("GetStatus")(pypopulations[i]
						                               .attr("recorder")
						                               .attr("_spike_detector")
						                               .attr("device"),
						                           "events"))[0];
						py::buffer buffer(nest_data["senders"].attr("data"));
						py::buffer_info info = buffer.request();
						Matrix<int64_t> neuron_ids(
						    info.shape[0], info.ndim,
						    reinterpret_cast<int64_t *>(info.ptr), true);

						py::buffer buffer2(nest_data["times"].attr("data"));
						py::buffer_info info2 = buffer2.request();

						Matrix<double> spikes(
						    info2.shape[0], info2.ndim,
						    reinterpret_cast<double *>(info2.ptr), true);

						size_t offset = py::cast<size_t>(
						    py::list(pypopulations[i].attr("all_cells"))[0]);
						for (size_t k = 0; k < populations[i].size(); k++) {
							auto neuron = populations[i][k];
							auto idx = neuron.type().signal_index("spikes");
							size_t len =
							    std::count(neuron_ids.begin(), neuron_ids.end(),
							               k + offset);
							auto data = std::make_shared<Matrix<Real>>(len, 1);
							size_t counter = 0;
							for (size_t l = 0; l < neuron_ids.size(); l++) {
								if (size_t(neuron_ids[l]) == k + offset) {
									assert(counter < len);
									(*data)(counter, 0) = spikes[l];
									counter++;
								}
							}
							neuron.signals().data(idx.value(), std::move(data));
						}
					}
					else {
                        
						py::module nest = py::module::import("nest");
                        /*SCALE_FACTORS = {'v': 1, 'gsyn_exc': 0.001, 'gsyn_inh': 0.001}
                        VARIABLE_MAP = {'v': 'V_m', 'gsyn_exc': 'g_ex', 'gsyn_inh': 'g_in'}*/
                        py::dict nest_data = py::tuple(
						    nest.attr("GetStatus")(pypopulations[i]
						                               .attr("recorder")
                                                       .attr("_multimeter")
                                                       .attr("device"),
						                           "events"))[0];
                                                   
                        //which neuron
                        py::buffer buffer(nest_data["senders"].attr("data"));
                        //when
                        py::buffer buffer2(nest_data["times"].attr("data"));
                        // actual data
                        py::buffer buffer3(nest_data["V_m"].attr("data"));
						std::cerr << "not implemented" << std::endl;
						throw;
					}
				}
			}
		}
		else {
			py::object neo_block = pypopulations[i].attr("get_data")();
			get_data = std::chrono::system_clock::now();
			for (size_t j = 0; j < signals.size(); j++) {
				if (populations[i].signals().is_recording(j)) {
					if (signals[j] == "spikes") {
						if (m_simulator != "nest") {
							py::list spiketrains =
							    (py::list(neo_block.attr("segments"))[0])
							        .attr("spiketrains");
							size_t len = py::len(spiketrains);
							for (size_t k = 0; k < len; k++) {
								size_t index =
								    py::cast<size_t>(spiketrains[k].attr(
								        "annotations")["source_index"]);
								auto neuron = populations[i][index];
								auto idx = neuron.type().signal_index("spikes");
								// number of spikes
								std::vector<Real> spikes =
								    py::cast<std::vector<Real>>(spiketrains[k]);
								auto data = std::make_shared<Matrix<Real>>(
								    spikes.size(), 1);

								for (size_t i = 0; i < spikes.size(); i++) {
									(*data)(i, 0) = spikes[i];
								}
								neuron.signals().data(idx.value(),
								                      std::move(data));
							}
						}
						else {
							// TODO
							std::cerr << "not implemented" << std::endl;
							throw;
						}
					}
				}
			}
		}
	}
	auto finished = std::chrono::system_clock::now();
	std::cout << "setup "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(setup -
	                                                                   start)
	                 .count()
	          << std::endl;
	std::cout << "buildpop "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 buildpop - setup)
	                 .count()
	          << std::endl;
	std::cout << "buildconn "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 buildconn - buildpop)
	                 .count()
	          << std::endl;
	std::cout << "execrun "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 execrun - buildconn)
	                 .count()
	          << std::endl;
	std::cout << "get_data "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 get_data - execrun)
	                 .count()
	          << std::endl;
	std::cout << "finished "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 finished - get_data)
	                 .count()
	          << std::endl;
	pynn.attr("end")();

	/*7
	        // Send the network description to the simulator
	        binnf::marshall_network(source, proc.child_stdin());


	        int res;

	        // Keep track of log messages, at the end read results.
	        if ((!binnf::marshall_response(source, std_res)) |
	            ((res = proc.wait()) != 0)) {
	            err_thread.join();

	            // Explicitly state if the process was killed by a signal
	            if (res < 0) {
	                source.logger().error(
	                    "cypress",
	                    "Simulator child process killed by signal " +
	                        std::to_string(-res));
	            }

	            // Only dump stderr if no error message has been logged
	            if (source.logger().count(LogSeverity::ERROR) == 0) {
	                std::ifstream log_stream_in(log_path);
	                Process::generic_pipe(log_stream_in, std::cerr);
	                throw ExecutionError(
	                    std::string("Error while executing the simulator,
	see ")
	+ log_path + " for the above information");
	            }
	            else {
	                throw ExecutionError(
	                    std::string("Error while executing the simulator,
	see ")
	+ log_path + " for the simulators stderr output");
	            }

	#endif
	    }
	    // Remove the log file
	    if (!m_keep_log) {
	        unlink(log_path.c_str());
	    }*/
}  // namespace cypress

std::string PyNN_::nmpi_platform() const
{
	auto it = SIMULATOR_NMPI_MAP.find(m_normalised_simulator);
	if (it != SIMULATOR_NMPI_MAP.end()) {
		return it->second;
	}
	return std::string();
}

std::vector<std::string> PyNN_::simulators()
{
	// Split the simulator import map into the simulators and the
	// corresponding imports
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
}  // namespace cypress
