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
 * @file resource.hpp
 *
 * Provides functionality to access binary data embedded in the executable as
 * a regular file. Currently the embedded data is just extracted into a
 * temporary file on first access.
 *
 * This functionality is used to embed required python scripts to access the
 * neuromorphic hardware platform directly in the executable -- which allows
 * to ship applications built ontop of Cypress as a single file.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_UTIL_RESOURCE_HPP
#define CYPRESS_UTIL_RESOURCE_HPP

#include <string>
#include <vector>

namespace cypress {
/**
 * The Resource class is used to manage (binary) resources.
 */
class Resource {
private:
	const std::vector<char> m_data;
	mutable std::string m_filename;
	mutable int m_fd;

public:
	/**
	 * Creates the resource.
	 */
	Resource(const std::vector<char> &data) : m_data(data), m_fd(-1) {}

	/**
	 * Creates the resource.
	 */
	Resource(const std::string &data)
	    : m_data(data.begin(), data.end()), m_fd(-1)
	{
	}

	/**
	 * Destroys the resource and deletes any temporary file created to access
	 */
	~Resource();

	/**
	 * Returns a reference at the vector containing the data.
	 */
	const std::vector<char> &data() const { return m_data; }

	/**
	 * Returns the name of a file which will contain the data stored in the
	 * resource.
	 */
	const std::string &open() const;
};
}
#endif /* CYPRESS_UTIL_RESOURCE_HPP */
