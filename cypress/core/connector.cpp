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

#include <algorithm>
#include <limits>

#include <cypress/core/connector.hpp>

namespace cypress {

/**
 * Class Connector
 */

std::unique_ptr<AllToAllConnector> Connector::all_to_all(float weight,
                                                         float delay)
{
	return std::move(std::make_unique<AllToAllConnector>(weight, delay));
}

std::unique_ptr<OneToOneConnector> Connector::one_to_one(float weight,
                                                         float delay)
{
	return std::move(std::make_unique<OneToOneConnector>(weight, delay));
}

/*
 * Method instantiate_connections
 */

std::vector<Connection> instantiate_connections(
    const std::vector<ConnectionDescriptor> &descrs)
{
	// Connection which is greater than all other possible valid connections,
	// but smaller than the smallest invalid connection
	static const Connection LAST_VALID(
	    std::numeric_limits<PopulationIndex>::max(),
	    std::numeric_limits<PopulationIndex>::max(),
	    std::numeric_limits<NeuronIndex>::max(),
	    std::numeric_limits<NeuronIndex>::max(), 1.0, 0.0);

	// Iterate over the connection descriptors and instantiate them
	// TODO: Parallelise
	std::vector<Connection> res;
	ssize_t cursor = 0;
	for (const auto &descr : descrs) {
		// Make sure there is enough memory in the result vector
		const size_t size = descr.size();
		if ((res.size() - cursor) < size) {
			res.resize(cursor + size);
		}

		// Instantiate the connections
		const size_t written = descr.connect(&res[cursor]);

		// Sort the generated connections, search the first invalid connection
		const auto it_begin = res.begin() + cursor;
		const auto it_end = res.begin() + (cursor + written);
		std::sort(it_begin, it_end);
		auto it = std::upper_bound(it_begin, it_end, LAST_VALID);

		// Place the cursor after the first invalid connection
		cursor = it - res.begin();
	}

	// Resize the result to the cursor position
	res.resize(cursor);
	return res;
}

/*
 * Class AllToAllConnector
 */

const std::string AllToAllConnector::m_name = "AllToAllConnector";

size_t AllToAllConnector::connect(const ConnectionDescriptor &descr,
                                  Connection tar_mem[]) const
{
	size_t i = 0;
	for (NeuronIndex src = descr.nid_src0(); src < descr.nid_src1(); src++) {
		for (NeuronIndex tar = descr.nid_tar0(); tar < descr.nid_tar1();
		     tar++) {
			tar_mem[i++] = Connection(descr.pid_src(), descr.pid_tar(), src,
			                          tar, weight(), delay());
		}
	}
	return i;
}

/*
 * Class OneToOneConnector
 */

const std::string OneToOneConnector::m_name = "OneToOneConnector";

size_t OneToOneConnector::connect(const ConnectionDescriptor &descr,
                                  Connection tar_mem[]) const
{
	size_t i = 0;
	size_t n = descr.nsrc();  // == descr.ntar()
	for (; i < n; i++) {
		tar_mem[i] =
		    Connection(descr.pid_src(), descr.pid_tar(), descr.nid_src0() + i,
		               descr.nid_tar0() + i, weight(), delay());
	}
	return i;
}
}
