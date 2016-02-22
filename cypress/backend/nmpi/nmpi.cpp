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

#include <iostream>

#include <unistd.h>

#include <cypress/backend/nmpi/nmpi.hpp>
#include <cypress/backend/pynn/pynn.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/process.hpp>

namespace cypress {

static const std::string SERVER_ARG = "NMPI_SRV";

/**
 * Assembles the parameters which are passed to the broker process and executes
 * it.
 */
static int run_broker(const std::vector<std::string> &args,
                      const std::vector<std::string> &external_files,
                      const std::string &base, const std::string &platform)
{
	std::vector<std::string> params;

	params.emplace_back(Resources::NMPI_BROKER.open());

	params.emplace_back("--executable");
	params.emplace_back(args[0]);

	if (external_files.size() > 1) {
		params.emplace_back("--files");
		for (size_t i = 1; i < external_files.size(); i++) {
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

	return Process::exec("python", params, std::cin, std::cout, std::cerr);
}

NMPI::NMPI(std::unique_ptr<PyNN> pynn, int &argc, const char *argv[],
           const std::vector<std::string> &files, bool scan_args)
    : m_pynn(std::move(pynn))
{
	// Fetch the NMPI platform name
	const std::string platform = m_pynn->nmpi_platform();
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
	external_files.emplace_back(args[0]);
	external_file_ids.emplace_back(0);

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

	// Give control to the NMPI broker, relay its exit code
	exit(run_broker(args, external_files, base, platform));
}

NMPI::NMPI(const std::string &pynn_backend, int &argc, const char *argv[],
           const std::vector<std::string> &files, bool scan_args)
    : NMPI(std::make_unique<PyNN>(pynn_backend), argc, argv, files, scan_args)
{
}

NMPI::~NMPI() = default;

bool NMPI::check_args(int argc, const char *argv[])
{
	return (argc >= 2 && argv[argc - 1] == SERVER_ARG);
}

void NMPI::do_run(NetworkBase &network, float duration) const
{
	m_pynn->run(network, duration);
}
}
