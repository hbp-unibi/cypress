/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2018 Christoph Ostrau, Andreas Stöckel
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

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

#include <cypress/backend/power/energenie.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>

namespace cypress {
energenie::energenie(cypress::Json &config) { read_json_config(config); }

energenie::energenie(const std::string &config_filename)
{
	std::ifstream is(config_filename);
	if (is.good()) {
		Json obj;
		obj << is;
		read_json_config(obj);
	}
}

void energenie::read_json_config(cypress::Json &config)
{
	m_has_config = true;
	m_addr = config["address"];
	m_passwd = config["password"];
	for (Json::iterator it = config["device_map"].begin();
	     it != config["device_map"].end(); ++it) {
		m_device_port_map.emplace(it.key(), it.value());
	}

	global_logger().debug(
	    "energenie",
	    "Trying to connect to the energenie device at " + m_addr + "\"...");
	if (connected()) {
		global_logger().info("energenie", "Connection successful.");
		for (auto elem : m_device_port_map) {
			global_logger().debug("energenie", elem.first + " --> port " +
			                                       std::to_string(elem.second));
		}
	}
	else {
		global_logger().fatal_error("energenie",
		                            "No connection to the device!");
	}
}

std::string energenie::control(const std::string &cmd) const
{
	// Only one thread at a time may communicate with the energenie device!
	static std::mutex control_mutex;
	std::lock_guard<std::mutex> lock(control_mutex);

	std::stringstream ss_in, ss_out, ss_err;

	// Call curl to communicate with the device
	if (cmd == "") {
		if (cypress::Process::exec("curl",
		                           {"http://" + m_addr + "/login.html", "-s", "-d", "pw=1"},
		                           ss_in, ss_out, ss_err) != 0) {
			throw std::runtime_error(
			    "energenie: Error while executing curl, make sure the "
			    "program "
			    "is installed and there is a connection to the device!");
		}
	}
	else {
		if (cypress::Process::exec(
		        "curl", {"-sd", "\'" + cmd + "\'", "http://" + m_addr}, ss_in,
		        ss_out, ss_err) != 0) {
			throw std::runtime_error(
			    "energenie: Error while executing curl, make sure the "
			    "program "
			    "is installed and there is a connection to the device!");
		}
	}

	// Read the response and make sure the commands were processed correctly
	std::string line, res;
	while (std::getline(ss_out, line)) {
		res = line.substr(line.find("[") + 1);
	}
	return res;
}

bool energenie::connected()
{
	if (m_has_config) {
		try {
			control();
			return true;
		}
		catch (std::exception &err) {
			return false;
		}
	}
	return false;
}

void energenie::switch_on(int port)
{
	if (m_has_config) {
		control(std::string("cte") + std::to_string(port) + "=1");
	}
}

void energenie::switch_off(int port)
{
	if (m_has_config) {
		control(std::string("cte") + std::to_string(port) + "=0");
	}
}

bool energenie::state(int port)
{
	if (m_has_config) {
		std::string res = control("");
		if (port > 0 && size_t(port - 1) < res.size()) {
			return res[(port - 1) * 2] == '1';
		}
		else {
			throw std::runtime_error("Invalid port number");
		}
	}
	return false;
}

int energenie::device_port(const std::string &device)
{
	auto it = m_device_port_map.find(device);
	if (it != m_device_port_map.end()) {
		return it->second;
	}
	return -1;
}

bool energenie::state(const std::string &device)
{
	int port = device_port(device);
	if (port >= 0) {
		return state(port);
	}
	return false;
}

bool energenie::switch_on(const std::string &device)
{
	int port = device_port(device);
	if (port >= 0) {
		switch_on(port);
		return true;
	}
	return false;
}

bool energenie::switch_off(const std::string &device)
{
	int port = device_port(device);
	if (port >= 0) {
		switch_off(port);
		return true;
	}
	return false;
}
}  // namespace cypress
