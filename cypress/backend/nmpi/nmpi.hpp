/*
 *  CppNAM -- C++ Neural Associative Memory Simulator
 *  Copyright (C) 2016  Christoph Jenzen, Andreas Stöckel
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

#include <cypress/core/backend.hpp>

namespace cypress {

// Forward declarations
class PyNN;

/**
 * The NMPI backend executes the program on a Neuromorphic Platform Interface
 * server.
 */
class NMPI: public Backend {
private:
	/**
	 * Runs the 
	 */
	void do_run(Network &network, float duration) const override;

public:
	NMPI(const PyNN &pynn, int argc, const char*argv[], std::vector<std::string> &files = std::vector<std::string>());
};

}

#endif /* CYPRESS_BACKEND_NMPI_HPP */
