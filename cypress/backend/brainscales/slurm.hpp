/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2017 Christoph Jenzen
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

#ifndef CYPRESS_BACKEND_SLURM_HPP
#define CYPRESS_BACKEND_SLURM_HPP

#include <cypress/backend/pynn/pynn.hpp>

#include <string>
#include <vector>

#include <cypress/backend/serialize/to_json.hpp>
#include <cypress/core/backend.hpp>
#include <cypress/util/json.hpp>

namespace cypress {

class Slurm : public ToJson {
private:
	// Flags for different running modes:
	// m_write_binnf sets wether binnf files should be written
	bool m_write_binnf = true;
	// Trigger the python process via srun
	bool m_exec_python = true;
	// Read back results from the python process
	bool m_read_results = true;

	// keep json/cbor files
	bool m_keep_file = false;

	// True: use json, false: use cbor
	bool m_json = false;

	std::string m_norm_simulator;

	void do_run(NetworkBase &network, Real duration) const override;

public:
	/**
	 * Constructor of the Slurm backend. Throws an exception if the given PyNN
	 * backend does not exist.
	 *
	 * @param simulator is the name of the simulator backend to be used by PyNN.
	 * Use the static backends method to list available backends.
	 * @param setup contains additional setup information that should be passed
	 * to the backend.
	 */
	Slurm(const std::string &simulator, const Json &setup = Json());

	/**
	 * Destructor of the Slurm class.
	 */
	~Slurm() override;

	void set_flags(size_t num);

	void set_base_filename(const std::string &filename) { m_path = filename; }
	const std::string &get_base_filename() const { return m_path; }
};
}  // namespace cypress

#endif /* CYPRESS_BACKEND_SLURM_HPP */
