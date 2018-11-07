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

/**
 * @file power.hpp
 *
 * Contains an adapter backend which performs power management tasks, such as
 * automatically switchin the neuromorphic devices on/off and resetting them.
 *
 * @author Andreas Stöckel
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
	virtual ~PowerDevice(){};

	virtual bool has_config() = 0;

	/**
	 * Returns the state of the device with the given name. If the device cannot
	 * be controlled, false should be returned.
	 *
	 * @param device is the canonical backend name (returned by backend.name())
	 * of the device for which the state should be returned.
	 * @return true if the device is currently switched on, false in all other
	 * cases.
	 */
	virtual bool state(const std::string &device) = 0;

	/**
	 * Switches the device with teh given name on.
	 *
	 * @param device is the canonical backend name (returned by backend.name())
	 * of the device that should be controlled.
	 * @return true if the operation was successful, false otherwise.
	 */
	virtual bool switch_on(const std::string &device) = 0;

	/**
	 * Switches the device with teh given name off.
	 *
	 * @param device is the canonical backend name (returned by backend.name())
	 * of the device that should be controlled.
	 * @return true if the operation was successful, false otherwise.
	 */
	virtual bool switch_off(const std::string &device) = 0;
};

/**
 * The PowerManagementBackend is responsible for switching neuromorphic hardware
 * devices on and off when necessary and to automatically reset them in case
 * there is an error during the execution, which are sometimes caused by buggy
 * firmware. E.g. the Spikey device sometimes looses the USB connection, which
 * can only be solved by a power cycle. Furthermore, due to annoying noise being
 * emitted from the device, it is extremely important for the sanity of the
 * people
 * sitting in the same room, that the device is switched on exactly if
 * experments are running.
 */
class PowerManagementBackend : public Backend {
private:
	std::shared_ptr<PowerDevice> m_device;
	std::unique_ptr<Backend> m_backend;

	/**
	 * This method just forwards the given data to the backend instance.
	 */
	void do_run(NetworkBase &network, Real duration) const override;

public:
	/**
	 * Creates a new PowerManagementBackend instance.
	 *
	 * @param device is the device which is responsible for switching the power
	 * to the neuromorphic hardware.
	 * @param backend is the backend which actually executes the network.
	 */
	PowerManagementBackend(std::shared_ptr<PowerDevice> device,
	                       std::unique_ptr<Backend> backend);

	PowerManagementBackend(std::unique_ptr<Backend> backend,
	                       const std::string &config_filename = "");

	/**
	 * Destroys the PowerManagementBackend backend instance.
	 */
	~PowerManagementBackend() override;

	/**
	 * Returns the neuron types supported by the actual backend.
	 */
	std::unordered_set<const NeuronType *> supported_neuron_types()
	    const override
	{
		return m_backend->supported_neuron_types();
	}

	/**
	 * Returns the canonical name of the backend. Forward the call to the actual
	 * backend being wrapped by the PowerManagementBackend.
	 */
	std::string name() const override { return m_backend->name(); }
};
}  // namespace cypress

#endif /* CYPRESS_BACKEND_POWER_POWER_HPP */
