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

#ifndef CYPRESS_BACKEND_NEST_HPP
#define CYPRESS_BACKEND_NEST_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include <cypress/core/backend.hpp>

namespace cypress {
/**
 * The NEST backend directly runs the simulation on the NEST simulator without
 * using PyNN. A recent version of NEST must be installed.
 */
class NEST : public Backend {
private:
	void do_run(NetworkBase &network, float duration) const override;

public:
	/**
	 * Exception thrown if the given PyNN backend is not found.
	 */
	class NESTSimulatorNotFound : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	/**
	 * Constructor of the NEST backend. For now takes no parameters.
	 */
	NEST();

	/**
	 * Destructor of the PyNN class.
	 */
	~NEST() override;

	/**
	 * Returns true if NEST is installed.
	 */
	static bool installed();

	/**
	 * Returns the installed NEST version or an empty string if NEST is not
	 * found.
	 */
	static std::string version();
};
}

#endif /* CYPRESS_BACKEND_NEST_HPP */
