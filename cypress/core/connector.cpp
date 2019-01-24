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
	if ((!descr.connector().allow_self_connections()) &&
	    descr.pid_src() == descr.pid_tar()) {
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				if (n_src != n_tar) {
					tar.emplace_back(descr.pid_src(), descr.pid_tar(), n_src,
					                 n_tar, *m_synapse);
				}
			}
		}
	}
	else {
		for (NeuronIndex n_src = descr.nid_src0(); n_src < descr.nid_src1();
		     n_src++) {
			for (NeuronIndex n_tar = descr.nid_tar0(); n_tar < descr.nid_tar1();
			     n_tar++) {
				tar.emplace_back(descr.pid_src(), descr.pid_tar(), n_src, n_tar,
				                 *m_synapse);
			}
		}
	}
}

bool AllToAllConnector::group_connect(const ConnectionDescriptor &descr,
                                      GroupConnection &tar) const
{
	if (!m_synapse) {
		throw;
	}
	tar = GroupConnection(descr.pid_src(), descr.pid_tar(), descr.nid_src0(),
	                      descr.nid_src1(), descr.nid_tar0(), descr.nid_tar1(),
	                      m_synapse->parameters(), 0, "AllToAllConnector",
	                      m_synapse->name(),
	                      descr.connector().allow_self_connections());
	return true;
}

/*
 * Class OneToOneConnector
 */

void OneToOneConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<Connection> &tar) const
{
	for (NeuronIndex i = 0; i < descr.nsrc(); i++) {
		tar.emplace_back(descr.pid_src(), descr.pid_tar(), descr.nid_src0() + i,
		                 descr.nid_tar0() + i, *m_synapse);
	}
}

bool OneToOneConnector::group_connect(const ConnectionDescriptor &descr,
                                      GroupConnection &tar) const
{
	tar = GroupConnection(descr.pid_src(), descr.pid_tar(), descr.nid_src0(),
	                      descr.nid_src1(), descr.nid_tar0(), descr.nid_tar1(),
	                      m_synapse->parameters(), 0, "OneToOneConnector",
	                      m_synapse->name(),
	                      descr.connector().allow_self_connections());
	return true;
}

/*
 * Class FromListConnector
 */

void FromListConnector::connect(const ConnectionDescriptor &descr,
                                std::vector<Connection> &tar) const
{
	for (const LocalConnection &c : m_connections) {
		if (c.SynapseParameters.size() > 2) {
			// TODO
			throw ExecutionError(
			    "Only static synapses are supported for this backend!");
		}
		StaticSynapse temp(c.SynapseParameters);
		if (c.src >= descr.nid_src0() && c.src < descr.nid_src1() &&
		    c.tar >= descr.nid_tar0() && c.tar < descr.nid_tar1()) {
			tar.emplace_back(descr.pid_src(), descr.pid_tar(), c.src, c.tar,
			                 temp);
		}
	}
}

bool FromListConnector::group_connect(const ConnectionDescriptor &,
                                      GroupConnection &) const
{
	return false;
}
}  // namespace cypress
