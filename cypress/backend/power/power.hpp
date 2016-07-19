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

#ifndef CYPRESS_BACKEND_POWER_POWER_HPP
#define CYPRESS_BACKEND_POWER_POWER_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <cypress/core/backend.hpp>

namespace cypress {
/**
 * Abstract base class used to represent an device which is capable of switching
 * a power device.
 */
class PowerDevice {
public:
	virtual ~PowerDevice() {};

	virtual bool state(const std::string &device) = 0;
	virtual bool switch_on(const std::string &device) = 0;
	virtual bool switch_off(const std::string &device) = 0;
};

/**
 * The NMPI backend executes the program on a Neuromorphic Platform Interface
 * server.
 */
class PowerManagementBackend : public Backend {
private:
	std::unique_ptr<PowerDevice> m_device;
	std::unique_ptr<Backend> m_backend;

	/**
	 * This method just forwards the given data to the PyNN instance -- it is
	 * not called on the client computer, since the constructor intercepts the
	 * program flow.
	 */
	void do_run(NetworkBase &network, float duration) const override;

public:
	/**
	 * Creates a new PowerManagementBackend instance.
	 *
	 * @param device is the device which is responsible for switching the power
	 * to the neuromorphic hardware.
	 * @param backend is the backend which actually executes the network.
	 */
	PowerManagementBackend(std::unique_ptr<PowerDevice> device,
	                       std::unique_ptr<Backend> backend);

	/**
	 * Destroys the PowerManagementBackend backend instance.
	 */
	~PowerManagementBackend() override;

	/**
	 * Returns the canonical name of the backend.
	 */
	std::string name() const override { return m_backend->name(); }
};
}

#endif /* CYPRESS_BACKEND_POWER_POWER_HPP */
