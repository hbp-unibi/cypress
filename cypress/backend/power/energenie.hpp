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

/**
 * @file energenie.hpp
 *
 * Code used to communicate with the energenie intelligent power outlet.
 *
 * @author Andreas Stöckel, Christoph Ostrau
 */

#ifndef CPPNAM_BACKEND_POWER_ENERGENIE_HPP
#define CPPNAM_BACKEND_POWER_ENERGENIE_HPP

#include <cypress/backend/power/power.hpp>
#include <cypress/util/json.hpp>

#include <map>
#include <string>

namespace cypress {
/**
 * Class used to communicate with a energenie intelligent power outlet. This is
 * used to switch neuromorphic hardware devices on and off, as bugs in the
 * hardware sometimes cause them to become responseless, causing experiments
 * to crash with an exception.
 */
class energenie : public PowerDevice {
private:
	std::string m_addr;
	std::string m_passwd;
	std::map<std::string, int> m_device_port_map;

	bool m_has_config = false;

	void read_json_config(cypress::Json &config);
	std::string control(const std::string &cmd = std::string()) const;

public:
	/**
	 * Reads the energenie configuration from the given json configuration.
	 * The configuration must have the following format:
	 *
	 * {
	 *     "address": "192.168.240.254",
	 *     "password": "1",
	 *     "device_map": {
	 *         "spikey": 3,
	 *         "nmmc1": 2
	 *     }
	 * }
	 */
	energenie(cypress::Json &config);

	~energenie() override{};

	/**
	 * Reads the energenie configuration from the given JSON file. See the
	 * other constructor for the expected format. If the file is not found,
	 * the energenie class will simply do nothing.
	 *
	 * @param config_filename is the name of the file from which the
	 * configuration should be read.
	 */
	explicit energenie(const std::string &config_filename = ".energenie.json");

	/**
	 * Returns true if the configuration was successfully read in the
	 * constructor.
	 */
	bool has_config() { return m_has_config; }

	/**
	 * Returns true if the class can successfully communicate with the device.
	 */
	bool connected();

	/**
	 * Returns the port the given device is connected to. The device name must
	 * be the cannonical backend name as returned by "backend.name()". Returns
	 * -1 if no port is mapped to the given device.
	 */
	int device_port(const std::string &device);

	/**
	 * Returns true if the port with the given port id is currently switched on,
	 * false if the device is switched off. Throws an exception if the
	 * communication with the device fails or the given port does not exist.
	 */
	bool state(int port);

	void switch_on(int port);
	void switch_off(int port);

	bool state(const std::string &device) override;
	bool switch_on(const std::string &device) override;
	bool switch_off(const std::string &device) override;
};
}  // namespace cypress

#endif /* CPPNAM_BACKEND_POWER_ENERGENIE_HPP */
