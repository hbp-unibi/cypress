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

#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>

#include <cypress/backend/power/power.hpp>
#include <cypress/util/process.hpp>

namespace cypress {
namespace {
using namespace std::chrono;

static void sleep(float delay)
{
	if (delay > 0.0) {
		std::chrono::duration<float> p =
		    std::chrono::milliseconds(size_t(1000.0 * delay));
		std::this_thread::sleep_for(p);
	}
}

/**
 * Class internally used to switch off the device once a certain timeout has
 * passed or when the application exits.
 */
class PowerOffManager {
private:
	static constexpr float TIMEOUT =
	    10.0;  // Switch off after ten seconds of inactivity

	using Time = steady_clock::time_point;

	/**
	 * Class managing a device which is currently on the "PowerOff" list.
	 */
	class Descr {
	private:
		std::shared_ptr<PowerDevice> m_device;
		std::string m_name;
		Time m_time;

	public:
		Descr(std::shared_ptr<PowerDevice> device, const std::string &name)
		    : m_device(std::move(device)),
		      m_name(name),
		      m_time(steady_clock::now())
		{
		}

		void switch_off() { m_device->switch_off(m_name); }

		bool try_switch_off(Time t)
		{
			auto dt = duration_cast<milliseconds>(t - m_time).count();
			if (dt >= TIMEOUT * 1000.0) {
				switch_off();
				return true;
			}
			return false;
		}

		const std::string &name() { return m_name; }
	};

	std::atomic<bool> m_done;
	std::vector<Descr> m_descrs;
	std::unique_ptr<std::thread> m_power_off_thread;
	std::mutex m_power_off_mutex;

	void create_thread()
	{
		if (m_power_off_thread) {
			return;
		}
		m_power_off_thread = std::make_unique<std::thread>([&] {
			while (!m_done) {
				{
					std::lock_guard<std::mutex> lock(m_power_off_mutex);
					Time t = steady_clock::now();
					for (auto it = m_descrs.begin(); it < m_descrs.end();
					     it++) {
						if (it->try_switch_off(t)) {
							it = m_descrs.erase(it);
						}
					}
				}
				sleep(0.1);  // Sleep for 100ms
			}
		});
	}

public:
	PowerOffManager() : m_done(false) {}

	/**
	 * Destructor -- switches off all devices and waits for the power off thread
	 * to exit.
	 */
	~PowerOffManager()
	{
		{
			std::lock_guard<std::mutex> lock(m_power_off_mutex);
			for (Descr &descr : m_descrs) {
				descr.switch_off();
			}
			m_descrs.clear();
		}
		m_done = true;
		m_power_off_thread->join();
	}

	/**
	 * Adds a device to the power off list.
	 */
	void add_device(std::shared_ptr<PowerDevice> device,
	                const std::string &name)
	{
		remove_device(name);

		std::lock_guard<std::mutex> lock(m_power_off_mutex);
		m_descrs.emplace_back(std::move(device), name);

		create_thread();
	}

	/**
	 * Remove the device from the power off list.
	 */
	void remove_device(const std::string &name)
	{
		std::lock_guard<std::mutex> lock(m_power_off_mutex);
		for (auto it = m_descrs.begin(); it < m_descrs.end(); it++) {
			if (it->name() == name) {
				it = m_descrs.erase(it);
			}
		}
	}
};
}

PowerManagementBackend::PowerManagementBackend(
    std::shared_ptr<PowerDevice> device, std::unique_ptr<Backend> backend)
    : m_device(std::move(device)), m_backend(std::move(backend))
{
}

PowerManagementBackend::~PowerManagementBackend()
{
	// Do nothing here
}

void PowerManagementBackend::do_run(NetworkBase &network, float duration) const
{
	static PowerOffManager power_off_manager;
	const float delay = 4.0;
	std::string dev_name = m_backend->name();  // Get the device name

	// Do not switch off the given device
	power_off_manager.remove_device(dev_name);

	// Try to repeat the experiment a few times, but not more than three times
	int repeat = 4;
	while (repeat > 0) {
		// Make sure the device is switched on
		if (!m_device->state(dev_name) && m_device->switch_on(dev_name)) {
			sleep(delay);  // Sleep for five seconds
		}
		try {
			// Run the network using the given backend
			m_backend->run(network, duration);
			power_off_manager.add_device(m_device, dev_name);
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
			power_off_manager.add_device(m_device, dev_name);
			throw;
		}
	}
}
}

