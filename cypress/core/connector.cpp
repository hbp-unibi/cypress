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
#include <iostream>

namespace cypress {

/*
 * Method instantiate_connections
 */

// Connection which is greater than all other possible valid connections,
// but smaller than the smallest invalid connection
static const LocalConnection LAST_VALID(std::numeric_limits<NeuronIndex>::max(),
                                        std::numeric_limits<NeuronIndex>::max(),
                                        1.0, 0.0);

std::vector<std::vector<LocalConnection>> instantiate_connections(
    const std::vector<ConnectionDescriptor> &descrs)
{
	// Iterate over the connection descriptors and instantiate them
	std::vector<std::vector<LocalConnection>> res(descrs.size());
	for (size_t i = 0; i < descrs.size(); i++) {
		descrs[i].connect(res[i]);  // Sort the generated connections
		std::sort(res[i].begin(), res[i].end());
		// Resize the connection list to end at the first invalid connection
		auto it = std::upper_bound(res[i].begin(), res[i].end(), LAST_VALID);
		res[i].resize(it - res[i].begin());
	}
	return res;
}

/*
 * Class AllToAllConnector
 */

void AllToAllConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<LocalConnection> &tar) const
{
	if ((!descr.connector().allow_self_connections()) &&
	    descr.pid_src() == descr.pid_tar()) {
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				if (n_src != n_tar) {
					tar.emplace_back(n_src, n_tar, *m_synapse);
				}
			}
		}
	}
	else {
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				tar.emplace_back(n_src, n_tar, *m_synapse);
			}
		}
	}
}

/*
 * Class OneToOneConnector
 */

void OneToOneConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<LocalConnection> &tar) const
{
	for (NeuronIndex i = 0; i < descr.nsrc(); i++) {
		tar.emplace_back(descr.nid_src0() + i, descr.nid_tar0() + i,
		                 *m_synapse);
	}
}

/*
 * Class FromListConnector
 */

void FromListConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<LocalConnection> &tar) const
{
	tar = m_connections;
	for (auto it = tar.begin(); it != tar.end();) {
		if ((*it).src >= descr.nid_src0() && (*it).src < descr.nid_src1() &&
		    (*it).tar >= descr.nid_tar0() && (*it).tar < descr.nid_tar1()) {
			++it;
		}
		else {
			it = tar.erase(it);
		}
	}
}

bool FromListConnector::group_connect(const ConnectionDescriptor &) const
{
	return false;
}
}  // namespace cypress
