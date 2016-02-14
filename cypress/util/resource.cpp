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

#include <mutex>

#include <stdlib.h>
#include <unistd.h>

#include <cypress/util/resource.hpp>

namespace cypress {
Resource::~Resource()
{
	if (m_fd >= 0) {
		close(m_fd);
		unlink(m_filename.c_str());
		m_fd = -1;
		m_filename = "";
	}
}

const std::string &Resource::open() const
{
	// Protect access to this method
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	// Create a temporary file and dump the resource content there
	if (m_filename.empty()) {
		m_filename = "/tmp/cypress_XXXXXX";
		m_fd = mkstemp(&m_filename[0]);
		write(m_fd, &m_data[0], m_data.size());
	}
	return m_filename;
}
}
