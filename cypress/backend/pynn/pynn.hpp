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

#ifndef CYPRESS_BACKEND_PYNN_HPP
#define CYPRESS_BACKEND_PYNN_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include <cypress/util/json.hpp>
#include <cypress/core/backend.hpp>

namespace cypress {
/**
 * The Backend class is an abstract base class which provides the facilities
 * for network execution.
 */
class PyNN : public Backend {
private:
	std::string m_simulator;
	std::string m_normalised_simulator;
	std::vector<std::string> m_imports;
	Json m_setup;

	void do_run(NetworkBase &network, float duration) const override;

public:
	/**
	 * Exception thrown if the given PyNN backend is not found.
	 */
	class PyNNSimulatorNotFound : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	/**
	 * Constructor of the PyNN backend. Throws an exception if the given PyNN
	 * backend does not exist.
	 *
	 * @param simulator is the name of the simulator backend to be used by PyNN.
	 * Use the static backends method to list available backends.
	 * @param setup contains additional setup information that should be passed
	 * to the backend.
	 */
	PyNN(const std::string &simulator, const Json &setup = Json());

	/**
	 * Destructor of the PyNN class.
	 */
	~PyNN() override;

	/**
	 * Calculates the timestep the simulation will be running with. This does
	 * not make any sense for analogue hardware, why zero is returned in this
	 * case.
	 *
	 * @return the simulation timestep in milliseconds.
	 */
	float timestep();

	/**
	 * Returns the canonical name of the backend.
	 */
	std::string name() const override { return m_normalised_simulator; }

	/**
	 * Returns the simulator name as provided by the user.
	 */
	const std::string &simulator() const { return m_simulator; }

	/**
	 * Returns the canonical simulator name.
	 */
	const std::string &normalised_simulator() const
	{
		return m_normalised_simulator;
	}

	/**
	 * Returns the Python imports that tried to be used when starting the
	 * platform.
	 */
	const std::vector<std::string> &imports() const { return m_imports; }

	/**
	 * Returns the NMPI platform name corresponding to the currently chosen PyNN
	 * backend.
	 */
	std::string nmpi_platform() const;

	/**
	 * Lists available PyNN simulators.
	 */
	static std::vector<std::string> simulators();
};
}

#endif /* CYPRESS_BACKEND_PYNN_HPP */
