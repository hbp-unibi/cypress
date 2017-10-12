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

#include <cstdio>

#include <string>
#include <thread>
#include <vector>

#include <cypress/backend/pynn/slurm.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/json.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>

namespace cypress {

void Slurm::set_flags(size_t num)
{
	switch (num) {
		case 0:
			m_write_binnf = true;
			m_exec_python = true;
			m_read_results = true;
			break;
		case 1:
			m_write_binnf = true;
			m_exec_python = true;
			m_read_results = false;
			break;
		case 2:
			m_write_binnf = false;
			m_exec_python = false;
			m_read_results = true;
			break;
		case 3:
			m_write_binnf = true;
			m_exec_python = false;
			m_read_results = false;
			break;
		case 4:
			m_write_binnf = false;
			m_exec_python = true;
			m_read_results = false;
			break;
		case 5:
			m_write_binnf = false;
			m_exec_python = true;
			m_read_results = true;
			break;
		default:
			m_write_binnf = true;
			m_exec_python = true;
			m_read_results = true;
			break;
	}
}

Slurm::Slurm(const std::string &simulator, const Json &setup)
    : PyNN(simulator, setup)
{
	if (setup.find("slurm_mode") != setup.end()) {
		set_flags(setup["slurm_mode"]);
	}
	if (setup.find("slurm_filename") != setup.end()) {
		m_filename = setup["slurm_filename"];
	}
	else {
		// Generate filename
		m_filename = ".cypress_" + m_normalised_simulator + "_XXXXXX";
		filesystem::tmpfile(m_filename);
	}
}
namespace {
bool file_is_empty(std::string filename)
{
	std::ifstream file(filename);
	return file.good() && (file.peek() == std::ifstream::traits_type::eof());
}
}

void Slurm::do_run(NetworkBase &network, Real duration) const
{
	if (m_write_binnf) {
		// TODO Check whether file exists
		write_binnf(network, m_filename);
	}

	if (m_exec_python) {
		std::string import = get_import(m_imports, m_simulator);
		std::vector<std::string> params;

		// Creating file, so that the python part won't create fifos and block
		// afterwards
		std::ofstream(m_filename + "_res", std::ios::out).close();
		std::ofstream(m_filename + "_log", std::ios::out).close();

		// Get the current working directory
		std::string current_dir = std::string(get_current_dir_name()) + "/";

		// Set simulator dependent options for slurm
		if (m_normalised_simulator == "nmpm1") {
			std::string hicann = "367", wafer = "33";
			if (m_setup.find("hicann") != m_setup.end()) {
				Json j_hicann = m_setup["hicann"];
				hicann = j_hicann.dump(-1);
				if (j_hicann.is_array()) {
					hicann.erase(hicann.begin());
					hicann.erase(hicann.end() - 1);
				}
			}
			else {
				network.logger().warn("cypress", "Using default hicann!");
			}
			if (m_setup.find("wafer") != m_setup.end()) {
				Json j_wafer = m_setup["wafer"];
				wafer = j_wafer.dump(-1);
				if (j_wafer.is_array()) {
					wafer.erase(wafer.begin());
					wafer.erase(wafer.end());
				}
			}
			else {
				network.logger().warn("cypress", "Using default wafer!");
			}

			params = std::vector<std::string>({
			    "-p", "experiment", "--wmod", wafer, "--hicann", hicann, "bash",
			    "-c",
			});
		}
		else if (m_normalised_simulator == "spikey") {
			size_t station = 538;
			if (m_setup.find("station") != m_setup.end()) {
				station = m_setup["station"];
			}
			else {
				network.logger().warn("cypress", "Using default spikey 538!");
			}
			params = std::vector<std::string>({
			    "-p", "spikey", "--gres", "station" + std::to_string(station),
			    "bash", "-c",
			});
		}

		else if (m_normalised_simulator == "ess") {
			params = std::vector<std::string>({
			    "-p", "simulation", "-c", "8", "--mem", "30G", "bash", "-c",
			});
		}
		else {
			throw NotSupportedException("Simulator " + m_normalised_simulator +
			                            " not supported with Slurm!");
		}

		// Add the bash script executed by srun
		params.push_back(
		    "ls > /dev/null; python " +
		    Resources::PYNN_INTERFACE.open_local(m_filename + ".py") + " run " +
		    "--simulator " + m_normalised_simulator + " --library " + import +
		    " --setup " + "'" + m_setup.dump() + "'" + " --duration " +
		    std::to_string(duration) + " --in " + current_dir + m_filename +
		    "_stdin" + " --out " + current_dir + m_filename + "_res" +
		    " --logs " + current_dir + m_filename + "_log; ls >/dev/null");

		// Synchronize files on servers (Heidelberg setup...)
		system("ls > /dev/null");

		// Run the srun (non-blocking at this point)
		Process proc("srun", params);

		std::ofstream log_stream(m_filename);

		// This process is only meant to cover the first milliseconds before
		// all communication is switched to files.
		std::thread log_thread_beg(Process::generic_pipe,
		                           std::ref(proc.child_stdout()),
		                           std::ref(std::cout));

		// Continiously track stderr of child
		std::thread err_thread(Process::generic_pipe,
		                       std::ref(proc.child_stderr()),
		                       std::ref(log_stream));

		// Wait for process to finish
		int res = proc.wait();
		// Synchronize files on servers (Heidelberg setup...)
		system("ls > /dev/null");
		if (res != 0 || file_is_empty(m_filename + "_res")) {
			err_thread.join();
			log_thread_beg.join();
			// Explicitly state if the process was killed by a signal
			if (res < 0) {
				network.logger().error(
				    "cypress", "Simulator child process killed by signal " +
				                   std::to_string(-res));
			}
			throw ExecutionError(
			    std::string("Error while executing the simulator, see ") +
			    m_filename + " for the simulators stderr output");
		}
		err_thread.join();
		log_thread_beg.join();
	}

	if (m_read_results) {
		read_back_binnf(network, m_filename);
		unlink((m_filename + "_stdin").c_str());
		unlink((m_filename + "_res").c_str());
		unlink((m_filename + "_log").c_str());
		unlink((m_filename + ".py").c_str());
		unlink((m_filename).c_str());
	}
}

Slurm::~Slurm() = default;
}
