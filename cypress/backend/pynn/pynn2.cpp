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

#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>  //Type conversions

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
 * Map from cypress/PyNN recording variables to NEST variables
 */
static const std::unordered_map<std::string, std::string>
    NEST_RECORDING_VARIABLES = {
        {"v", "V_m"}, {"gsyn_exc", "g_ex"}, {"gsyn_inh", "g_in"}};
/**
 * Map from cypress/PyNN recording variables to NEST scales (NEST uses different
 * units to record)
 */
static const std::unordered_map<std::string, Real> NEST_RECORDING_SCALES = {
    {"v", 1}, {"gsyn_exc", 0.001}, {"gsyn_inh", 0.001}};

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
				a.append("--quiet");
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

py::dict PyNN_::json_to_dict(Json json)
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
std::string PyNN_::get_neuron_class(const NeuronType &neuron_type)
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

void PyNN_::init_logger()
{
	py::module logger = py::module::import("logging");
	int32_t loglevel = global_logger().min_level();
	logger.attr("getLogger")("PyNN").attr("setLevel")(loglevel);
	logger.attr("getLogger")("").attr("setLevel")(loglevel);
}

py::object PyNN_::create_source_population(const PopulationBase &pop,
                                           py::module &pynn)
{
	// This covers all spike sources!

	// Create Spike Source
	py::dict neuron_params;
	py::list spikes;
	for (auto neuron : pop) {
		const std::vector<Real> &temp = neuron.parameters().parameters();
		spikes.append(
		    py::array_t<Real>({temp.size()}, {sizeof(Real)}, temp.data()));
	}

	auto neuron_type = pynn.attr("SpikeSourceArray")("spike_times"_a = spikes);

	py::module numpy = py::module::import("numpy");
	return pynn.attr("Population")("size"_a = numpy.attr("int")(pop.size()),
	                               "cellclass"_a = neuron_type);
}

py::object PyNN_::create_homogeneous_pop(const PopulationBase &pop,
                                         py::module &pynn, bool &init_available)
{
	py::dict neuron_params;
	const auto &params = pop[0].parameters();
	const auto &param_names = pop.type().parameter_names;
	for (size_t j = 0; j < param_names.size(); j++) {
		neuron_params = py::dict(py::arg(param_names[j].c_str()) = params[j],
		                         **neuron_params);
	}

	py::module numpy = py::module::import("numpy");
	py::object pypop = pynn.attr("Population")(
	    "size"_a = numpy.attr("int")(pop.size()),
	    "cellclass"_a =
	        pynn.attr(get_neuron_class(pop.type()).c_str())(**neuron_params));
	init_available = false;
	try {
		auto idx = pop[0].type().signal_index("v_rest");
		if (idx.valid()) {
			pypop.attr("initialize")(**py::dict("v"_a = params[idx.value()]));
			init_available = true;
		}
	}
	catch (...) {
		init_available = false;
		global_logger().warn("cypress",
		                     "Backend does not support explicit "
		                     "initialization of the membrane "
		                     "potential!");
	}
	return pypop;
}

void PyNN_::set_inhomogeneous_parameters(const PopulationBase &pop,
                                         py::object &pypop, bool init_available)
{
	auto idx = pop[0].type().signal_index("v_rest");
	const auto &params = pop[0].parameters();
	const auto &param_names = pop.type().parameter_names;

	for (size_t id = 0; id < params.size(); id++) {
		std::vector<Real> new_params;
		for (auto neuron : pop) {
			new_params.push_back(neuron.parameters()[id]);
		}
		pypop.attr("set")(
		    py::arg(param_names[id].c_str()) = py::array_t<Real>(
		        {new_params.size()}, {sizeof(Real)}, new_params.data()));
		if (init_available && id == idx.value()) {
			pypop.attr("initialize")(**py::dict(
			    "v"_a = py::array_t<Real>({new_params.size()}, {sizeof(Real)},
			                              new_params.data())));
		}
	}
}

void PyNN_::set_homogeneous_rec(const PopulationBase &pop, py::object &pypop)
{
	std::vector<std::string> signals = pop.type().signal_names;
	for (size_t j = 0; j < signals.size(); j++) {
		if (pop.signals().is_recording(j)) {
			pypop.attr("record")(signals[j].c_str());
		}
	}
}

void PyNN_::set_inhomogenous_rec(const PopulationBase &pop, py::object &pypop,
                                 py::module &pynn)
{
	std::vector<std::string> signals = pop.type().signal_names;
	for (size_t j = 0; j < signals.size(); j++) {
		std::vector<size_t> neuron_ids;
		for (size_t k = 0; k < pop.size(); k++) {
			if (pop[k].signals().is_recording(j)) {
				neuron_ids.push_back(k);
			}
		}
		py::object popview = pynn.attr("PopulationView")(
		    pypop, py::array_t<size_t>({neuron_ids.size()}, {sizeof(size_t)},
		                               neuron_ids.data()));
		popview.attr("record")(signals[j].c_str());
	}
}

py::object PyNN_::get_pop_view(const py::module &pynn, const py::object &py_pop,
                               const PopulationBase &c_pop, const size_t &start,
                               const size_t &end)
{
	if (start == 0 && end == c_pop.size()) {
		return py_pop;
	}
	else {
		try {
			return pynn.attr("PopulationView")(py_pop,
			                                   py::slice(start, end, 1));
		}
		catch (py::error_already_set) {
			throw NotSupportedException(
			    "Popviews are not supported by SpiNNaker");
		}
	}
}

py::object PyNN_::get_connector8(const std::string &connector_name,
                                 const GroupConnction group_conn,
                                 const py::module &pynn)
{
	if (connector_name == "AllToAllConnector") {
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = group_conn.synapse.weight,
		    "delays"_a = group_conn.synapse.delay,
		    "allow_self_connections"_a = py::cast(false));
	}
	else if (connector_name == "OneToOneConnector") {
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = group_conn.synapse.weight,
		    "delays"_a = group_conn.synapse.delay);
	}
	else if (connector_name == "FixedProbabilityConnector") {
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = group_conn.synapse.weight,
		    "delays"_a = group_conn.synapse.delay,
		    "p_connect"_a = group_conn.additional_parameter,
		    "allow_self_connections"_a = py::cast(false));
	}
	else if (connector_name == "FixedNumberPreConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = group_conn.synapse.weight,
		    "delays"_a = group_conn.synapse.delay,
		    "n"_a = np.attr("int")(group_conn.additional_parameter),
		    "allow_self_connections"_a = py::cast(false));
	}
	else if (connector_name == "FixedNumberPostConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = group_conn.synapse.weight,
		    "delays"_a = group_conn.synapse.delay,
		    "n"_a = np.attr("int")(group_conn.additional_parameter),
		    "allow_self_connections"_a = py::cast(false));
	}
	throw NotSupportedException("Requested group connection " + connector_name +
	                            " is not supported");
}

py::object PyNN_::get_connector(const std::string &connector_name,
                                const py::module &pynn,
                                const Real &additional_parameter)
{
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
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "n"_a = np.attr("int")(additional_parameter),
		    "allow_self_connections"_a = py::cast(false));
	}
	else if (connector_name == "FixedNumberPostConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "n"_a = np.attr("int")(additional_parameter),
		    "allow_self_connections"_a = py::cast(false));
	}
	throw NotSupportedException("Requested group connection " + connector_name +
	                            " is not supported");
}

py::object PyNN_::group_connect(const std::vector<PopulationBase> &populations,
                                const std::vector<py::object> &pypopulations,
                                const ConnectionDescriptor conn,
                                const GroupConnction &group_conn,
                                const py::module &pynn,
                                const std::string &conn_name,
                                const Real timestep)
{
	py::object source = get_pop_view(pynn, pypopulations[conn.pid_src()],
	                                 populations[conn.pid_src()],
	                                 group_conn.src0, group_conn.src1);

	py::object target = get_pop_view(pynn, pypopulations[conn.pid_tar()],
	                                 populations[conn.pid_tar()],
	                                 group_conn.tar0, group_conn.tar1);

	std::string receptor = "excitatory";
	if (group_conn.synapse.weight < 0) {
		receptor = "inhibitory";
	}
	return pynn.attr("Projection")(
	    source, target,
	    get_connector(conn_name, pynn, group_conn.additional_parameter),
	    "synapse_type"_a = pynn.attr("StaticSynapse")(
	        "weight"_a = abs(group_conn.synapse.weight),
	        "delay"_a =
	            std::max(round(group_conn.synapse.delay / timestep), 1.0) *
	            timestep),
	    "receptor_type"_a = receptor);
}

/**
 * Creates a PyNN FromList Connection
 *
 * @param pypopulations list of python populations
 * @param conn ConnectionDescriptor of the List connection
 * @param pynn Handler for PyNN python module
 * @param timestep Timestep of the simulator, default 0
 * @return tuple of the excitatory,inhibitory connection. List is separated for
 * compatibility reasons
 */
std::tuple<py::object, py::object> PyNN_::list_connect(
    const std::vector<py::object> &pypopulations,
    const ConnectionDescriptor conn, const py::module &pynn,
    const Real timestep)
{
	std::tuple<py::object, py::object> ret =
	    std::make_tuple(py::object(), py::object());
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
		std::get<0>(ret) = pynn.attr("Projection")(
		    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
		    connector,
		    "synapse_type"_a =
		        pynn.attr("StaticSynapse")("weight"_a = 0, "delay"_a = 1),
		    "receptor_type"_a = "excitatory");
	}
	if (num_inh > 0) {
		py::detail::any_container<ssize_t> shape(
		    {static_cast<long>(conns_inh.rows()), 4});
		py::array_t<Real> temp(shape, {4 * sizeof(Real), sizeof(Real)},
		                       conns_inh.data());
		py::object connector = pynn.attr("FromListConnector")(temp);
		std::get<1>(ret) = pynn.attr("Projection")(
		    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
		    connector,
		    "synapse_type"_a =
		        pynn.attr("StaticSynapse")("weight"_a = 0, "delay"_a = 1),
		    "receptor_type"_a = "inhibitory");
	}
	return ret;
}

template <typename T>
Matrix<T> PyNN_::matrix_from_numpy(py::object object)
{
	// Check the data type
	std::string type = py::cast<std::string>(object.attr("dtype").attr("name"));
	if (type == "int8") {
		assert(typeid(T) == typeid(int8_t));
	}
	else if (type == "int16") {
		assert(typeid(T) == typeid(int16_t));
	}
	else if (type == "int32") {
		assert(typeid(T) == typeid(int32_t));
	}
	else if (type == "int64") {
		assert(typeid(T) == typeid(int64_t));
	}
	else if (type == "uint8") {
		assert(typeid(T) == typeid(uint8_t));
	}
	else if (type == "uint16") {
		assert(typeid(T) == typeid(uint16_t));
	}
	else if (type == "uint32") {
		assert(typeid(T) == typeid(uint32_t));
	}
	else if (type == "uint64") {
		assert(typeid(T) == typeid(uint64_t));
	}
	else if (type == "float64") {
		assert(typeid(T) == typeid(double));
	}
	else if (type == "float32") {
		assert(typeid(T) == typeid(float));
	}
	else {
		throw;
	}

	// Get the data dimension
	std::vector<int> shape = py::cast<std::vector<int>>(object.attr("shape"));
	size_t second_dim = 0;
	if (shape.size() == 1) {
		second_dim = 1;
	}
	else if (shape.size() == 2) {
		second_dim = shape[1];
	}
	else {
		throw;
	}
	py::buffer_info buffer_data = py::buffer(object.attr("data")).request();

	return Matrix<T>(shape[0], second_dim,
	                 reinterpret_cast<T *>(buffer_data.ptr), false);
}

void PyNN_::fetch_data_nest(const std::vector<PopulationBase> &populations,
                            const std::vector<py::object> &pypopulations)
{
	for (size_t i = 0; i < populations.size(); i++) {
		std::vector<std::string> signals = populations[i].type().signal_names;
		for (size_t j = 0; j < signals.size(); j++) {
			if (populations[i].signals().is_recording(j)) {
				if (signals[j] == "spikes") {
					py::module nest = py::module::import("nest");
					py::dict nest_data =
					    py::tuple(nest.attr("GetStatus")(pypopulations[i]
					                                         .attr("recorder")
					                                         .attr("_spike_"
					                                               "detector")
					                                         .attr("device"),
					                                     "events"))[0];

					Matrix<int64_t> neuron_ids =
					    matrix_from_numpy<int64_t>(nest_data["senders"]);
					Matrix<double> spikes =
					    matrix_from_numpy<double>(nest_data["times"]);

					size_t offset = py::cast<size_t>(
					    py::list(pypopulations[i].attr("all_cells"))[0]);
					for (size_t k = 0; k < populations[i].size(); k++) {
						auto neuron = populations[i][k];
						auto idx = neuron.type().signal_index("spikes");
						size_t len = std::count(neuron_ids.begin(),
						                        neuron_ids.end(), k + offset);
						if (len == 0) {
							continue;
						}
						auto data = std::make_shared<Matrix<Real>>(len, 1);
						size_t counter = 0;
						for (size_t l = 0; l < neuron_ids.size(); l++) {
							if (size_t(neuron_ids[l]) == k + offset) {
								assert(counter < len);
								(*data)(counter, 0) = Real(spikes[l]);
								counter++;
							}
						}
						neuron.signals().data(idx.value(), std::move(data));
					}
				}
				else {
					auto record_it = NEST_RECORDING_VARIABLES.find(signals[j]);
					auto scale_it = NEST_RECORDING_SCALES.find(signals[j]);
					if (record_it == NEST_RECORDING_VARIABLES.end() ||
					    scale_it == NEST_RECORDING_SCALES.end()) {
						throw NotSupportedException(
						    "This is probably a "
						    "bug in the NEST "
						    "binding.");
					}
					Real scale = scale_it->second;

					py::module nest = py::module::import("nest");
					py::dict nest_data = py::tuple(
					    nest.attr("GetStatus")(pypopulations[i]
					                               .attr("recorder")
					                               .attr("_multimeter")
					                               .attr("device"),
					                           "events"))[0];

					Matrix<int64_t> neuron_ids =
					    matrix_from_numpy<int64_t>(nest_data["senders"]);
					Matrix<double> time =
					    matrix_from_numpy<double>(nest_data["times"]);
					Matrix<double> pydata = matrix_from_numpy<double>(
					    nest_data[record_it->second.c_str()]);

					size_t offset = py::cast<size_t>(
					    py::list(pypopulations[i].attr("all_cells"))[0]);

					for (size_t k = 0; k < populations[i].size(); k++) {
						auto neuron = populations[i][k];
						auto idx = neuron.type().signal_index(signals[j]);
						size_t len = std::count(neuron_ids.begin(),
						                        neuron_ids.end(), k + offset);
						if (len == 0) {
							continue;
						}
						auto data = std::make_shared<Matrix<Real>>(len, 2);
						size_t counter = 0;
						for (size_t l = 0; l < neuron_ids.size(); l++) {
							if (size_t(neuron_ids[l]) == k + offset) {
								assert(counter < len);
								(*data)(counter, 0) = Real(time[l]);
								(*data)(counter, 1) = Real(pydata[l] * scale);
								counter++;
							}
						}
						neuron.signals().data(idx.value(), std::move(data));
					}
				}
			}
		}
	}
}

void PyNN_::fetch_data_spinnaker(const std::vector<PopulationBase> &populations,
                                 const std::vector<py::object> &pypopulations)
{
	for (size_t i = 0; i < populations.size(); i++) {
		std::vector<std::string> signals = populations[i].type().signal_names;
		for (size_t j = 0; j < signals.size(); j++) {
			if (populations[i].signals().is_recording(j)) {
				py::object data =
				    pypopulations[i].attr("spinnaker_get_data")(signals[j]);
				Matrix<double> datac = matrix_from_numpy<double>(data);

				if (signals[j] == "spikes") {
					for (size_t k = 0; k < populations[i].size(); k++) {
						auto neuron = populations[i][k];
						auto idx = neuron.type().signal_index("spikes");
						size_t len = 0;
						for (size_t l = 0; l < datac.rows(); l++) {
							if (size_t(datac(l, 0)) == k) {
								len++;
							}
						}

						if (len == 0) {
							;
							continue;
						}
						auto res = std::make_shared<Matrix<Real>>(len, 1);
						size_t counter = 0;
						for (size_t l = 0; l < datac.rows(); l++) {
							if (size_t(datac(l, 0)) == k) {
								assert(counter < len);
								(*res)(counter, 0) = Real(datac(l, 1));
								counter++;
							}
						}
						neuron.signals().data(idx.value(), std::move(res));
					}
				}
				else {
					for (size_t k = 0; k < populations[i].size(); k++) {
						auto neuron = populations[i][k];
						auto idx = neuron.type().signal_index(signals[j]);

						size_t len = 0;
						for (size_t l = 0; l < datac.rows(); l++) {
							if (size_t(datac(l, 0)) == k) {
								len++;
							}
						}
						std::cout << k << ", " << len << std::endl;
						if (len == 0) {
							continue;
						}
						auto res = std::make_shared<Matrix<Real>>(len, 2);
						size_t counter = 0;
						for (size_t l = 0; l < datac.rows(); l++) {
							if (size_t(datac(l, 0)) == k) {
								assert(counter < len);
								(*res)(counter, 0) = Real(datac(l, 1));
								(*res)(counter, 1) = Real(datac(l, 2));
								counter++;
							}
						}
						neuron.signals().data(idx.value(), std::move(res));
					}
				}
			}
		}
	}
}

void PyNN_::fetch_data_neo5(const std::vector<PopulationBase> &populations,
                            const std::vector<py::object> &pypopulations)
{
	for (size_t i = 0; i < populations.size(); i++) {
		std::vector<std::string> signals = populations[i].type().signal_names;
		py::object neo_block = pypopulations[i].attr("get_data")();
		for (size_t j = 0; j < signals.size(); j++) {
			if (populations[i].signals().is_recording(j)) {
				if (signals[j] == "spikes") {
					py::list spiketrains =
					    (py::list(neo_block.attr("segments"))[0])
					        .attr("spiketrains");
					size_t len = py::len(spiketrains);
					for (size_t k = 0; k < len; k++) {
						size_t index = py::cast<size_t>(
						    spiketrains[k].attr("annotations")["source_"
						                                       "index"]);
						auto neuron = populations[i][index];
						auto idx = neuron.type().signal_index("spikes");
						// number of spikes
						std::vector<Real> spikes =
						    py::cast<std::vector<Real>>(spiketrains[k]);
						auto data =
						    std::make_shared<Matrix<Real>>(spikes.size(), 1);

						for (size_t i = 0; i < spikes.size(); i++) {
							(*data)(i, 0) = spikes[i];
						}
						neuron.signals().data(idx.value(), std::move(data));
					}
				}
				else {
					py::list analogsignals =
					    (py::list(neo_block.attr("segments"))[0])
					        .attr("analogsignals");
					size_t signal_index = 0;
					for (size_t k = 0; k < analogsignals.size(); k++) {
						if (py::cast<std::string>(
						        analogsignals[k].attr("name")) == signals[j]) {
							signal_index = k;
							break;
						}
					}
					Matrix<int64_t> neuron_ids =
					    matrix_from_numpy<int64_t>(analogsignals[signal_index]
					                                   .attr("channel_index")
					                                   .attr("channel_ids"));
					Matrix<double> time = matrix_from_numpy<double>(
					    analogsignals[signal_index].attr("times"));
					for (size_t k = 0; k < neuron_ids.size(); k++) {

						Matrix<double> pydata = matrix_from_numpy<double>(
						    py::list(analogsignals[signal_index].attr("T"))[k]);

						auto data =
						    std::make_shared<Matrix<Real>>(pydata.size(), 2);
						for (size_t l = 0; l < pydata.size(); l++) {
							(*data)(l, 0) = Real(time[l]);
							(*data)(l, 1) = Real(pydata[l]);
						}

						auto idx = populations[i][neuron_ids(k, 0)]
						               .type()
						               .signal_index(signals[j]);
						auto neuron = populations[i][neuron_ids(k, 0)];
						neuron.signals().data(idx.value(), std::move(data));
					}
				}
			}
		}
	}
}

void PyNN_::do_run(NetworkBase &source, Real duration) const
{
	// TODO : LOGGer
	py::scoped_interpreter guard;
	init_logger();

	py::module sys = py::module::import("sys");
	std::string import = get_import(m_imports, m_simulator);

	auto start = std::chrono::system_clock::now();
	// Bug when importing NEST
	if (import == "pyNN.nest") {
		auto a = py::list();
		a.append("pynest");
		a.append("--quiet");
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

	for (size_t i = 0; i < populations.size(); i++) {
		if (populations.size() == 0) {
			pypopulations.push_back(py::object());
			continue;
		}

		if (&populations[i].type() == &SpikeSourceArray::inst()) {
			pypopulations.push_back(
			    create_source_population(populations[i], pynn));
			const bool homogeneous_rec = populations[i].homogeneous_record();
			if (homogeneous_rec) {
				set_homogeneous_rec(populations[i], pypopulations.back());
			}
			else {
				set_inhomogenous_rec(populations[i], pypopulations.back(),
				                     pynn);
			}
		}
		else {
			bool homogeneous = populations[i].homogeneous_parameters();
			bool init_available = false;
			pypopulations.push_back(
			    create_homogeneous_pop(populations[i], pynn, init_available));

			if (!homogeneous) {
				set_inhomogeneous_parameters(
				    populations[i], pypopulations.back(), init_available);
			}
			const bool homogeneous_rec = populations[i].homogeneous_record();
			if (homogeneous_rec) {
				set_homogeneous_rec(populations[i], pypopulations.back());
			}
			else {
				set_inhomogenous_rec(populations[i], pypopulations.back(),
				                     pynn);
			}
		}
	}

	auto buildpop = std::chrono::system_clock::now();
	Real timestep = 0;
	if (m_simulator == "nest" || m_simulator == "spinnaker") {
		global_logger().info("cypress",
		                     "Delays are rounded to "
		                     "multiples of the "
		                     "timestep");
		timestep = py::cast<Real>(pynn.attr("get_time_step")());
		if (m_simulator == "spinnaker") {  // timestep to milliseconds
			timestep = timestep / 1000.0;
		}
	}
	for (auto conn : source.connections()) {
		auto it = SUPPORTED_CONNECTIONS.find(conn.connector().name());
		GroupConnction group_conn;
		if (it != SUPPORTED_CONNECTIONS.end() &&
		    conn.connector().group_connect(conn, group_conn)) {
			// Group connections
			group_connect(populations, pypopulations, conn, group_conn, pynn,
			              it->second, timestep);
		}
		else {
			list_connect(pypopulations, conn, pynn, timestep);
		}
	}

	auto buildconn = std::chrono::system_clock::now();
	int duration_rounded = int((duration + timestep) / timestep) * timestep;
	pynn.attr("run")(duration_rounded);
	auto execrun = std::chrono::system_clock::now();

	// fetch data
	if (m_simulator == "nest") {
		fetch_data_nest(populations, pypopulations);
	}
	if (m_simulator == "spinnaker") {
		fetch_data_spinnaker(populations, pypopulations);
	}
	else {
		fetch_data_neo5(populations, pypopulations);
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
	std::cout << "finished "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 finished - execrun)
	                 .count()
	          << std::endl;
	pynn.attr("end")();

	/*
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
	// Split the simulator import map into the
	// simulators and the corresponding imports
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
