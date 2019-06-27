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
#include <cypress/backend/pynn/pynn.hpp>

#include <cypress/backend/brainscales/slurm.hpp>

#include <cstdio>

#include <string>
#include <thread>
#include <vector>

#include <cypress/backend/resources.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/json.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>

namespace cypress {

static const std::unordered_map<std::string, std::string>
    NORMALISED_SIMULATOR_NAMES = {
        {"hardware.spikey", "spikey"},
        {"spikey", "spikey"},
        {"Spikey", "spikey"},
        {"brainscales", "nmpm1"},
        {"BrainScaleS", "nmpm1"},
        {"nmpm1", "nmpm1"},
        {"ess", "ess"},
};

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
    : ToJson(simulator, setup)
{
	if (m_setup.find("slurm_mode") != m_setup.end()) {
		set_flags(setup["slurm_mode"]);
		m_setup.erase(m_setup.find("slurm_mode"));
	}
	if (m_setup.find("slurm_filename") != m_setup.end()) {
		m_path = setup["slurm_filename"];
		m_setup.erase(m_setup.find("slurm_filename"));
	}

	if (m_setup.find("keep_files") != m_setup.end()) {
		m_keep_file = setup["keep_files"];
		m_setup.erase(m_setup.find("keep_files"));
	}

	if (m_setup.find("json") != m_setup.end()) {
		m_json = setup["json"];
		m_setup.erase(m_setup.find("json"));
	}

	auto it = NORMALISED_SIMULATOR_NAMES.find(simulator);
	if (it != NORMALISED_SIMULATOR_NAMES.end()) {
		m_norm_simulator = it->second;
	}
	else {
		m_norm_simulator = m_simulator;
	}

	if (m_norm_simulator == "nmpm1") {
		if (m_setup.find("ess") != m_setup.end()) {
			if (m_setup["ess"].get<bool>()) {
				m_norm_simulator = "ess";
			}
		}
	}
}
namespace {
bool file_is_empty(std::string filename)
{
	std::ifstream file(filename);
	return file.good() && (file.peek() == std::ifstream::traits_type::eof());
}
}  // namespace

void Slurm::do_run(NetworkBase &network, Real duration) const
{
	if (m_write_binnf) {
		// TODO Check whether file exists
		auto json = output_json(network, duration);
		std::ofstream file_out;
		if (m_json) {
			file_out.open(m_path + ".json", std::ios::binary);
			file_out << json;
		}
		else {
			file_out.open(m_path + ".cbor", std::ios::binary);
			Json::to_cbor(json, file_out);
		}
		file_out.close();
		system("ls > /dev/null");
	}

	if (m_exec_python) {
		std::vector<std::string> params;

		// Get the current working directory
		std::string current_dir = std::string(get_current_dir_name()) + "/";

		// Flag for temporary workaround for allocating several hicanns
		bool init_reticles = false;
		std::string wafer;

		// Set simulator dependent options for slurm
		if (m_norm_simulator == "nmpm1") {
			std::string hicann = "367";
			wafer = "33";
			if (m_setup.find("hicann") != m_setup.end()) {
				Json j_hicann = m_setup["hicann"];
				hicann = j_hicann.dump(-1);
				if (hicann[0] == '\"' || hicann[0] == '\'') {
					hicann.erase(hicann.begin());
					hicann.erase(hicann.end() - 1);
				}
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

			// Temporary solution to avoid L1 locking issues when using several
			// hicanns
			/*if (m_setup.find("hicann") != m_setup.end()) {
			    if (m_setup["hicann"].is_array() &&
			        m_setup["hicann"].size() >= 3) {
			        init_reticles = true;
			    }
			}*/

			params = std::vector<std::string>({
			    "-p",
			    "experiment",
			    "--wmod",
			    wafer,
			    "--hicann",
			    hicann,
			    "bash",
			    "-c",
			});
		}
		else if (m_norm_simulator == "spikey") {
			size_t station = 538;
			if (m_setup.find("station") != m_setup.end()) {
				station = m_setup["station"];
			}
			else {
				network.logger().warn("cypress", "Using default spikey 538!");
			}
			params = std::vector<std::string>({
			    "-p",
			    "spikey",
			    "--gres",
			    "station" + std::to_string(station),
			    "bash",
			    "-c",
			});
		}
		else if (m_norm_simulator == "ess") {
			params = std::vector<std::string>({
			    "-c",
			    "sbatch -p simulation -c 8 --mem 30G" /*
			    "-p", "simulation", "-c", "8", "--mem", "30G", "bash", "-c"*/
			    ,
			});
		}
		else {
			throw NotSupportedException("Simulator " + m_simulator +
			                            " not supported with Slurm!");
		}

		// Add the bash script executed by srun
		std::string script = "ls > /dev/null;";
		if (init_reticles) {
			script.append("sthal_init_reticles.py " + wafer +
			              " -z >text.txt &\n");
		}

		if (m_norm_simulator == "nmpm1") {
			script.append("run_nmpm_software ");
		}

		script.append(m_json_path + " " + m_path);
        if(!m_json){
            script.append(" 1");
        }
        script.append(";ls >/dev/null;wait");

		// Synchronize files on servers (Heidelberg setup...)
		system("ls > /dev/null");

		// Run the srun (non-blocking at this point)
		std::string slurm;
		if (m_norm_simulator == "ess") {
			slurm = "bash";
			params.back().append(" -W <<EOF\n#!/bin/sh\n" + script + "\nEOF");
		}
		else {
			params.push_back(script);
			slurm = "srun";
		}

		for (size_t i = 0; i < 3; i++) {
			Process proc(slurm, params);

			// std::ofstream log_stream(m_path);

			// This process is only meant to cover the first milliseconds before
			// all communication is switched to files.
			std::thread log_thread_beg(Process::generic_pipe,
			                           std::ref(proc.child_stdout()),
			                           std::ref(std::cout));

			// Continiously track stderr of child
			std::thread err_thread(Process::generic_pipe,
			                       std::ref(proc.child_stderr()),
			                       std::ref(std::cerr));

			// Wait for process to finish
			int res = proc.wait();
			// Synchronize files on servers (Heidelberg setup...)
			system("ls > /dev/null");
			std::string suffix = m_json ? "_res.json" : "_res.cbor";
			if (res != 0 || file_is_empty(m_path + suffix)) {
				err_thread.join();
				log_thread_beg.join();
				if (i < 2) {
					continue;
				}
				// Explicitly state if the process was killed by a signal
				if (res < 0) {
					network.logger().error(
					    "cypress", "Simulator child process killed by signal " +
					                   std::to_string(-res));
				}
				throw ExecutionError(
				    std::string("Error while executing the simulator, see ") +
				    m_path + " for the simulators stderr output");
			}
			err_thread.join();
			log_thread_beg.join();
			break;
		}
	}

	if (m_read_results) {
		std::ifstream file_in;
		std::string suffix = m_json ? ".json" : ".cbor";
		file_in.open(m_path + suffix, std::ios::binary);
		auto json = Json::from_cbor(file_in);
		read_json(json, network);

		if (!m_keep_file) {
			unlink((m_path + suffix).c_str());
			unlink((m_path + "_res" + suffix).c_str());
		}
	}
}

Slurm::~Slurm() = default;
}  // namespace cypress
