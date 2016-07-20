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

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>

#include <cypress/backend/power/power.hpp>
#include <cypress/util/process.hpp>

namespace cypress {

static void sleep(float delay)
{
	if (delay > 0.0) {
		std::chrono::duration<float> p =
		    std::chrono::milliseconds(size_t(1000.0 * delay));
		std::this_thread::sleep_for(p);
	}
}

PowerManagementBackend::PowerManagementBackend(
    std::unique_ptr<PowerDevice> device, std::unique_ptr<Backend> backend)
    : m_device(std::move(device)), m_backend(std::move(backend))
{
}

PowerManagementBackend::~PowerManagementBackend()
{
	// Do nothing here
}

void PowerManagementBackend::do_run(NetworkBase &network, float duration) const
{
	const float delay = 4.0;
	std::string dev_name = m_backend->name();  // Get the device name

	// Try to repeat the experiment a few times, but not more than a few times
	int repeat = 4;
	while (repeat > 0) {
		// Make sure the device is switched on
		if (!m_device->state(dev_name) && m_device->switch_on(dev_name)) {
			sleep(delay);  // Sleep for five seconds
		}
		try {
			// Run the network using the given backend
			m_backend->run(network, duration);
			return;
		}
		catch (std::runtime_error e) {
			// If an exception occurred, switch off the device. If this was
			// successful, try again
			if (repeat > 1) {
				if (m_device->switch_off(dev_name)) {
					std::cerr
					    << "Error while executing the simulation, going "
					       "to power-cycle the neuromorphic device and retry!"
					    << std::endl;
					sleep(delay);
					repeat--;
					continue;
				}
			}
			throw;
		}
	}
}
}

