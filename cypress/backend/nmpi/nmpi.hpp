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

#pragma once

#ifndef CYPRESS_BACKEND_NMPI_HPP
#define CYPRESS_BACKEND_NMPI_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <cypress/core/backend.hpp>

namespace cypress {

// Forward declarations
class PyNN;

/**
 * The NMPI backend executes the program on a Neuromorphic Platform Interface
 * server.
 */
class NMPI : public Backend {
private:
	/**
	 * Pointer at the actual PyNN backend instance.
	 */
	std::unique_ptr<PyNN> m_pynn;

	/**
	 * This method just forwards the given data to the PyNN instance -- it is
	 * not called on the client computer, since the constructor intercepts the
	 * program flow.
	 */
	void do_run(NetworkBase &network, float duration) const override;

public:
	/**
	 * Exception thrown when the chosen PyNN backend has no associated NMPI
	 * platform.
	 */
	class NoNMPIPlatform : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	/**
	 * Exception thrown if the command line arguments contain errors.
	 */
	class InvalidCommandLine : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	/**
	 * Creates a new instance of the NMPI backend. On the client computer, the
	 * constructor does not return. Instead, it uploads the current executable
	 * to the NMPI server and runs it there.
	 *
	 * @param pynn is a pointer at the PyNN backend that should be used.
	 * @param argc is the number of command line arguments passed at the
	 * executable.
	 * @param argv is the array containing the command line arguments.
	 * @param files is a vector containing the names of additional files that
	 * should be uploaded to the neuromorphic compute platform.
	 * @param scan_args if true, automagically scans the command line arguments
	 * for for filenames and uploads these files too.
	 */
	NMPI(std::unique_ptr<PyNN> pynn, int &argc, const char *argv[],
	     const std::vector<std::string> &files = std::vector<std::string>(),
	     bool scan_args = true);

	/**
	 * Creates a new instance of the NMPI backend. On the client computer, the
	 * constructor does not return. Instead, it uploads the current executable
	 * to the NMPI server and runs it there.
	 *
	 * @param pynn_backend is the name of the pynn backend that should be used
	 * on the neuromorphic compute platform.
	 * @param argv is the array containing the command line arguments.
	 * @param files is a vector containing the names of additional files that
	 * should be uploaded to the neuromorphic compute platform.
	 * @param scan_args if true, automagically scans the command line arguments
	 * for for filenames and uploads these files too.
	 */
	NMPI(const std::string &pynn_backend, int &argc, const char *argv[],
	     const std::vector<std::string> &files = std::vector<std::string>(),
	     bool scan_args = true);

	/**
	 * Destructor. Destroys the PyNN backend.
	 */
	~NMPI() override;

	/**
	 * Returns true if the given arguments consititure a call on the NMPI server
	 * in which case no further checking should be performed.
	 */
	static bool check_args(int argc, const char *argv[]);
};
}

#endif /* CYPRESS_BACKEND_NMPI_HPP */
