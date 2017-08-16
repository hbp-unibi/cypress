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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

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

const std::string Resource::open_local(std::string filename) const
{
	// Protect access to this method
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	std::string current_dir = std::string(get_current_dir_name()) + "/";
	// Create a temporary file and dump the resource content there
	int fd = ::open((current_dir + filename).c_str(), (O_CREAT | O_WRONLY),
	                (S_IRWXU | S_IRWXG));
	write(fd, &m_data[0], m_data.size());
	return current_dir + filename;
}
}
