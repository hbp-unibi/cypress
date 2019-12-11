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

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <cypress/backend/pynn/pynn.hpp>

#include <unistd.h>
#include <iostream>
#include <sstream>

#include <cypress/backend/brainscales/brainscales_lib.hpp>
#include <cypress/backend/nmpi/nmpi.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>

namespace cypress {

/**
 * Map between the canonical simulator name and the NMPI platform name.
 */
static const std::unordered_map<std::string, std::string> SIMULATOR_NMPI_MAP = {
    {"nmmc1", "SpiNNaker"},
    {"nmpm1", "BrainScaleS"},
    {"ess", "BrainScaleS-ESS"},
    {"spikey", "Spikey"},
};

std::string nmpi_platform(const std::string &norm_sim)
{
	auto it = SIMULATOR_NMPI_MAP.find(norm_sim);
	if (it != SIMULATOR_NMPI_MAP.end()) {
		return it->second;
	}
	return std::string();
}

static const std::string SERVER_ARG = "NMPI_SRV";

/**
 * Assembles the parameters which are passed to the broker process and executes
 * it.
 */
static int run_broker(const std::vector<std::string> &args,
                      const std::vector<std::string> &external_files,
                      const std::string &base, const std::string &platform,
                      const int wafer = 0)
{
	std::vector<std::string> params;

	params.emplace_back(Resources::NMPI_BROKER.open());

	params.emplace_back("--executable");
	params.emplace_back(args[0]);

	if (external_files.size() > 0) {
		params.emplace_back("--files");
		for (size_t i = 0; i < external_files.size(); i++) {
			params.emplace_back(external_files[i]);
		}
	}

	if (args.size() > 1) {
		params.emplace_back("--args");
		for (size_t i = 1; i < args.size(); i++) {
			params.emplace_back(args[i]);
		}
	}

	params.emplace_back("--base");
	params.emplace_back(base);

	params.emplace_back("--platform");
	params.emplace_back(platform);

	if (wafer != 0) {
		params.emplace_back("--wafer");
		params.emplace_back(std::to_string(wafer));
	}

	return Process::exec_no_redirect("python", params);
}

void NMPI::init(std::unique_ptr<Backend> pynn, int &argc, const char *argv[],
                const std::vector<std::string> &files, bool scan_args)
{
	m_pynn = std::move(pynn);
	// Fetch the NMPI platform name
	const std::string platform = nmpi_platform(m_pynn->name());
	if (platform.empty()) {
		throw NoNMPIPlatform(
		    "The chosen PyNN backend does not possess an NMPI platform.");
	}

	std::vector<std::string> external_files(files);
	std::vector<int> external_file_ids(files.size(), -1);

	// There must be at least one command line argument
	if (argc == 0) {
		throw InvalidCommandLine("No command line arguments given.");
	}

	// Copy the command line arguments to a vector
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++) {
		args.emplace_back(argv[i]);
	}

	// If the last argument is SERVER_ARG, we're running on the server. Hide
	// the argument and continue.
	if (args.size() >= 2 && args.back() == SERVER_ARG) {
		argc--;
		return;
	}

	// Make sure the first argument points at an executable
	if (access(args[0].c_str(), R_OK | X_OK) != 0) {
		throw InvalidCommandLine(
		    "Executable not specified in command line argument zero.");
	}

	// If scan_args is set, check whether the command line contains references
	// to any files and add those to the file list
	if (scan_args) {
		for (size_t i = 1; i < args.size(); i++) {
			if (access(args[i].c_str(), R_OK) == 0) {
				external_files.emplace_back(args[i]);
				external_file_ids.emplace_back(i);
			}
		}
	}

	// Canonicalise the external_files list, replace the arguments
	filesystem::canonicalise_files(external_files);
	for (size_t i = 0; i < external_files.size(); i++) {
		if (external_file_ids[i] >= 0) {
			args[external_file_ids[i]] = external_files[i];
		}
	}

	// Calculate the base directory
	std::string base = "";
	if (external_files.size() > 0) {
		base =
		    filesystem::longest_common_path(filesystem::dirs(external_files));
	}
	else {
		base = filesystem::longest_common_path(filesystem::dirs({args[0]}));
	}

	// Add SERVER_ARG to the arguments
	args.emplace_back(SERVER_ARG);
	// Give control to the NMPI broker, relay its exit code
	exit(run_broker(args, external_files, base, platform));
}

namespace {
std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string elem;
	while (getline(ss, elem, delim)) {
		elems.push_back(elem);
	}
	return elems;
}

std::string join(const std::vector<std::string> &elems, char delim)
{
	bool first = true;
	std::stringstream ss;
	for (const std::string &elem : elems) {
		if (!first) {
			ss << delim;
		}
		first = false;
		ss << elem;
	}
	return ss.str();
}
}  // namespace
NMPI::NMPI(std::unique_ptr<Backend> pynn, int &argc, const char *argv[],
           const std::vector<std::string> &files, bool scan_args)
{
	init(std::move(pynn), argc, argv, files, scan_args);
}

NMPI::NMPI(const std::string &backend, int &argc, const char *argv[],
           const std::vector<std::string> &files, bool scan_args)
{
	// This is either a non-NMPI platform or the BrainScaleS case, not on server
	auto parts = split(backend, '.');
	if (parts[0] == "nmpi") {
		parts.erase(parts.begin());
	}
	auto backend_str = join(parts, '.');
	parts = split(backend_str, '=');
	if (parts[0] == "nmpm1" || parts[0] == "BrainScaleS" ||
	    parts[0] == "brainscales") {
		Json setup;
		if (parts.size() > 1) {
			setup = Json::parse(parts[1]);
		}
		init_bs(parts[0], argc, argv, setup, files, scan_args);
	}
	else {
		auto ptr = std::make_unique<PyNN>(backend_str);
		init(std::move(ptr), argc, argv, files, scan_args);
	}
}

namespace {
bool file_exists(const std::string &filename)
{
	std::ifstream ifs(filename);
	return ifs.good();
}
}  // namespace

void NMPI::init_bs(const std::string &bs_backend, int &argc, const char *argv[],
                   Json setup, const std::vector<std::string> &files,
                   bool scan_args)
{

	const std::string platform = nmpi_platform(bs_backend);
	if (platform.empty()) {
		throw NoNMPIPlatform(
		    "The chosen PyNN backend does not possess an NMPI platform.");
	}

	std::vector<std::string> external_files(files);
	std::vector<int> external_file_ids(files.size(), -1);
	std::string bs_lib_path;
	if (file_exists("libBS2CYPRESS.so")) {
		bs_lib_path = "libBS2CYPRESS.so";
	}
	else if (file_exists(std::string(BS_LIBRARY_INSTALL_PATH))) {
		bs_lib_path = BS_LIBRARY_INSTALL_PATH;
	}
	else if (file_exists(std::string(BS_LIBRARY_PATH))) {
		bs_lib_path = BS_LIBRARY_PATH;
	}
	else {
		throw CypressException(
		    "libBS2CYPRESS not found! Make sure it is provided locally or "
		    "install correctly!");
	}

	global_logger().debug("cypress", "libBS2CYPRESS path " + bs_lib_path);
	external_files.emplace_back(std::string(bs_lib_path));
	external_file_ids.emplace_back(-1);

	// There must be at least one command line argument
	if (argc == 0) {
		throw InvalidCommandLine("No command line arguments given.");
	}

	// Copy the command line arguments to a vector
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++) {
		args.emplace_back(argv[i]);
	}

	// If the last argument is SERVER_ARG, we're running on the server. Hide
	// the argument and continue.
	if (args.size() >= 2 && args.back() == SERVER_ARG) {
		argc--;
		m_pynn = std::unique_ptr<Backend>(
		    BS_Lib::instance().create_bs_backend(setup));
		return;
	}

	// Make sure the first argument points at an executable
	if (access(args[0].c_str(), R_OK | X_OK) != 0) {
		throw InvalidCommandLine(
		    "Executable not specified in command line argument zero.");
	}

	// If scan_args is set, check whether the command line contains references
	// to any files and add those to the file list
	if (scan_args) {
		for (size_t i = 1; i < args.size(); i++) {
			if (access(args[i].c_str(), R_OK) == 0) {
				external_files.emplace_back(args[i]);
				external_file_ids.emplace_back(i);
			}
		}
	}

	// Canonicalise the external_files list, replace the arguments
	filesystem::canonicalise_files(external_files);
	for (size_t i = 0; i < external_files.size(); i++) {
		if (external_file_ids[i] >= 0) {
			args[external_file_ids[i]] = external_files[i];
		}
	}

	// Calculate the base directory
	const std::string base =
	    filesystem::longest_common_path(filesystem::dirs(external_files));

	// Add SERVER_ARG to the arguments
	args.emplace_back(SERVER_ARG);

	int wafer = 0;
	if (setup.find("wafer") != setup.end()) {
		if (setup["wafer"].is_array()) {
			global_logger().warn("cypress", "Currently not supported");
			wafer = setup["wafer"][0];
		}
		else {
			wafer = int(setup["wafer"]);
		}
	}
	else {
		wafer = 33;
		global_logger().info("cypress", "NMPI: Taking default wafer 33");
	}

	// Give control to the NMPI broker, relay its exit code
	exit(run_broker(args, external_files, base, platform, wafer));
}

NMPI::~NMPI() = default;

std::unordered_set<const NeuronType *> NMPI::supported_neuron_types() const
{
	return m_pynn->supported_neuron_types();
}

std::string NMPI::name() const { return m_pynn->name(); }

bool NMPI::check_args(int argc, const char *argv[])
{
	return (argc >= 2 && argv[argc - 1] == SERVER_ARG);
}

void NMPI::do_run(NetworkBase &network, Real duration) const
{
	m_pynn->run(network, duration);
}
}  // namespace cypress
