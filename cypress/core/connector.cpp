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

/*
 * Method instantiate_connections
 */

// Connection which is greater than all other possible valid connections,
// but smaller than the smallest invalid connection
static const Connection LAST_VALID(std::numeric_limits<PopulationIndex>::max(),
                                   std::numeric_limits<PopulationIndex>::max(),
                                   std::numeric_limits<NeuronIndex>::max(),
                                   std::numeric_limits<NeuronIndex>::max(), 1.0,
                                   0.0);

std::vector<Connection> instantiate_connections(
    const std::vector<ConnectionDescriptor> &descrs)
{
	// Iterate over the connection descriptors and instantiate them
	std::vector<Connection> res;
	for (const auto &descr : descrs) {
		descr.connect(res);
	}

	// Sort the generated connections
	std::sort(res.begin(), res.end());

	// Resize the connection list to end at the first invalid connection
	auto it = std::upper_bound(res.begin(), res.end(), LAST_VALID);
	res.resize(it - res.begin());

	return res;
}

/*
 * Class AllToAllConnector
 */

void AllToAllConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<Connection> &tar) const
{
	for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
	     n_src++) {
		for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
		     n_tar++) {
			tar.emplace_back(descr.pid_src(), descr.pid_tar(), n_src, n_tar,
			                 weight(), delay());
		}
	}
}

bool AllToAllConnector::group_connect(const ConnectionDescriptor &descr,
	                     GroupConnction &tar) const
{
	tar.psrc = descr.pid_src();
	tar.src0 = descr.nid_src0();
	tar.src1 = descr.nid_src1();
	tar.ptar = descr.pid_tar();
	tar.tar0 = descr.nid_tar0();
    tar.tar1 = descr.nid_tar1();
	tar.synapse = Synapse(m_weight, m_delay);
	tar.additional_parameter = 0;
    tar.connection_name = "AllToAllConnector";
    return true;
}

/*
 * Class OneToOneConnector
 */

void OneToOneConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<Connection> &tar) const
{
	for (size_t i = 0; i < descr.nsrc(); i++) {
		tar.emplace_back(descr.pid_src(), descr.pid_tar(), descr.nid_src0() + i,
		                 descr.nid_tar0() + i, weight(), delay());
	}
}

bool OneToOneConnector::group_connect(const ConnectionDescriptor &descr,
	                     GroupConnction &tar) const
{
	tar.psrc = descr.pid_src();
	tar.src0 = descr.nid_src0();
	tar.src1 = descr.nid_src1();
	tar.ptar = descr.pid_tar();
	tar.tar0 = descr.nid_tar0();
    tar.tar1 = descr.nid_tar1();
	tar.synapse = Synapse(m_weight, m_delay);
	tar.additional_parameter = 0;
    tar.connection_name = "OneToOneConnector";
    return true;
}

/*
 * Class FromListConnector
 */

void FromListConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<Connection> &tar) const
{
	for (const LocalConnection &c : m_connections) {
		if (c.src >= descr.nid_src0() && c.src < descr.nid_src1() &&
		    c.tar >= descr.nid_tar0() && c.tar < descr.nid_tar1()) {
			tar.emplace_back(descr.pid_src(), descr.pid_tar(), c.src, c.tar,
			                 c.synapse.weight, c.synapse.delay);
		}
	}
}

bool FromListConnector::group_connect(const ConnectionDescriptor &,
	                   GroupConnction &) const 
{
    return false;
}
}
