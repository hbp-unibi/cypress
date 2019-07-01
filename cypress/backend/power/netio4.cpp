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

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

#include <cypress/backend/power/netio4.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>

namespace cypress {
NetIO4::NetIO4(cypress::Json &config) { read_json_config(config); }

NetIO4::NetIO4(const std::string &config_filename)
{
	std::ifstream is(config_filename);
	if (is.good()) {
		Json obj;
        is >> obj;
		read_json_config(obj);
	}
}

void NetIO4::read_json_config(cypress::Json &config)
{
	m_has_config = true;
	m_addr = config["address"];
	m_port = config["port"];
	m_user = config["user"];
	m_passwd = config["password"];
	for (Json::iterator it = config["device_map"].begin();
	     it != config["device_map"].end(); ++it) {
		m_device_port_map.emplace(it.key(), it.value());
	}

	global_logger().debug(
	    "NETIO4", "Trying to connect to the NETIO4 device at " + m_addr + ":" +
	                  std::to_string(m_port) + " with username \"" + m_user +
	                  "\"...");
	if (connected()) {
		global_logger().info("NETIO4", "Connection successful.");
		for (auto elem : m_device_port_map) {
			global_logger().debug("NETIO4", elem.first + " --> port " +
			                                    std::to_string(elem.second));
		}
	}
	else {
		global_logger().fatal_error("NETIO4", "No connection to the device!");
	}
}

std::string NetIO4::control(const std::string &cmd) const
{
	// Only one thread at a time may communicate with the NETIO4 device!
	static std::mutex control_mutex;
	std::lock_guard<std::mutex> lock(control_mutex);

	std::stringstream ss_in, ss_out, ss_err;

	// Build the input
	ss_in << "login " << m_user << " " << m_passwd << "\n";
	if (!cmd.empty()) {
		ss_in << cmd << "\n";
	}

	// Call netcat to communicate with the device
	if (cypress::Process::exec("nc",
	                           {"-w", "5", m_addr, std::to_string(m_port)},
	                           ss_in, ss_out, ss_err) != 0) {
		throw std::runtime_error(
		    "NETIO4: Error while executing netcat (nc), make sure the program "
		    "is installed and there is a connection to the device!");
	}

	// Read the response and make sure the commands were processed correctly
	std::string line, res;
	bool first = true;
	while (std::getline(ss_out, line)) {
		if (first) {
			first = false;
		}
		else if (line.size() < 3 || line.substr(0, 3) != "250") {
			throw std::runtime_error("NETIO4: Error parsing device response!");
		}
		else {
			res = line.substr(4, line.size() - 5);  // Cut of the "/r"
		}
	}
	return res;
}

bool NetIO4::connected()
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

void NetIO4::switch_on(int port)
{
	if (m_has_config) {
		control(std::string("port ") + std::to_string(port) + " 1");
	}
}

void NetIO4::switch_off(int port)
{
	if (m_has_config) {
		control(std::string("port ") + std::to_string(port) + " 0");
	}
}

bool NetIO4::state(int port)
{
	if (m_has_config) {
		std::string res = control("port list");
		if (port > 0 && size_t(port - 1) < res.size()) {
			return res[port - 1] == '1';
		}
		else {
			throw std::runtime_error("Invalid port number");
		}
	}
	return false;
}

int NetIO4::device_port(const std::string &device)
{
	auto it = m_device_port_map.find(device);
	if (it != m_device_port_map.end()) {
		return it->second;
	}
	return -1;
}

bool NetIO4::state(const std::string &device)
{
	int port = device_port(device);
	if (port >= 0) {
		return state(port);
	}
	return false;
}

bool NetIO4::switch_on(const std::string &device)
{
	int port = device_port(device);
	if (port >= 0) {
		switch_on(port);
		return true;
	}
	return false;
}

bool NetIO4::switch_off(const std::string &device)
{
	int port = device_port(device);
	if (port >= 0) {
		switch_off(port);
		return true;
	}
	return false;
}
}  // namespace cypress
