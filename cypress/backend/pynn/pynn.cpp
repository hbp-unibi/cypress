/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2018 Christoph Jenzen
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

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>  //Type conversions
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cypress/backend/pynn/pynn.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>
#include <fstream>
#include <future>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cypress {

namespace py = pybind11;
using namespace py::literals;

PythonInstance::PythonInstance()
{
	py::initialize_interpreter();
	py::module logging = py::module::import("logging");
	int32_t loglevel = global_logger().min_level();
	logging.attr("basicConfig")("level"_a = loglevel);
}
PythonInstance::~PythonInstance()
{
	// Here we should call
	// py::finalize_interpreter();
	/*
	 * Py_Finalize() will lead to a SegFault here
	 */
}

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
    NORMALISED_SIMULATOR_NAMES = {{"spinnaker", "nmmc1"},
                                  {"hardware.spikey", "spikey"},
                                  {"spikey", "spikey"},
                                  {"nest", "nest"},
                                  {"nm-mc1", "nmmc1"}};

/**
 * Maps certain simulator names to the correct PyNN module names. If multiple
 * module names are given, the first found module is used.
 */
static const std::unordered_map<std::string, std::string> SIMULATOR_IMPORT_MAP =
    {{"nest", "pyNN.nest"},
     {"nmmc1", "pyNN.spiNNaker"},
     {"spikey", "pyNN.hardware.spikey"}};

/**
 * Map between the canonical system names and the
 */
static const std::unordered_map<std::string, SystemProperties>
    SIMULATOR_PROPERTIES = {{"nest", {false, false, false}},
                            {"nmmc1", {false, true, false}},
                            {"spikey", {true, true, false}}};  // TODO WHAT FOR?

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
        {"nmmc1",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(), &IfCurrExp::inst()}},
        {"nmpm1",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(),
          &EifCondExpIsfaIsta::inst()}},
        {"ess",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(),
          &EifCondExpIsfaIsta::inst()}},
        {"spikey", {&SpikeSourceArray::inst(), &IfFacetsHardware1::inst()}},
        {"__default__",
         {&SpikeSourceArray::inst(), &IfCondExp::inst(),
          &EifCondExpIsfaIsta::inst(), &IfCurrExp::inst()}}};

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
		PythonInstance::instance();
		std::vector<bool> res;
		for (auto &i : imports) {
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

PyNN::PyNN(const std::string &simulator, const Json &setup)
    : m_simulator(simulator), m_setup(setup)
{
	PythonInstance::instance();
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

PyNN::~PyNN() = default;

int PyNN::get_pynn_version()
{
	py::module pynn = py::module::import("pyNN");
	std::string version = py::cast<std::string>(pynn.attr("__version__"));
	std::stringstream ss(version);
	std::string main, submain;
	std::getline(ss, main, '.');
	std::getline(ss, submain, '.');
	int main_n = std::stoi(main);
	int submain_n = std::stoi(submain);

	if (main_n != 0 ||
	    !((submain_n == 6) || (submain_n == 8) || (submain_n == 9))) {
		throw NotSupportedException("PyNN version " + main + "." + submain +
		                            "is not supported");
	}
	return submain_n;
}
std::string PyNN::get_import(const std::vector<std::string> &imports,
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
	if (normalised_simulator == "nmpm1") {
		return "pyhmf";
	}
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

Real PyNN::timestep()
{
	SystemProperties props = PyNNUtil::properties(m_normalised_simulator);
	if (props.analogue()) {
		return 0.0;  // No minimum timestep on analogue neuromorphic hardware
	}
	return m_setup.value("timestep", 0.1);  // Default is 0.1ms
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

int PyNN::get_neo_version()
{
	int pynn_v = get_pynn_version();
	if (pynn_v < 8) {
		// No neo necessary or expected
		return 0;
	}
	try {
		py::module neo = py::module::import("neo");
	}
	catch (...) {
		throw NotSupportedException("PyNN version " + std::to_string(pynn_v) +
		                            "." + " requires neo");
	}
	py::module neo = py::module::import("neo");
	std::string version = py::cast<std::string>(neo.attr("__version__"));
	std::stringstream ss(version);
	std::string main, submain;
	std::getline(ss, main, '.');
	std::getline(ss, submain, '.');
	int main_n = std::stoi(main);
	int submain_n = std::stoi(submain);

	if (main_n != 0 || !((submain_n > 3) && (submain_n < 9))) {
		throw NotSupportedException("Neo version " + main + "." + submain +
		                            " is not supported");
	}
	return submain_n;
}

std::unordered_set<const NeuronType *> PyNN::supported_neuron_types() const
{
	auto it = SUPPORTED_NEURON_TYPE_MAP.find(m_normalised_simulator);
	if (it != SUPPORTED_NEURON_TYPE_MAP.end()) {
		return it->second;
	}
	return SUPPORTED_NEURON_TYPE_MAP.find("__default__")->second;
}

py::dict PyNN::json_to_dict(Json json)
{
	py::dict dict;
	for (Json::iterator i = json.begin(); i != json.end(); ++i) {
		Json value = i.value();
		if (value.is_object()) {
			dict = py::dict(py::arg(i.key().c_str()) = **json_to_dict(value),
			                **dict);
		}
		else if (value.is_string()) {
			std::string temp = value;
			dict = py::dict(py::arg(i.key().c_str()) = temp, **dict);
		}
		else if (value.is_array()) {  // structured
			py::list list;
			for (auto &entry : value) {
				if (entry.is_number_float()) {
					list.append(double(entry));
				}
				else if (entry.is_boolean()) {
					list.append(bool(entry));
				}
				else if (entry.is_number_integer() ||
				         entry.is_number_unsigned()) {
					list.append(int(entry));
				}
				else if (entry.is_string()) {
					std::string temp = entry;
					list.append(temp);
				}
				else {
					throw NotSupportedException(
					    "Datatype is not supported for conversion to python "
					    "dict!");
				}
			}
			dict = py::dict(py::arg(i.key().c_str()) = list, **dict);
		}
		else if (value.is_boolean()) {
			bool temp = value;
			dict = py::dict(py::arg(i.key().c_str()) = temp, **dict);
		}
		else if (value.is_number_float()) {
			double temp = value;
			dict = py::dict(py::arg(i.key().c_str()) = temp, **dict);
		}
		else if (value.is_number_integer() or value.is_number_unsigned()) {
			int temp = value;
			dict = py::dict(py::arg(i.key().c_str()) = temp, **dict);
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
std::string PyNN::get_neuron_class(const NeuronType &neuron_type)
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
	else if (&neuron_type == &IfCurrExp::inst()) {
		return "IF_curr_exp";
	}
	else {
		throw NotSupportedException("This neuron type is not supported yet!");
	}
}

void PyNN::init_logger()
{
	py::module logger = py::module::import("logging");
	py::module util = py::module::import("pyNN.utility");
	int32_t loglevel = global_logger().min_level();
	try {
		util.attr("init_logging")(py::none(), "level"_a = loglevel);
	}
	catch (...) {
	}
}

py::object PyNN::create_source_population(const PopulationBase &pop,
                                          py::module &pynn)
{
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

py::object PyNN::create_homogeneous_pop(const PopulationBase &pop,
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
	try {
		auto idx = pop[0].type().parameter_index("v_rest");
		if (idx.valid()) {
			pypop.attr("initialize")("v"_a = params[idx.value()]);
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

void PyNN::set_inhomogeneous_parameters(const PopulationBase &pop,
                                        py::object &pypop, bool init_available)
{
	auto idx = pop[0].type().parameter_index("v_rest");
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

void PyNN::set_homogeneous_rec(const PopulationBase &pop, py::object &pypop)
{
	const std::vector<std::string> &signals = pop.type().signal_names;
	for (size_t j = 0; j < signals.size(); j++) {
		if (pop.signals().is_recording(j)) {
			pypop.attr("record")(signals[j].c_str());
		}
	}
}

void PyNN::set_inhomogenous_rec(const PopulationBase &pop, py::object &pypop,
                                py::module &pynn)
{
	const std::vector<std::string> &signals = pop.type().signal_names;
	for (size_t j = 0; j < signals.size(); j++) {
		std::vector<uint64_t> neuron_ids;
		for (uint64_t k = 0; k < pop.size(); k++) {
			if (pop[k].signals().is_recording(j)) {
				neuron_ids.push_back(k);
			}
		}
		if (neuron_ids.size() == 0) {
			continue;
		}
		py::object popview = pynn.attr("PopulationView")(
		    pypop,
		    py::array_t<uint64_t>({neuron_ids.size()}, {sizeof(uint64_t)},
		                          neuron_ids.data()));
		popview.attr("record")(signals[j].c_str());
	}
}

py::object PyNN::get_pop_view(const py::module &pynn, const py::object &py_pop,
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
		catch (py::error_already_set &) {
			throw NotSupportedException(
			    "Popviews are not supported by SpiNNaker");
		}
	}
}

py::object PyNN::get_connector7(const ConnectionDescriptor &conn,
                                const py::module &pynn)
{
	auto params = conn.connector().synapse()->parameters();
	std::string name = conn.connector().synapse()->name();
	std::string connector_name =
	    SUPPORTED_CONNECTIONS.find(conn.connector().name())->second;
	Real weight = fabs(params[0]);
	Real delay = params[1];
	if (connector_name == "AllToAllConnector") {
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = weight, "delays"_a = delay,
		    "allow_self_connections"_a =
		        py::cast(conn.connector().allow_self_connections()));
	}
	else if (connector_name == "OneToOneConnector") {
		return pynn.attr(connector_name.c_str())("weights"_a = weight,
		                                         "delays"_a = delay);
	}
	else if (connector_name == "FixedProbabilityConnector") {
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = weight, "delays"_a = delay,
		    "p_connect"_a = conn.connector().additional_parameter(),
		    "allow_self_connections"_a =
		        py::cast(conn.connector().allow_self_connections()));
	}
	else if (connector_name == "FixedNumberPreConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = weight, "delays"_a = delay,
		    "n"_a = np.attr("int")(conn.connector().additional_parameter()),
		    "allow_self_connections"_a =
		        py::cast(conn.connector().allow_self_connections()));
	}
	else if (connector_name == "FixedNumberPostConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "weights"_a = weight, "delays"_a = delay,
		    "n"_a = np.attr("int")(conn.connector().additional_parameter()),
		    "allow_self_connections"_a =
		        py::cast(conn.connector().allow_self_connections()));
	}
	throw NotSupportedException("Requested group connection " + connector_name +
	                            " is not supported");
}

py::object PyNN::get_connector(const std::string &connector_name,
                               const py::module &pynn,
                               const Real &additional_parameter,
                               const bool allow_self_connections)
{
	if (connector_name == "AllToAllConnector") {
		return pynn.attr(connector_name.c_str())(
		    "allow_self_connections"_a = py::cast(allow_self_connections));
	}
	else if (connector_name == "OneToOneConnector") {
		return pynn.attr(connector_name.c_str())();
	}
	else if (connector_name == "FixedProbabilityConnector") {
		return pynn.attr(connector_name.c_str())(
		    "p_connect"_a = additional_parameter,
		    "allow_self_connections"_a = py::cast(allow_self_connections));
	}
	else if (connector_name == "FixedNumberPreConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "n"_a = np.attr("int")(additional_parameter),
		    "allow_self_connections"_a = py::cast(allow_self_connections));
	}
	else if (connector_name == "FixedNumberPostConnector") {
		py::module np = py::module::import("numpy");
		return pynn.attr(connector_name.c_str())(
		    "n"_a = np.attr("int")(additional_parameter),
		    "allow_self_connections"_a = py::cast(allow_self_connections));
	}
	throw NotSupportedException("Requested group connection " + connector_name +
	                            " is not supported");
}

namespace {
bool popview_warnign_emitted = false;

inline py::object fallback_connector(
    const std::vector<PopulationBase> &populations,
    const std::vector<py::object> &pypopulations,
    const ConnectionDescriptor &conn, const py::module &pynn, bool &nest_flag,
    const Real &timestep)
{
	// Issue related to PyNN #625
	bool current_based = false;
	if (!populations[conn.pid_tar()].type().conductance_based && nest_flag) {
		current_based = true;
	}
	std::tuple<py::object, py::object> ret =
	    PyNN::list_connect(pypopulations, conn, pynn, current_based, timestep);
	if (conn.connector().synapse()->parameters()[0] >= 0) {
		return std::get<0>(ret);
	}
	return std::get<1>(ret);
}

inline py::object get_synapse(const std::string &name,
                              const std::vector<Real> &params,
                              const py::module &pynn, const Real &weight,
                              const Real &delay)
{
	py::object synapse;
	if (name == "StaticSynapse") {
		synapse =
		    pynn.attr("StaticSynapse")("weight"_a = weight, "delay"_a = delay);
	}
	else if (name == "SpikePairRuleAdditive") {
		py::object timing_dependence = pynn.attr("SpikePairRule")(
		    "tau_plus"_a = params[2], "tau_minus"_a = params[3],
		    "A_plus"_a = params[4], "A_minus"_a = params[5]);
		py::object weight_dependence = pynn.attr("AdditiveWeightDependence")(
		    "w_min"_a = params[6], "w_max"_a = params[7]);
		synapse = pynn.attr("STDPMechanism")(
		    "weight"_a = weight, "delay"_a = delay,
		    "timing_dependence"_a = timing_dependence,
		    "weight_dependence"_a = weight_dependence);
	}
	else if (name == "SpikePairRuleMultiplicative") {
		py::object timing_dependence = pynn.attr("SpikePairRule")(
		    "tau_plus"_a = params[2], "tau_minus"_a = params[3],
		    "A_plus"_a = params[4], "A_minus"_a = params[5]);
		py::object weight_dependence =
		    pynn.attr("MultiplicativeWeightDependence")("w_min"_a = params[6],
		                                                "w_max"_a = params[7]);
		synapse = pynn.attr("STDPMechanism")(
		    "weight"_a = weight, "delay"_a = delay,
		    "timing_dependence"_a = timing_dependence,
		    "weight_dependence"_a = weight_dependence);
	}
	else if (name == "TsodyksMarkramMechanism") {
		synapse = pynn.attr("TsodyksMarkramSynapse")(
		    "weight"_a = weight, "delay"_a = delay, "U"_a = params[2],
		    "tau_rec"_a = params[3], "tau_facil"_a = params[4]);
	}
	else {
		throw ExecutionError(name + " is not supported for this backend!");
	}
	return synapse;
}

}  // namespace
py::object PyNN::group_connect(const std::vector<PopulationBase> &populations,
                               const std::vector<py::object> &pypopulations,
                               const ConnectionDescriptor &conn,
                               const py::module &pynn, bool nest_flag,
                               const Real timestep)
{
	if (popview_warnign_emitted) {
		return fallback_connector(populations, pypopulations, conn, pynn,
		                          nest_flag, timestep);
	}
	py::object source, target;
	std::string conn_name =
	    SUPPORTED_CONNECTIONS.find(conn.connector().name())->second;
	const auto &params = conn.connector().synapse()->parameters();
	std::string name = conn.connector().synapse()->name();
	try {
		source = get_pop_view(pynn, pypopulations[conn.pid_src()],
		                      populations[conn.pid_src()], conn.nid_src0(),
		                      conn.nid_src1());
		if ((conn.pid_src() == conn.pid_tar()) &&
		    (conn.nid_src0() == conn.nid_tar0()) &&
		    (conn.nid_src1() == conn.nid_tar1())) {
			// Workaround for pyNN to make the allow_self_connections flag
			// Issue #622
			target = source;
		}
		else {
			target = get_pop_view(pynn, pypopulations[conn.pid_tar()],
			                      populations[conn.pid_tar()], conn.nid_tar0(),
			                      conn.nid_tar1());
		}
	}
	catch (...) {
		if (!popview_warnign_emitted) {
			global_logger().info(
			    "cypress", "PopViews not supported. Using list connector");
			popview_warnign_emitted = true;
		}
		return fallback_connector(populations, pypopulations, conn, pynn,
		                          nest_flag, timestep);
	}

	// Issue related to PyNN #625
	bool current_based = false;
	if (!populations[conn.pid_tar()].type().conductance_based && nest_flag) {
		current_based = true;
	}

	std::string receptor = "excitatory";
	if (params[0] < 0 && !current_based) {
		receptor = "inhibitory";
	}
	Real delay = params[1];
	if (timestep != 0) {
		delay = std::max(round(delay / timestep), Real(1.0)) * timestep;
	}

	double weight = current_based ? params[0] : fabs(params[0]);

	py::object synapse = get_synapse(name, params, pynn, weight, delay);
	try {
		return pynn.attr("Projection")(
		    source, target,
		    get_connector(conn_name, pynn,
		                  conn.connector().additional_parameter(),
		                  conn.connector().allow_self_connections()),
		    "synapse_type"_a = synapse, "receptor_type"_a = receptor);
	}
	catch (...) {
		if (!popview_warnign_emitted) {
			global_logger().info(
			    "cypress", "PopViews not supported. Using list connector");
			popview_warnign_emitted = true;
		}
		return fallback_connector(populations, pypopulations, conn, pynn,
		                          nest_flag, timestep);
	}
}

py::object PyNN::group_connect7(const std::vector<PopulationBase> &,
                                const std::vector<py::object> &pypopulations,
                                const ConnectionDescriptor &conn,
                                const py::module &pynn)
{
	std::string conn_name =
	    SUPPORTED_CONNECTIONS.find(conn.connector().name())->second;
	const auto &params = conn.connector().synapse()->parameters();
	std::string name = conn.connector().synapse()->name();

	py::object source = pypopulations[conn.pid_src()];
	py::object target = pypopulations[conn.pid_tar()];
	std::string receptor = "excitatory";
	if (params[0] < 0) {
		receptor = "inhibitory";
	}
	if (name == "StaticSynapse") {
		return pynn.attr("Projection")(
		    source, target, get_connector7(conn, pynn), "target"_a = receptor);
	}
	else {
		py::object synapse;
		if (name == "SpikePairRuleAdditive") {
			synapse = pynn.attr("STDPMechanism")(
			    "timing_dependence"_a = pynn.attr("SpikePairRule")(
			        "tau_plus"_a = params[2], "tau_minus"_a = params[3]),
			    "weight_dependence"_a = pynn.attr("AdditiveWeightDependence")(
			        "w_min"_a = params[6], "w_max"_a = params[7],
			        "A_plus"_a = params[4], "A_minus"_a = params[5]));
			return pynn.attr("Projection")(
			    source, target, get_connector7(conn, pynn),
			    "target"_a = receptor,
			    "synapse_dynamics"_a =
			        pynn.attr("SynapseDynamics")("slow"_a = synapse));
		}
		else if (name == "TsodyksMarkramMechanism") {
			synapse = pynn.attr("TsodyksMarkramMechanism")(
			    "U"_a = params[2], "tau_rec"_a = params[3],
			    "tau_facil"_a = params[4]);
			return pynn.attr("Projection")(
			    source, target, get_connector7(conn, pynn),
			    "target"_a = receptor,
			    "synapse_dynamics"_a =
			        pynn.attr("SynapseDynamics")("fast"_a = synapse));
		}

		else {
			throw ExecutionError(
			    "Only static synapses are supported for this backend!");
		}
	}
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
std::tuple<py::object, py::object> PyNN::list_connect(
    const std::vector<py::object> &pypopulations,
    const ConnectionDescriptor conn, const py::module &pynn,
    const bool current_based, const Real timestep)
{
	std::tuple<py::object, py::object> ret =
	    std::make_tuple(py::object(), py::object());
	std::vector<LocalConnection> conns_full;
	size_t num_inh = 0;
	conn.connect(conns_full);
	for (auto &i : conns_full) {
		if (i.inhibitory()) {
			num_inh++;
		}
	}
	size_t num_syn_params = conns_full[0].SynapseParameters.size();
	Matrix<Real> *conns_exc = nullptr, *conns_inh = nullptr;
	if (conns_full.size() - num_inh > 0) {
		conns_exc =
		    new Matrix<Real>(conns_full.size() - num_inh, 2 + num_syn_params);
	}
	if (num_inh > 0) {
		conns_inh = new Matrix<Real>(num_inh, 2 + num_syn_params);
	}

	size_t counter_ex = 0, counter_in = 0;
	for (auto &i : conns_full) {
		if (i.SynapseParameters[0] >= 0) {
			(*conns_exc)(counter_ex, 0) = i.src;
			(*conns_exc)(counter_ex, 1) = i.tar;
			(*conns_exc)(counter_ex, 2) = i.SynapseParameters[0];
			if (timestep != 0) {
				Real delay = std::max(round(i.SynapseParameters[1] / timestep),
				                      Real(1.0)) *
				             timestep;
				(*conns_exc)(counter_ex, 3) = delay;
			}
			else {
				(*conns_exc)(counter_ex, 3) = i.SynapseParameters[1];
			}
			for (size_t j = 2; j < i.SynapseParameters.size(); j++) {
				(*conns_exc)(counter_ex, 2 + j) = i.SynapseParameters[j];
			}
			counter_ex++;
		}
		else {
			(*conns_inh)(counter_in, 0) = i.src;
			(*conns_inh)(counter_in, 1) = i.tar;
			if (!current_based) {
				(*conns_inh)(counter_in, 2) = -i.SynapseParameters[0];
			}
			else {
				(*conns_inh)(counter_in, 2) = i.SynapseParameters[0];
			}
			if (timestep != 0) {
				Real delay = std::max(round(i.SynapseParameters[1] / timestep),
				                      Real(1.0)) *
				             timestep;
				(*conns_inh)(counter_in, 3) = delay;
			}
			else {
				(*conns_inh)(counter_in, 3) = i.SynapseParameters[1];
			}
			for (size_t j = 2; j < i.SynapseParameters.size(); j++) {
				(*conns_exc)(counter_in, 2 + j) = i.SynapseParameters[j];
			}
			counter_in++;
		}
	}

	auto temp_params = conn.connector().synapse()->parameters();
	if (temp_params[0] == 0) {
		temp_params[0] = 0.1;
		temp_params[1] = 1.0;  // Bug on SpiNNaker
	}
	bool static_synapse = conn.connector().synapse_name() == "StaticSynapse"
	                          ? true
	                          : false;  // SpiNNaker 4 bug
	auto synapse_type = get_synapse(conn.connector().synapse_name(),
	                                temp_params, pynn, 0.0, 1.0);
	std::vector<std::string> syn_param_names =
	    conn.connector().synapse()->parameter_names();
	py::list py_names = py::cast(syn_param_names);

	if (conns_full.size() - num_inh > 0) {
		auto capsule = py::capsule(conns_exc, [](void *v) {
			delete reinterpret_cast<Matrix<Real> *>(v);
		});
		py::array temp_array({(*conns_exc).rows(), (*conns_exc).cols()},
		                     (*conns_exc).data(), capsule);
		py::object connector;
		if (static_synapse) {
			connector = pynn.attr("FromListConnector")(temp_array);
		}
		else {
			connector = pynn.attr("FromListConnector")(
			    temp_array, "column_names"_a = py_names);
		}
		std::get<0>(ret) = pynn.attr("Projection")(
		    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
		    connector, "synapse_type"_a = synapse_type,
		    "receptor_type"_a = "excitatory");
	}
	if (num_inh > 0) {
		auto capsule = py::capsule(conns_inh, [](void *v) {
			delete reinterpret_cast<Matrix<Real> *>(v);
		});
		py::array temp_array({(*conns_inh).rows(), (*conns_inh).cols()},
		                     (*conns_inh).data(), capsule);
		py::object connector;
		if (static_synapse) {
			connector = pynn.attr("FromListConnector")(temp_array);
		}
		else {
			connector = pynn.attr("FromListConnector")(
			    temp_array, "column_names"_a = py_names);
		}
		if (!current_based) {
			std::get<1>(ret) = pynn.attr("Projection")(
			    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
			    connector, "synapse_type"_a = synapse_type,
			    "receptor_type"_a = "inhibitory");
		}
		else {
			std::get<1>(ret) = pynn.attr("Projection")(
			    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
			    connector, "synapse_type"_a = synapse_type);
		}
	}
	return ret;
}

std::tuple<py::object, py::object> PyNN::list_connect7(
    const std::vector<py::object> &pypopulations,
    const ConnectionDescriptor conn, const py::module &pynn)
{
	if (conn.connector().synapse_name() != "StaticSynapse") {
		// TODO
		throw ExecutionError(
		    "Only static synapses are supported for this backend!");
	}
	std::vector<LocalConnection> conns_full;
	conn.connect(conns_full);
	py::list conn_exc, conn_inh;
	std::tuple<py::object, py::object> ret =
	    std::make_tuple(py::object(), py::object());

	for (const auto &conn : conns_full) {
		py::list local_conn;
		local_conn.append(conn.src);
		local_conn.append(conn.tar);
		if (conn.excitatory()) {
			local_conn.append(conn.SynapseParameters[0]);
			local_conn.append(conn.SynapseParameters[1]);
			conn_exc.append(local_conn);
		}
		else {
			local_conn.append(-conn.SynapseParameters[0]);
			local_conn.append(conn.SynapseParameters[1]);
			conn_inh.append(local_conn);
		}
	}

	if (conn_exc.size() > 0) {
		py::object connector = pynn.attr("FromListConnector")(conn_exc);

		std::get<0>(ret) = pynn.attr("Projection")(
		    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
		    connector, "target"_a = "excitatory");
	}
	if (conn_inh.size() > 0) {
		py::object connector = pynn.attr("FromListConnector")(conn_inh);
		std::get<1>(ret) = pynn.attr("Projection")(
		    pypopulations[conn.pid_src()], pypopulations[conn.pid_tar()],
		    connector, "target"_a = "inhibitory");
	}
	return ret;
}

namespace {
template <typename T, typename T2>
void inline assert_types()
{
	if (typeid(T) != typeid(T2)) {
		throw ExecutionError("C type " + std::string(typeid(T).name()) +
		                     " does not match python type " +
		                     std::string(typeid(T2).name()) + "! ");
	}
}
}  // namespace

template <typename T>
Matrix<T> PyNN::matrix_from_numpy(const py::object &object, bool transposed)
{
	// Check the data type
	std::string type;
	type = py::cast<std::string>(object.attr("dtype").attr("name"));
	if (type == "int8") {
		assert_types<T, int8_t>();
	}
	else if (type == "int16") {
		assert_types<T, int16_t>();
	}
	else if (type == "int32") {
		assert_types<T, int32_t>();
	}
	else if (type == "int64") {
		assert_types<T, int64_t>();
	}
	else if (type == "uint8") {
		assert_types<T, uint8_t>();
	}
	else if (type == "uint16") {
		assert_types<T, uint16_t>();
	}
	else if (type == "uint32") {
		assert_types<T, uint32_t>();
	}
	else if (type == "uint64") {
		assert_types<T, uint64_t>();
	}
	else if (type == "float64") {
		assert_types<T, double>();
	}
	else if (type == "float32") {
		assert_types<T, float>();
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
		throw ExecutionError(
		    "Python arrays with dimension >2 are not supported!");
	}

	py::buffer_info buffer_data = py::buffer(object.attr("data")).request();
	if (transposed) {
		return Matrix<T>(second_dim, shape[0],
		                 reinterpret_cast<T *>(buffer_data.ptr), false);
	}
	else {
		return Matrix<T>(shape[0], second_dim,
		                 reinterpret_cast<T *>(buffer_data.ptr), false);
	}
}

void PyNN::fetch_data_nest(const std::vector<PopulationBase> &populations,
                           const std::vector<py::object> &pypopulations)
{
	for (size_t i = 0; i < populations.size(); i++) {
		if (populations[i].size() == 0) {
			continue;
		}
		std::vector<std::string> signals = populations[i].type().signal_names;
		for (size_t j = 0; j < signals.size(); j++) {
			bool is_recording = false;
			for (auto neuron : populations[i]) {
				if (neuron.signals().is_recording(j)) {
					is_recording = true;
					break;
				}
			}

			if (is_recording) {
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

void PyNN::fetch_data_spinnaker(const std::vector<PopulationBase> &populations,
                                const std::vector<py::object> &pypopulations)
{
	for (size_t i = 0; i < populations.size(); i++) {
		if (populations[i].size() == 0) {
			continue;
		}
		std::vector<std::string> signals = populations[i].type().signal_names;
		for (size_t j = 0; j < signals.size(); j++) {
			bool is_recording = false;
			for (auto neuron : populations[i]) {
				if (neuron.signals().is_recording(j)) {
					is_recording = true;
					break;
				}
			}

			if (is_recording) {
				py::object data =
				    pypopulations[i].attr("spinnaker_get_data")(signals[j]);
				Matrix<double> datac = matrix_from_numpy<double>(data);

				if (signals[j] == "spikes") {
					size_t counter = 0;
					for (size_t k = 0; k < populations[i].size(); k++) {
						if (size_t(datac(counter, 0)) != k) {
							continue;
						}
						size_t len = 0;
						while (counter + len < datac.rows()) {
							if (size_t(datac(counter + len, 0)) == k) {
								len++;
							}
							else {
								break;
							}
						}
						auto res = std::make_shared<Matrix<Real>>(len, 1);
						for (size_t l = 0; l < len; l++) {
							(*res)(l, 0) = Real(datac(counter, 1));
							counter++;
						}
						auto neuron = populations[i][k];
						auto idx = neuron.type().signal_index("spikes");
						neuron.signals().data(idx.value(), std::move(res));
					}
					assert(datac.rows() == counter);
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

void PyNN::fetch_data_neo(const std::vector<PopulationBase> &populations,
                          const std::vector<py::object> &pypopulations)
{
	int neo_v = get_neo_version();
	for (size_t i = 0; i < populations.size(); i++) {
		if (populations[i].size() == 0) {
			continue;
		}
		std::vector<std::string> signals = populations[i].type().signal_names;
		py::object neo_block = pypopulations[i].attr("get_data")();
		for (size_t j = 0; j < signals.size(); j++) {
			bool is_recording = false;
			for (auto neuron : populations[i]) {
				if (neuron.signals().is_recording(j)) {
					is_recording = true;
					break;
				}
			}

			if (is_recording) {
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
					py::list analogsignals;
					if (neo_v > 4) {
						analogsignals =
						    (py::list(neo_block.attr("segments"))[0])
						        .attr("analogsignals");
					}
					else {
						analogsignals =
						    (py::list(neo_block.attr("segments"))[0])
						        .attr("analogsignalarrays");
					}
					size_t signal_index = 0;
					for (size_t k = 0; k < analogsignals.size(); k++) {
						if (py::cast<std::string>(
						        analogsignals[k].attr("name")) == signals[j]) {
							signal_index = k;
							break;
						}
					}
					py::object py_neuron_ids;
					bool new_spinnaker = false;

					if (neo_v > 4) {
						py_neuron_ids = analogsignals[signal_index]
						                    .attr("channel_index")
						                    .attr("channel_ids");
						// SpiNNaker compatibility fix
						if (py::len(py_neuron_ids) == 0) {
							auto py_neuron_ids2 =
							    analogsignals[signal_index].attr(
							        "annotations")["source_ids"];
							py::module np = py::module::import("numpy");
							py_neuron_ids = np.attr("array")(
							    py_neuron_ids2, "dtype"_a = np.attr("int64"));
							new_spinnaker = true;
						}
					}
					else {
						py_neuron_ids =
						    analogsignals[signal_index].attr("channel_indexes");
					}
					Matrix<int64_t> neuron_ids =
					    matrix_from_numpy<int64_t>(py_neuron_ids);
					if (new_spinnaker) {
						for (size_t i = 0; i < neuron_ids.size(); i++) {
							neuron_ids[i] = neuron_ids[i] - 1;
						}
					}
					py::object py_times =
					    analogsignals[signal_index].attr("times");
					Matrix<double> time = matrix_from_numpy<double>(py_times);
					if (neo_v > 4 && !new_spinnaker) {
						py::object py_pydata =
						    analogsignals[signal_index].attr("as_array")();
						Matrix<double> pydata =
						    matrix_from_numpy<double>(py_pydata, true);

						for (size_t k = 0; k < neuron_ids.size(); k++) {
							assert(time.size() == pydata.cols());
							auto data =
							    std::make_shared<Matrix<Real>>(time.size(), 2);
							for (size_t l = 0; l < time.size(); l++) {
								(*data)(l, 0) = time[l];
								(*data)(l, 1) = pydata(k, l);
							}

							auto neuron = populations[i][neuron_ids[k]];
							auto idx = neuron.type().signal_index(signals[j]);
							neuron.signals().data(idx.value(), std::move(data));
						}
					}
					else {
						for (size_t k = 0; k < neuron_ids.size(); k++) {
							py::list transposed =
							    py::list(analogsignals[signal_index].attr("T"));
							Matrix<double> pydata =
							    matrix_from_numpy<double>(transposed[k]);
							auto data = std::make_shared<Matrix<Real>>(
							    pydata.size(), 2);
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
}

namespace {
/**
 * Used internally to get weights and delays from plastic synapses.
 *
 * @param proj Pynn Projection object
 * @return A list of local connections
 */
std::vector<LocalConnection> get_weights(py::object &proj)
{
	if (!proj) {
		return {};
	}
	py::list arguments;
	arguments.append("weight");
	arguments.append("delay");
	py::list list = proj.attr("get")(arguments, "format"_a = "list");
	std::vector<LocalConnection> weights;
	for (size_t j = 0; j < py::len(list); j++) {
		py::list lc = list[j];
		weights.push_back(
		    LocalConnection(py::cast<Real>(lc[0]), py::cast<Real>(lc[1]),
		                    py::cast<Real>(lc[2]), py::cast<Real>(lc[3])));
	}
	return weights;
}

}  // namespace

void PyNN::do_run(NetworkBase &source, Real duration) const
{
	auto gc = py::module::import("gc");
	gc.attr("collect")();

	py::module sys = py::module::import("sys");
	std::string import = get_import(m_imports, m_simulator);
	init_logger();

	auto start = std::chrono::steady_clock::now();
	// Bug when importing NEST
	if (import == "pyNN.nest") {
		auto a = py::list();
		a.append("pynest");
		a.append("--quiet");
		sys.attr("argv") = a;
	}
	int neurons_per_core = 0, sneurons_per_core = 0;
	if (m_setup.find("neurons_per_core") != m_setup.end()) {
		neurons_per_core = m_setup["neurons_per_core"];
	}
	if (m_setup.find("source_neurons_per_core") != m_setup.end()) {
		sneurons_per_core = m_setup["source_neurons_per_core"];
	}
	// Setup simulator
	py::module pynn = py::module::import(import.c_str());
	auto dict = json_to_dict(m_setup);
	if (import == "pyNN.hardware.spikey") {
		spikey_run(source, duration, pynn, dict);
		return;
	}
	try {
		pynn.attr("setup")(**dict);
	}
	catch (const pybind11::error_already_set &e) {
		throw ExecutionError(e.what());
	}

	if (import == "pyNN.spiNNaker") {
		if (neurons_per_core > 0) {
			global_logger().info("cypress",
			                     "Setting Number of Neurons per core for the "
			                     "IF_cond_exp model to " +
			                         std::to_string(neurons_per_core));
			pynn.attr("set_number_of_neurons_per_core")(
			    pynn.attr("IF_cond_exp"), neurons_per_core);
		}
		if (sneurons_per_core > 0) {
			global_logger().info(
			    "cypress",
			    "Setting Number of Neurons per core for Source Neurons to " +
			        std::to_string(sneurons_per_core));
			pynn.attr("set_number_of_neurons_per_core")(
			    pynn.attr("SpikeSourceArray"), sneurons_per_core);
		}
	}

	// Create populations
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

	Real timestep = 0;
	if (m_normalised_simulator == "nest" || m_normalised_simulator == "nmmc1") {
		global_logger().info("cypress",
		                     "Delays are rounded to "
		                     "multiples of the "
		                     "timestep");
		timestep = py::cast<Real>(pynn.attr("get_time_step")());
		if (m_normalised_simulator == "nmmc1") {
			// timestep to milliseconds
			timestep = timestep * 1000.0;
		}
	}

	std::vector<std::tuple<size_t, py::object>> group_projections;
	std::vector<std::tuple<size_t, std::tuple<py::object, py::object>>>
	    list_projections;
	for (size_t i = 0; i < source.connections().size(); i++) {
		const auto &conn = source.connections()[i];
		auto it = SUPPORTED_CONNECTIONS.find(conn.connector().name());

		if (it != SUPPORTED_CONNECTIONS.end() &&
		    conn.connector().group_connect(conn)) {
			// Group connections
			auto proj =
			    group_connect(populations, pypopulations, conn, pynn,
			                  m_normalised_simulator == "nest", timestep);
			group_projections.push_back(std::make_tuple(i, proj));
		}
		else {
			// Issue related to PyNN #625
			bool current_based = false;
			if (!populations[conn.pid_tar()].type().conductance_based &&
			    (m_normalised_simulator == "nest")) {
				current_based = true;
			}
			list_projections.emplace_back(
			    std::make_tuple(i, list_connect(pypopulations, conn, pynn,
			                                    current_based, timestep)));
		}
	}

	Real duration_rounded = 0;
	if (timestep != 0) {
		duration_rounded =
		    int((duration + timestep) / timestep) * timestep + timestep;
	}
	else {
		duration_rounded = duration;
	}
	Real duration_pure = 0.0;
	if (m_normalised_simulator == "nmmc1") {
		auto util = py::module::import("spinn_front_end_common.utilities");
		size_t time_scale = py::cast<size_t>(util.attr("globals_variables")
		                                         .attr("get_simulator")()
		                                         .attr("time_scale_factor"));
		duration_pure = Real(time_scale) * duration_rounded / 1000.0_R;
	}

	auto buildconn = std::chrono::steady_clock::now();
	try {
		if (m_normalised_simulator == "nest") {
			pynn.attr("run")(0);
			auto nest = py::module::import("nest");
			auto run_0 = std::chrono::steady_clock::now();
			nest.attr("Simulate")(duration_rounded);
			auto run_1 = std::chrono::steady_clock::now();
			duration_pure = std::chrono::duration<Real>(run_1 - run_0).count();
		}
		else {
			pynn.attr("run")(duration_rounded);
		}
	}
	catch (const pybind11::error_already_set &e) {
		try {
			pynn.attr("end");
		}
		catch (...) {
		}
		throw ExecutionError(e.what());
	}

	auto execrun = std::chrono::steady_clock::now();

	// fetch data
	if (m_normalised_simulator == "nest") {
		fetch_data_nest(populations, pypopulations);
	}
	else if (m_normalised_simulator == "nmmc1") {
		fetch_data_spinnaker(populations, pypopulations);
	}
	else {
		fetch_data_neo(populations, pypopulations);
	}

	for (size_t i = 0; i < group_projections.size(); i++) {
		size_t index = std::get<0>(group_projections[i]);
		if (source.connections()[index].connector().synapse()->learning()) {
			auto weights = get_weights(std::get<1>(group_projections[i]));
			source.connections()[index].connector()._store_learned_weights(
			    std::move(weights));
		}
	}

	for (size_t i = 0; i < list_projections.size(); i++) {
		size_t index = std::get<0>(list_projections[i]);
		if (source.connections()[index].connector().synapse()->learning()) {
			auto weights_exc =
			    get_weights(std::get<0>(std::get<1>(list_projections[i])));
			auto weights_inh =
			    get_weights(std::get<1>(std::get<1>(list_projections[i])));
			weights_exc.insert(weights_exc.end(), weights_inh.begin(),
			                   weights_inh.end());
			source.connections()[index].connector()._store_learned_weights(
			    std::move(weights_exc));
		}
	}

	pynn.attr("end")();

	auto finished = std::chrono::steady_clock::now();

	source.runtime({std::chrono::duration<Real>(finished - start).count(),
	                std::chrono::duration<Real>(execrun - buildconn).count(),
	                std::chrono::duration<Real>(buildconn - start).count(),
	                std::chrono::duration<Real>(finished - execrun).count(),
	                duration_pure});

	/*
	// Remove the log file
	if (!m_keep_log) {
	    unlink(log_path.c_str());
	}*/
}

py::object PyNN::spikey_create_source_population(const PopulationBase &pop,
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

	auto neuron_type = pynn.attr("SpikeSourceArray");

	py::module numpy = py::module::import("numpy");
	py::object pypop =
	    pynn.attr("Population")(numpy.attr("int")(pop.size()), neuron_type);
	pypop.attr("tset")("spike_times", spikes);

	return pypop;
}

py::object PyNN::spikey_create_homogeneous_pop(const PopulationBase &pop,
                                               py::module &pynn)
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
	    numpy.attr("int")(pop.size()),
	    pynn.attr(PyNN::get_neuron_class(pop.type()).c_str()), neuron_params);
	global_logger().warn("cypress",
	                     "Backend does not support explicit "
	                     "initialization of the membrane "
	                     "potential!");
	return pypop;
}
void PyNN::spikey_set_homogeneous_rec(const PopulationBase &pop,
                                      py::object &pypop, py::module &pynn)
{
	std::vector<std::string> signals = pop.type().signal_names;
	for (size_t j = 0; j < signals.size(); j++) {
		if (pop.signals().is_recording(j)) {
			if (signals[j] == "spikes") {
				pypop.attr("record")();
			}
			else if (signals[j] == "v") {
				pynn.attr("record_v")(py::list(pypop)[0], "");
				global_logger().warn(
				    "cypress", "Recording membrane only for the first neuron!");
			}
			else {
				throw ExecutionError(
				    "Spikey supports recording only for membrane and spikes!");
			}
		}
	}
}

void PyNN::spikey_set_inhomogeneous_rec(const PopulationBase &pop,
                                        py::object &pypop, py::module pynn)
{
	std::vector<std::string> signals = pop.type().signal_names;
	py::list pypop_list = py::list(pypop);
	for (size_t j = 0; j < signals.size(); j++) {
		if (signals[j] == "spikes") {
			pypop.attr("record")();
			global_logger().info(
			    "cypress", "Can only record spikes for a full population");
			continue;
		}
		py::list list;
		for (size_t k = 0; k < pop.size(); k++) {
			if (pop[k].signals().is_recording(j)) {
				list.append(pypop_list[k]);
			}
		}
		if (list.size() == 0) {
			continue;
		}

		if (signals[j] == "v") {
			pynn.attr("record_v")(list, "");
			if (list.size() > 1) {
				global_logger().info("cypress",
				                     "Spikey might not support recording of "
				                     "more than one neuron");
			}
		}
		else if (signals[j] == "gsyn_exc" || signals[j] == "gsyn_inh") {
			global_logger().info("cypress",
			                     "Spikey does not support recording of "
			                     "gsyn_*");
		}
	}
}

void PyNN::spikey_set_inhomogeneous_parameters(const PopulationBase &pop,
                                               py::object &pypop)
{
	const auto &params = pop[0].parameters();
	const auto &param_names = pop.type().parameter_names;
	py::list pypop_list = py::list(pypop);

	for (size_t id = 0; id < params.size(); id++) {
		std::vector<Real> new_params;
		for (size_t neuron_id = 0; neuron_id < pop.size(); neuron_id++) {
			pypop_list[neuron_id].attr(param_names[id].c_str()) =
			    pop[neuron_id].parameters()[id];
		}
	}
}
namespace {
bool inline check_full_pop(ConnectionDescriptor conn,
                           const std::vector<PopulationBase> &populations)
{
	return conn.nid_src0() == 0 &&
	       conn.nid_src1() == NeuronIndex(populations[conn.pid_src()].size()) &&
	       conn.nid_tar0() == 0 &&
	       conn.nid_tar1() == NeuronIndex(populations[conn.pid_tar()].size());
}
}  // namespace

void PyNN::spikey_get_spikes(PopulationBase pop, py::object &pypop)
{
	int64_t first_id = py::cast<int64_t>(py::list(pypop)[0]);
	py::object spikes_py = pypop.attr("getSpikes")();
	Matrix<double> spikes = PyNN::matrix_from_numpy<double>(spikes_py, true);

	// Spike source get negative ids
	bool negative = false;
	if (first_id < 0) {
		first_id = -first_id;
		negative = true;
	}

	if (!negative) {
		auto idx = pop[0].type().signal_index("spikes");
		for (size_t neuron_id = 0; neuron_id < pop.size(); neuron_id++) {
			size_t counter = 0;
			for (size_t i = 0; i < spikes.cols(); i++) {
				if (spikes(0, i) == neuron_id + first_id) {
					counter++;
				}
			}

			auto data = std::make_shared<Matrix<Real>>(counter, 1);
			counter = 0;
			for (size_t i = 0; i < spikes.cols(); i++) {
				if (spikes(0, i) == neuron_id + first_id) {
					(*data)(counter, 0) = Real(spikes(1, i));
					counter++;
				}
			}
			pop[neuron_id].signals().data(idx.value(), std::move(data));
		}
	}
	else {
		auto idx = pop[0].type().signal_index("spikes");
		for (size_t neuron_id = 0; neuron_id < pop.size(); neuron_id++) {
			size_t counter = 0;
			for (size_t i = 0; i < spikes.cols(); i++) {
				if (-spikes(0, i) == neuron_id + first_id) {
					counter++;
				}
			}

			auto data = std::make_shared<Matrix<Real>>(counter, 1);
			counter = 0;
			for (size_t i = 0; i < spikes.cols(); i++) {
				if (-spikes(0, i) == neuron_id + first_id) {
					(*data)(counter, 0) = Real(spikes(1, i));
					counter++;
				}
			}
			pop[neuron_id].signals().data(idx.value(), std::move(data));
		}
	}
}

void PyNN::spikey_get_voltage(NeuronBase neuron, py::module &pynn)
{
	Matrix<double> membrane =
	    PyNN::matrix_from_numpy<double>(pynn.attr("membraneOutput"));
	Matrix<double> time =
	    PyNN::matrix_from_numpy<double>(pynn.attr("timeMembraneOutput"));

	auto idx = neuron.type().signal_index("v");
	auto data = std::make_shared<Matrix<Real>>(time.rows(), 2);
	for (size_t i = 0; i < time.size(); i++) {
		(*data)(i, 0) = Real(time[i]);
		(*data)(i, 1) = Real(membrane[i]);
	}
	neuron.signals().data(idx.value(), std::move(data));
}

namespace {
static std::map<LogSeverity, std::string> loglevelnames{{DEBUG, "DEBUG"},
                                                        {INFO, "INFO"},
                                                        {WARNING, "WARN"},
                                                        {ERROR, "ERROR"},
                                                        {FATAL_ERROR, "FATAL"}};

/**
 * Used internally to get weights from plastic synaptic connections
 *
 * @param proj python projection object
 * @param pypop_src Python source population
 * @param pypop_tar Python target population
 * @return std::vector< cypress::LocalConnection > List of local connections
 */
std::vector<LocalConnection> spikey_get_weights(py::object &proj,
                                                py::object &pypop_src,
                                                py::object &pypop_tar)
{

	std::vector<LocalConnection> weights;
	py::list pyweights =
	    proj.attr("getWeightsHW")("readHW"_a = true, "format"_a = "list");
	py::list delays = proj.attr("getDelays")();

	py::object connections = proj.attr("connections")();
	for (size_t connect_id = 0; connect_id < pyweights.size(); connect_id++) {
		py::list conn = connections.attr("next")();
		weights.push_back(LocalConnection(py::cast<NeuronIndex>(conn[0]),
		                                  py::cast<NeuronIndex>(conn[1]),
		                                  py::cast<Real>(pyweights[connect_id]),
		                                  py::cast<Real>(delays[connect_id])));
	}

	int64_t first_id = py::cast<int64_t>(py::list(pypop_src)[0]);
	if (first_id > 0) {
		for (auto &w : weights) {
			w.src = w.src - first_id;
		}
	}
	else if (first_id < 0) {
		for (auto &w : weights) {
			w.src = -w.src + first_id;
		}
	}

	first_id = py::cast<int64_t>(py::list(pypop_tar)[0]);
	if (first_id > 0) {
		for (auto &w : weights) {
			w.tar = w.tar - first_id;
		}
	}
	else if (first_id < 0) {
		for (auto &w : weights) {
			w.tar = -w.tar + first_id;
		}
	}

	return weights;
}
}  // namespace

void PyNN::spikey_run(NetworkBase &source, Real duration, py::module &pynn,
                      py::dict dict)
{
	auto start = std::chrono::system_clock::now();
	py::module pylogging = py::module::import("pylogging");
	std::vector<std::string> logger({"HAL.Cal", "HAL.Ctr", "HAL.PyS", "HAL.PRC",
	                                 "HAL.Spi", "PyN.cfg", "PyN.syn", "PyN.wks",
	                                 "Default"});
	for (std::string i : logger) {
		py::str py_s = py::str(i).attr("encode")("utf-8");
		std::string loglevel = loglevelnames[global_logger().min_level()];
		pylogging.attr("set_loglevel")(
		    pylogging.attr("get")(py_s),
		    pylogging.attr("LogLevel")
		        .attr(loglevel
		                  .c_str()));  // ALL DEBUG ERROR FATAL INFO TRACE WARN
	}
	try {
		pynn.attr("setup")(**dict);
	}
	catch (const pybind11::error_already_set &e) {
		throw ExecutionError(e.what());
	}

	const std::vector<PopulationBase> &populations = source.populations();
	std::vector<py::object> pypopulations;

	for (size_t i = 0; i < populations.size(); i++) {
		if (populations.size() == 0) {
			pypopulations.push_back(py::object());
			continue;
		}

		if (&populations[i].type() == &SpikeSourceArray::inst()) {
			pypopulations.push_back(
			    spikey_create_source_population(populations[i], pynn));
			const bool homogeneous_rec = populations[i].homogeneous_record();
			if (homogeneous_rec) {
				spikey_set_homogeneous_rec(populations[i], pypopulations.back(),
				                           pynn);
			}
			else {
				spikey_set_inhomogeneous_rec(populations[i],
				                             pypopulations.back(), pynn);
			}
		}
		else {
			bool homogeneous = populations[i].homogeneous_parameters();
			pypopulations.push_back(
			    spikey_create_homogeneous_pop(populations[i], pynn));

			if (!homogeneous) {
				spikey_set_inhomogeneous_parameters(populations[i],
				                                    pypopulations.back());
			}
			const bool homogeneous_rec = populations[i].homogeneous_record();
			if (homogeneous_rec) {
				spikey_set_homogeneous_rec(populations[i], pypopulations.back(),
				                           pynn);
			}
			else {
				spikey_set_inhomogeneous_rec(populations[i],
				                             pypopulations.back(), pynn);
			}
		}
	}

	std::vector<std::tuple<size_t, py::object>> group_projections;

	for (size_t i = 0; i < source.connections().size(); i++) {
		const auto &conn = source.connections()[i];
		auto it = SUPPORTED_CONNECTIONS.find(conn.connector().name());

		if (it != SUPPORTED_CONNECTIONS.end() &&
		    conn.connector().group_connect(conn) &&
		    check_full_pop(conn, populations)) {
			// Group connections
			auto proj = group_connect7(populations, pypopulations, conn, pynn);
			group_projections.push_back(std::make_tuple(i, proj));
		}
		else {
			list_connect7(pypopulations, conn, pynn);
		}
	}

	auto buildconn = std::chrono::system_clock::now();

	pynn.attr("hardware").attr("hwa").attr("autoSTDPFrequency") = duration;
	try {
		pynn.attr("run")(duration);
	}
	catch (const pybind11::error_already_set &e) {
		throw ExecutionError(e.what());
	}

	auto execrun = std::chrono::system_clock::now();

	for (size_t i = 0; i < populations.size(); i++) {
		if (populations[i].size() == 0) {
			continue;
		}
		std::vector<std::string> signals = populations[i].type().signal_names;
		for (size_t j = 0; j < signals.size(); j++) {
			bool is_recording = false;
			for (auto neuron : populations[i]) {
				if (neuron.signals().is_recording(j)) {
					is_recording = true;
					break;
				}
			}

			if (is_recording) {
				if (signals[j] == "spikes") {
					spikey_get_spikes(populations[i], pypopulations[i]);
				}
				else if (signals[j] == "v") {
					for (auto neuron : populations[i]) {
						if (neuron.signals().is_recording(j)) {
							spikey_get_voltage(neuron, pynn);
						}
					}
				}
				else {
					throw;  // TODO
				}
			}
		}
	}

	// Get Weights for learned/plastic synapses
	for (size_t i = 0; i < group_projections.size(); i++) {
		size_t index = std::get<0>(group_projections[i]);
		if (source.connections()[index].connector().synapse()->learning()) {
			auto proj = std::get<1>(group_projections[i]);
			auto weights = spikey_get_weights(
			    proj, pypopulations[source.connections()[index].pid_src()],
			    pypopulations[source.connections()[index].pid_tar()]);
			source.connections()[index].connector()._store_learned_weights(
			    std::move(weights));
		}
	}

	pynn.attr("end")();
	auto finished = std::chrono::system_clock::now();

	source.runtime({std::chrono::duration<Real>(finished - start).count(),
	                std::chrono::duration<Real>(execrun - buildconn).count(),
	                std::chrono::duration<Real>(buildconn - start).count(),
	                std::chrono::duration<Real>(execrun - start).count(),
	                duration * 1.0e-7_R});
}
}  // namespace cypress
